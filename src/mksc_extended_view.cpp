#include "mksc_extended_view.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "gba_bus.h"
#include "gba_ppu.h"
#include "runtime_bus_bridge.h"

extern "C" unsigned g_ws_extra_left;
extern "C" unsigned g_ws_extra_right;

namespace mksc {
namespace {

int (*g_previous_tilemap_provider)(int, int, int, std::uint16_t*) = nullptr;
int (*g_previous_bg_x_provider)(int, int, int, int*) = nullptr;
int (*g_previous_obj_attr_x_provider)(int, std::uint16_t, std::uint16_t,
                                      std::uint16_t, int*) = nullptr;
int (*g_previous_affine_filter_provider)(int, int) = nullptr;

struct HudRect {
    int x0;
    int y0;
    int x1;
    int y1;
};

// BG0 is a transparent race-HUD plane. These are the five authored groups
// visible in native 240x160 space. Deliberately exclude the center item box,
// countdown, and pause dialog so they remain centered.
constexpr HudRect kLeftHud[] = {
    {0, 0, 100, 28},    // coins + lap
    {0, 28, 32, 128},   // running order, including animated portrait spill
    {0, 120, 64, 160},  // current position
};
constexpr HudRect kRightHud[] = {
    {140, 0, 240, 28},   // timer + race indicators
    {160, 72, 240, 160}, // minimap
};

std::uint16_t read16(const std::uint8_t* io, unsigned offset) {
    return static_cast<std::uint16_t>(
        io[offset] | (static_cast<std::uint16_t>(io[offset + 1]) << 8));
}

bool race_register_layout(const std::uint8_t* io) {
    const std::uint16_t dispcnt = read16(io, 0x00);
    const std::uint16_t mode = dispcnt & 0x0007u;
    // Mario Kart changes from Mode 0 (HUD/horizon) to Mode 1 (affine road)
    // partway down every race frame.
    if (mode > 1) return false;
    if ((dispcnt & 0x1F00u) != 0x1F00u) return false; // BG0-3 + OBJ
    if ((dispcnt & 0xE000u) != 0) return false;       // transition windows

    const std::uint16_t bg0cnt = read16(io, 0x08);
    const std::uint16_t bg1cnt = read16(io, 0x0A);
    const std::uint16_t bg2cnt = read16(io, 0x0C);
    const std::uint16_t bg3cnt = read16(io, 0x0E);

    // During a race the perspective road lives in the sole 512-pixel-wide
    // regular background. The other three rings are 256 pixels wide, with
    // BG0 at HUD priority and the scenery layers behind it. This signature
    // excludes the title, selection screens, and Mode 1 cup previews.
    const bool standard_race =
        (bg1cnt & 0xC003u) == 0x0003u &&
        (bg2cnt & 0xC003u) == 0x4003u;

    // The pause overlay preserves the Mode 0 horizon layout, but switches
    // the lower Mode 1 scanlines to a second road buffer. Keep extending the
    // underlying race scene there; sprites still draw the menu only once in
    // the native-width center.
    const bool paused_lower_race =
        mode == 1 &&
        (bg1cnt & 0xC003u) == 0x0001u &&
        (bg2cnt & 0xC003u) == 0xC001u;

    return (bg0cnt & 0xC003u) == 0x0000u &&
           (bg3cnt & 0xC003u) == 0x0003u &&
           (standard_race || paused_lower_race);
}

bool compute_race_layout(const gba::GbaBus* bus) {
    if (!bus) return false;
    const std::uint8_t* io = bus->io().raw();
    if (!race_register_layout(io)) return false;

    // Race results preserve the live road's display-control and background
    // registers, so the PPU signature alone also matched the timing and
    // standings screens. Require the stable left "LAP" tiles from the actual
    // race HUD, then accept either top-right "TIME" or the lap-transition
    // "LAP" variant. Pauses and overlays retain this map underneath their
    // centered UI; results replace it.
    // Check the canonical top-half HUD map in screen block 7. Live races use
    // block 7 for this map even while the lower Mode 1 phase temporarily
    // selects block 6. Results reverse that arrangement and leave a stale
    // race-like map in block 6, so following the instantaneous BG0CNT would
    // still misclassify the lower half of results (and reject live Mode 1).
    constexpr std::size_t map_base = 7u * 0x800u;
    constexpr std::size_t kVramBytes = 96u * 1024u;
    constexpr unsigned kLeftLapTiles[] = {4, 5, 6};
    constexpr unsigned kRightTextTiles[] = {18, 19, 20};
    constexpr std::uint16_t kLapValues[] = {0x0044u, 0x0045u, 0x0046u};
    constexpr std::uint16_t kTimeValues[] = {0x004Au, 0x004Bu, 0x004Cu};
    const std::uint8_t* vram = bus->vram_ptr();
    if (!vram || map_base + (kRightTextTiles[2] + 1u) * 2u > kVramBytes)
        return false;
    for (unsigned i = 0; i < 3; ++i) {
        // Palette-bank and flip attributes are applied while the frame is
        // rendered; the tile identity in bits 0..9 is the stable signature.
        const std::uint16_t entry = read16(vram, static_cast<unsigned>(
            map_base + kLeftLapTiles[i] * 2u));
        if ((entry & 0x03FFu) != kLapValues[i]) return false;
    }
    bool right_time = true;
    bool right_lap = true;
    for (unsigned i = 0; i < 3; ++i) {
        const std::uint16_t entry = read16(vram, static_cast<unsigned>(
            map_base + kRightTextTiles[i] * 2u)) & 0x03FFu;
        right_time = right_time && entry == kTimeValues[i];
        right_lap = right_lap && entry == kLapValues[i];
    }
    return right_time || right_lap;
}

bool race_layout(const gba::GbaBus* bus) {
    // The providers below are hot (the BG remapper runs per output pixel).
    // Scene identity cannot change inside one emulated frame, so inspect the
    // game-owned tilemap only once per VBlank rather than six times per pixel.
    static const gba::GbaBus* cached_bus = nullptr;
    static unsigned long long cached_vblank = ~0ull;
    static bool cached_race = false;
    if (cached_bus != bus || cached_vblank != g_runtime_vblank_starts) {
        cached_bus = bus;
        cached_vblank = g_runtime_vblank_starts;
        cached_race = compute_race_layout(bus);
    }
    return cached_race;
}

void trace_scene_once(bool race, const std::uint8_t* io,
                      const gba::GbaBus* bus) {
    // This function is reached from the margin tile provider, potentially
    // tens of thousands of times per frame. On Windows getenv() takes the CRT
    // environment lock, so querying it per pixel made adaptive fullscreen
    // CPU-bound even when tracing was disabled.
    static const bool enabled = [] {
        const char* value = std::getenv("GBARECOMP_MKSC_WS_TRACE");
        return value && *value && *value != '0';
    }();
    if (!enabled) return;

    static std::uint16_t previous_dispcnt = 0xFFFFu;
    static bool previous_race = false;
    const std::uint16_t dispcnt = read16(io, 0x00);
    if (dispcnt == previous_dispcnt && race == previous_race) return;
    previous_dispcnt = dispcnt;
    previous_race = race;
    constexpr std::size_t map_base = 7u * 0x800u;
    constexpr unsigned kHudTiles[] = {4, 5, 6, 18, 19, 20};
    std::uint16_t hud[6] = {};
    const std::uint8_t* vram = bus ? bus->vram_ptr() : nullptr;
    if (vram) {
        for (unsigned i = 0; i < 6; ++i) {
            hud[i] = read16(vram, static_cast<unsigned>(
                map_base + kHudTiles[i] * 2u)) & 0x03FFu;
        }
    }
    std::fprintf(stderr,
        "[mksc:adaptive-view] race=%d DISPCNT=%04X "
        "BG=%04X/%04X/%04X/%04X HUD=%03X/%03X/%03X/%03X/%03X/%03X\n",
        race ? 1 : 0, dispcnt, read16(io, 0x08), read16(io, 0x0A),
        read16(io, 0x0C), read16(io, 0x0E), hud[0], hud[1], hud[2],
        hud[3], hud[4], hud[5]);
}

int race_tilemap_provider(int bg, int hw_x, int screen_y,
                          std::uint16_t* out_entry) {
    gba::GbaBus* bus = gbarecomp::active_bus();
    if (!bus) {
        return g_previous_tilemap_provider
            ? g_previous_tilemap_provider(bg, hw_x, screen_y, out_entry)
            : gba::kWsTilemapUnavailable;
    }

    const std::uint8_t* io = bus->io().raw();
    const bool race = race_layout(bus);
    trace_scene_once(race, io, bus);
    gba::g_ws_pillarbox = race ? 0 : 1;

    if (race && (hw_x < 0 || hw_x >= 240)) {
        // BG0 carries the lap/time HUD and must not repeat. BG1/3 carry the
        // scenery rings; the perspective road is affine BG2 in the Mode 1
        // phase and is extrapolated directly by the wide compositor.
        if (bg >= 1 && bg <= 3) return gba::kWsTilemapKeepWrapped;
        return gba::kWsTilemapUnavailable;
    }

    return g_previous_tilemap_provider
        ? g_previous_tilemap_provider(bg, hw_x, screen_y, out_entry)
        : gba::kWsTilemapUnavailable;
}

bool inside(const HudRect& rect, int x, int y) {
    return x >= rect.x0 && x < rect.x1 &&
           y >= rect.y0 && y < rect.y1;
}

int race_hud_bg_x_provider(int bg, int output_x, int screen_y,
                           int* out_hw_x) {
    gba::GbaBus* bus = gbarecomp::active_bus();
    if (bg != 0 || !out_hw_x || !bus || !race_layout(bus)) {
        return g_previous_bg_x_provider
            ? g_previous_bg_x_provider(bg, output_x, screen_y, out_hw_x)
            : 0;
    }

    const int hw_x = output_x - static_cast<int>(g_ws_extra_left);
    const int left_shift = -static_cast<int>(g_ws_extra_left);
    const int right_shift = static_cast<int>(g_ws_extra_right);

    // Draw each group at its adaptive destination first.
    for (const HudRect& rect : kLeftHud) {
        const HudRect destination = {
            rect.x0 + left_shift, rect.y0,
            rect.x1 + left_shift, rect.y1};
        if (inside(destination, hw_x, screen_y)) {
            *out_hw_x = hw_x - left_shift;
            return 1;
        }
    }
    for (const HudRect& rect : kRightHud) {
        const HudRect destination = {
            rect.x0 + right_shift, rect.y0,
            rect.x1 + right_shift, rect.y1};
        if (inside(destination, hw_x, screen_y)) {
            *out_hw_x = hw_x - right_shift;
            return 1;
        }
    }

    // Remove the original copy after it has moved. A negative action makes
    // only this transparent HUD-plane sample disappear; the road, scenery,
    // racers, and centered UI continue through their independent layers.
    if (left_shift != 0) {
        for (const HudRect& rect : kLeftHud)
            if (inside(rect, hw_x, screen_y)) return -1;
    }
    if (right_shift != 0) {
        for (const HudRect& rect : kRightHud)
            if (inside(rect, hw_x, screen_y)) return -1;
    }

    return g_previous_bg_x_provider
        ? g_previous_bg_x_provider(bg, output_x, screen_y, out_hw_x)
        : 0;
}

int race_hud_obj_x_provider(int oam_index, std::uint16_t attr0,
                            std::uint16_t attr1, std::uint16_t attr2,
                            int* out_x) {
    gba::GbaBus* bus = gbarecomp::active_bus();
    const int raw_x = static_cast<int>(attr1 & 0x01FFu);
    const int y = static_cast<int>(attr0 & 0x00FFu);
    // Race OAM slots 0-7 are its eight 8x8 minimap markers. Attribute 2 is
    // rewritten during the frame and differs between parked OAM and the
    // scanline latch, so identify them by their stable slot/shape/map bounds.
    if (out_x && bus && g_ws_extra_right != 0 &&
        race_layout(bus) &&
        oam_index >= 0 && oam_index < 8 &&
        (attr0 & 0xC300u) == 0 &&
        (attr1 & 0xC000u) == 0 &&
        raw_x >= 160 && raw_x < 240 &&
        y >= 72 && y < 160) {
        int x = raw_x;
        if (x & 0x0100) x -= 0x0200;
        *out_x = x + static_cast<int>(g_ws_extra_right);
        return 1;
    }

    return g_previous_obj_attr_x_provider
        ? g_previous_obj_attr_x_provider(
              oam_index, attr0, attr1, attr2, out_x)
        : 0;
}

int race_affine_filter_provider(int bg, int screen_y) {
    gba::GbaBus* bus = gbarecomp::active_bus();
    if (bg == 2 && bus && race_layout(bus)) return 1;
    return g_previous_affine_filter_provider
        ? g_previous_affine_filter_provider(bg, screen_y)
        : 0;
}

}  // namespace

void install_extended_view(std::uint32_t, std::uint32_t) {
    g_previous_tilemap_provider = gba::g_ws_tilemap_provider;
    gba::g_ws_tilemap_provider = race_tilemap_provider;
    if (gba::g_ws_bg_x_provider != race_hud_bg_x_provider) {
        g_previous_bg_x_provider = gba::g_ws_bg_x_provider;
        gba::g_ws_bg_x_provider = race_hud_bg_x_provider;
    }
    gba::g_ws_bg_x_provider_layers |= 1u << 0;
    if (gba::g_ws_obj_attr_x_provider != race_hud_obj_x_provider) {
        g_previous_obj_attr_x_provider = gba::g_ws_obj_attr_x_provider;
        gba::g_ws_obj_attr_x_provider = race_hud_obj_x_provider;
    }
    if (gba::g_ws_affine_filter_provider != race_affine_filter_provider) {
        g_previous_affine_filter_provider =
            gba::g_ws_affine_filter_provider;
        gba::g_ws_affine_filter_provider =
            race_affine_filter_provider;
    }
    gba::g_ws_authored_margin_layers = 1;
    gba::g_ws_pillarbox = 1;
    gba::g_ws_pillarbox_left = 0;
    gba::g_ws_pillarbox_right = 0;
}

}  // namespace mksc

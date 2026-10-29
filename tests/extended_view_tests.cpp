#include "mksc_extended_view.h"
#include "gba_bus.h"
#include "gba_ppu.h"
#include "runtime_bus_bridge.h"
#include <array>
#include <cstdio>
#include <memory>

// Exercise the real providers with an in-memory race HUD, without a game ROM.
extern "C" {
unsigned g_ws_extra_left = 0;
unsigned g_ws_extra_right = 0;
extern unsigned long long g_runtime_state_epoch;
}
namespace {
int failures = 0;
void check(bool condition, const char* message) {
    if (!condition) {
        if (failures < 10) std::fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}
void race(gba::GbaBus& bus, bool paused = false) {
    bus.write16(0x04000000, paused ? 0x1F01 : 0x1F00);
    bus.write16(0x04000008, 0x0700);
    bus.write16(0x0400000A, paused ? 1 : 3);
    bus.write16(0x0400000C, paused ? 0xC001 : 0x4003);
    bus.write16(0x0400000E, 3);
    for (unsigned i = 0; i < 3; ++i) {
        bus.write16(0x06003800 + (4 + i) * 2, 0x44 + i);
        bus.write16(0x06003800 + (18 + i) * 2, 0x4A + i);
    }
    ++g_runtime_vblank_starts;
}
}

int main() {
    auto bus = std::make_unique<gba::GbaBus>();
    gbarecomp::set_active_bus(bus.get());
    mksc::install_extended_view(0, 0);
    int x = -999;
    g_ws_extra_left = 120;
    g_ws_extra_right = 120;
    race(*bus);
    bus->write16(0x0300361E, 0x0880);
    bus->write8(0x0300361B, 2);
    bus->write16(0x06003808, 0); // no LAP label in battle
    for (unsigned i = 0; i < 4; ++i) {
        constexpr unsigned portrait[] = {4, 5, 5, 4};
        constexpr unsigned item[] = {0x74, 0x76, 0x76, 0x74};
        bus->write16(0x06003800 + (2 * 32 + i) * 2, portrait[i]);
        bus->write16(0x06003800 + (13 + i) * 2, item[i]);
    }
    std::uint16_t battle_entry = 0;
    check(gba::g_ws_tilemap_provider(2, -22, 70, &battle_entry) ==
              gba::kWsTilemapKeepWrapped && !gba::g_ws_pillarbox,
          "live battle HUD authorizes actual arena margins");
    for (int slot = 0; slot < 128; ++slot) {
        check(gba::g_ws_obj_attr_x_provider(slot, 0x8018, 29, 0x10E8, &x) == 1 && x == -91,
              "battle balloons stay attached to left-anchored portraits");
    }
    check(gba::g_ws_obj_attr_x_provider(8, 0x8078, 100, 0x10E8, &x) == 0,
          "world balloons are not moved with the battle HUD");
    bus->write16(0x06003800 + 2 * 32 * 2, 0); // rankings replace portrait frame
    ++g_runtime_vblank_starts;
    check(gba::g_ws_tilemap_provider(2, -22, 70, &battle_entry) ==
              gba::kWsTilemapUnavailable && gba::g_ws_pillarbox,
          "battle rankings retain native framing");
    bus->write16(0x0300361E, 0x0180);
    race(*bus);
    for (unsigned left : {0u, 1u, 22u, 23u, 48u, 93u, 120u}) {
        g_ws_extra_left = left;
        g_ws_extra_right = left + 1;
        for (int y = 112; y < 152; ++y) {
            for (int x = 0; x < 48; ++x) {
                int source_x = -999;
                // Left anchoring means a native HUD pixel at x is sampled at
                // output x, regardless of the centered scene's left margin.
                check(gba::g_ws_bg_x_provider(0, x, y, &source_x) == 1 &&
                      source_x == x, "entire 6x5 rim stays left anchored");
            }
        }
        for (int slot = 0; slot < 128; ++slot) {
            for (unsigned matrix = 0; matrix < 32; ++matrix) {
                int x = -999;
                const auto attr1 = static_cast<std::uint16_t>(0xC1F8 | (matrix << 9));
                const int moved = gba::g_ws_obj_attr_x_provider(slot, 0x0168,
                    attr1, 0x73C0, &x);
                check(left ? moved == 1 && x == -8 - static_cast<int>(left)
                           : moved == 0 && x == -999,
                      "needle follows rim for every OAM slot and affine index");
            }
        }
    }
    g_ws_extra_left = 120;
    g_ws_extra_right = 120;
    x = -999;
    // Sky-map dimensions belong to course content. Recognize the same live
    // HUD with either 256- or 512-pixel sky rings, and keep actual background
    // samples available in both margins rather than pillarboxing the race.
    for (unsigned bg1_size : {0u, 0x4000u}) {
        for (unsigned bg2_size : {0u, 0x4000u}) {
            for (unsigned bg3_size : {0u, 0x4000u}) {
                race(*bus);
                bus->write16(0x0400000A, bg1_size | 3);
                bus->write16(0x0400000C, bg2_size | 3);
                bus->write16(0x0400000E, bg3_size | 3);
                std::uint16_t entry = 0;
                check(gba::g_ws_tilemap_provider(2, -22, 70, &entry) ==
                          gba::kWsTilemapKeepWrapped && !gba::g_ws_pillarbox,
                      "course sky sizes do not pillarbox live race margins");
                check(gba::g_ws_bg_x_provider(0, 0, 0, &x) == 1 && x == 0,
                      "HUD stays edge anchored for every sky-map size");
            }
        }
    }
    race(*bus);
    bus->write16(0x0400000A, 0x0C07); // Sky Garden
    bus->write16(0x0400000C, 0x4D06);
    bus->write16(0x0400000E, 0x0F05);
    std::uint16_t entry = 0;
    check(gba::g_ws_tilemap_provider(2, 261, 70, &entry) ==
              gba::kWsTilemapKeepWrapped && !gba::g_ws_pillarbox,
          "course-specific scenery priorities keep real right-margin samples");
    race(*bus);
    bus->write16(0x04000008, 0x0400);
    check(gba::g_ws_tilemap_provider(2, -22, 70, &entry) ==
              gba::kWsTilemapUnavailable && gba::g_ws_pillarbox,
          "menu background cannot reuse a stale race HUD in screen block 7");
    race(*bus);
    check(gba::g_ws_bg_x_provider(0, 160, 116, &x) == -1,
          "original upper-right rim is removed");
    for (auto attrs : {std::array<std::uint16_t, 3>{0x0068, 0xC1F8, 0x73C0},
                       {0x0168, 0x81F8, 0x73C0}, {0x0168, 0xC1F8, 0x73C1},
                       {0x0168, 0xC1F8, 0x63C0}, {0x0169, 0xC1F8, 0x73C0},
                       {0x0168, 0xC1F9, 0x73C0}}) {
        check(gba::g_ws_obj_attr_x_provider(8, attrs[0], attrs[1], attrs[2], &x) == 0,
              "unrelated sprites keep their original placement");
    }
    check(gba::g_ws_obj_attr_x_provider(0, 95, 199, 0x5118, &x) == 1 && x == 319,
          "minimap markers still move right");
    check(gba::g_ws_bg_x_provider(0, 220, 80, &x) == 0,
          "centered HUD/dialog region stays unchanged");
    race(*bus, true);
    check(gba::g_ws_obj_attr_x_provider(7, 0x0168, 0xC1F8, 0x73C0, &x) == 1 && x == -128,
          "paused race keeps needle anchored");
    bus->write16(0x06003808, 0); // results replace the race's LAP tiles
    ++g_runtime_vblank_starts;
    check(gba::g_ws_obj_attr_x_provider(7, 0x0168, 0xC1F8, 0x73C0, &x) == 0,
          "results/menu scenes are excluded");
    check(gba::g_ws_bg_x_provider(0, 40, 116, &x) == 0,
          "results/menu backgrounds are excluded");
    race(*bus,true);
    check(gba::g_ws_obj_attr_x_provider(7,0x0168,0xc1f8,0x73c0,&x)==1,
          "race policy is populated before restore");
    bus->write16(0x06003808,0);++g_runtime_state_epoch;
    check(gba::g_ws_obj_attr_x_provider(7,0x0168,0xc1f8,0x73c0,&x)==0,
          "restore invalidates scene policy even at the same frame number");
    check(mksc::high_resolution_affine_provider(2,100),
          "affine sampling stays enabled before/without the race HUD");
    bus->write16(0x04000000,0x3f01);
    check(mksc::high_resolution_affine_provider(2,100),
          "course-intro windows do not switch sampling quality");
    check(!mksc::high_resolution_affine_provider(0,100) &&
          !mksc::high_resolution_affine_provider(2,160),
          "only actual affine BG2 raster rows are authorized");
    bus->write16(0x04000000,0x1f00);
    check(!mksc::high_resolution_affine_provider(2,100),"regular text BG2 remains native");
    gbarecomp::set_active_bus(nullptr);
    check(gba::g_ws_obj_attr_x_provider(7, 0x0168, 0xC1F8, 0x73C0, &x) == 0,
          "no active game is a no-op");
    std::printf("extended_view_tests: %s\n", failures ? "FAILED" : "OK");
    return failures ? 1 : 0;
}

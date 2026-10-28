// Qualification adapter for MKSC USA's audited relocated mono MP2K service.
// GNU/LLVM --wrap is used only in the isolated pilot. The portable native
// service itself contains no linker/OS assumptions; other linkers retain LLE.
#include "runtime_arm.h"
#include "runtime_bus_bridge.h"
#include "gba_bus.h"
#include "mp2k_native.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#define MKSC_REAL_INNER(name) __real_##name
#define MKSC_REAL(name) MKSC_REAL_INNER(name)
#define MKSC_WRAP_INNER(name) __wrap_##name
#define MKSC_WRAP(name) MKSC_WRAP_INNER(name)
extern "C" void MKSC_REAL(MKSC_SOUNDMAIN_SYMBOL)(void);
extern "C" unsigned long long g_runtime_cycles;

namespace {
uint64_t calls = 0, handled = 0, unsupported = 0;
struct Report {
    ~Report() {
        std::fprintf(stderr, "native_mp2k calls=%llu handled=%llu reference=%llu\n",
            static_cast<unsigned long long>(calls), static_cast<unsigned long long>(handled),
            static_cast<unsigned long long>(unsupported));
    }
} report;

uint32_t load32(const uint8_t* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

void capture(const char* phase, gba::GbaBus& bus, uint64_t index) {
    const char* directory = std::getenv("GBARECOMP_MP2K_PROBE_DIR");
    if (!directory || !*directory || index > 16) return;
    char name[64];
    if (index == 1 && std::strcmp(phase, "before") == 0)
        std::fprintf(stderr, "native_mp2k_window ie=%04x if=%04x dispstat=%04x\n",
            bus.io().read16(0x200), bus.io().read16(0x202), bus.io().read16(4));
    std::snprintf(name, sizeof(name), "call-%03llu-%s.bin", static_cast<unsigned long long>(index), phase);
    const auto path = std::filesystem::path(directory) / name;
    auto* file = std::fopen(path.string().c_str(), "wb");
    if (!file) { std::fprintf(stderr, "native_mp2k cannot write probe fixture\n"); std::abort(); }
    const uint32_t header[] = {0x4d324b43, 1, sizeof(g_cpu), 0x40000, 0x8000};
    bool ok = std::fwrite(header, sizeof(header), 1, file) == 1 &&
        std::fwrite(&g_runtime_cycles, sizeof(g_runtime_cycles), 1, file) == 1 &&
        std::fwrite(&g_cpu, sizeof(g_cpu), 1, file) == 1 &&
        std::fwrite(bus.ewram_ptr(), 0x40000, 1, file) == 1 &&
        std::fwrite(bus.iwram_ptr(), 0x8000, 1, file) == 1;
    ok = std::fclose(file) == 0 && ok;
    if (!ok) { std::fprintf(stderr, "native_mp2k incomplete probe fixture\n"); std::abort(); }
}

bool native(gba::GbaBus& bus) {
#if defined(MKSC_MP2K_NATIVE)
    // The title's ordinary ROM identity gate remains authoritative. Verify
    // that this call still executes the audited unmodified SDK relocation.
    if (bus.rom_size() != 0x400000 || g_cpu.R[15] != 0x03002600 ||
        g_runtime_resume_pc || bus.has_write_observer() ||
        !gbarecomp::native_service_window_allowed() ||
        g_runtime_bus_read_override || gba::g_rom_read16_override || gba::g_rom_read32_override ||
        g_runtime_fn_entry_hook || g_runtime_insn_trace ||
        std::memcmp(bus.iwram_ptr() + 0x2600, bus.rom_ptr() + 0x5f2f4, 0x330) != 0)
        return false;
    const gba::Mp2kNativeMemory memory{std::span(bus.rom_ptr(), bus.rom_size()),
        std::span(bus.ewram_ptr(), 0x40000), std::span(bus.iwram_ptr(), 0x8000)};
    const auto* stack = memory.read(g_cpu.R[13], 64);
    if ((g_cpu.R[13] & 3) || !stack || load32(stack + 24) != g_cpu.R[0]) return false;
    gba::Mp2kNativeCall call{g_cpu.R[0], g_cpu.R[5], g_cpu.R[8], g_cpu.R[4]};
    // This initial window pilot stays inside VBlank and before any enabled
    // VCount interrupt. GamePak IRQ has no clock-driven source during this
    // RAM-only operation. Other enabled IRQ sources retain the original route.
    // FIFO DMA still advances normally.
    // A service that could hit its SDK deadline or a visible-line boundary
    // takes the maintained original route, before any native mutation.
    const uint32_t vcount = bus_read_u8(0x04000006);
    const uint16_t ie = bus.io().read16(0x200);
    if (vcount < 160 || vcount >= 227 || (ie & ~0x2005u) || bus.io().irq_pending()) return false;
    uint32_t lines = 227 - vcount;
    const uint32_t compare = bus.io().dispstat() >> 8;
    if ((ie & 4) && compare > vcount && compare < 228) {
        if (compare <= vcount + 1) return false;
        lines = compare - vcount - 1;
    }
    const uint32_t deadline = load32(stack + 20);
    if (deadline) {
        if (deadline <= vcount + 1) return false;
        if (deadline - vcount - 1 < lines) lines = deadline - vcount - 1;
    }
    call.max_guest_cycles = lines * 1232;
    // Capture the saved ABI before any PCM/channel mutation. The service has
    // no retained pointers and makes no call back into guest execution.
    uint32_t saved[9];
    for (unsigned i = 0; i != 9; ++i) saved[i] = load32(stack + 28 + i * 4);
    const auto* info = memory.read(call.sound_info, 0x50);
    if (!info) return false;
    const uint32_t period = load32(info + 0x18);
    const bool reverb = info[5] != 0;
    gba::Mp2kNativeStats work;
    if (gba::mp2k_native_mix(memory, call, &work) != gba::Mp2kNativeResult::Complete) return false;
    for (unsigned i = 0; i != 8; ++i) g_cpu.R[i] = saved[i];
    for (unsigned i = 0; i != 4; ++i) g_cpu.R[8 + i] = saved[i];
    g_cpu.R[3] = saved[8];
    g_cpu.R[12] = period;
    g_cpu.R[13] += 64;
    const uint32_t target = saved[8];
    g_cpu.R[15] = target & ~1u;
    g_cpu.cpsr = (g_cpu.cpsr & ~(0xf0000020u)) | 0x60000000u | ((target & 1u) ? 0x20u : 0u);
    // Pilot cost model, deliberately not claimed cycle-exact. Closed-form
    // timing is calibrated/qualified against captured original calls before
    // adoption. Advance the connected devices rather than omitting their work.
    (void)reverb;
    runtime_tick(work.guest_cycle_estimate);
    if (!runtime_call_should_return(target)) runtime_dispatch_with_exchange(target);
    return true;
#else
    (void)bus;
    return false;
#endif
}
} // namespace

extern "C" void MKSC_WRAP(MKSC_SOUNDMAIN_SYMBOL)(void) {
    if (g_cpu.R[15] != 0x03002600 || g_runtime_resume_pc) {
        MKSC_REAL(MKSC_SOUNDMAIN_SYMBOL)();
        return;
    }
    ++calls;
    auto* bus = gbarecomp::active_bus();
    if (!bus) { ++unsupported; MKSC_REAL(MKSC_SOUNDMAIN_SYMBOL)(); return; }
    capture("before", *bus, calls);
    if (native(*bus)) ++handled;
    else { ++unsupported; MKSC_REAL(MKSC_SOUNDMAIN_SYMBOL)(); }
    capture("after", *bus, calls);
}

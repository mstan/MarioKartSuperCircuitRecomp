// MKSC USA: publish one completed four-port tuple into the SDK receive rings.
// This replaces the complete IRQ-side publication loop (DE60..DE9C), including
// wrap and caller scratch state. The surrounding IRQ samples SIO, checks ring
// capacity and performs the next transfer/timer effects through maintained code.
#include "runtime_arm.h"
#include "runtime_native_region.h"
#include "runtime_bus_bridge.h"
#include "gba_bus.h"
#include "mod_function_hooks.h"

#include <algorithm>
#include <array>
#include <cstring>

extern "C" void __real_gf_tfunc_0802DE60(void);

namespace {
constexpr auto kId = "mksc.native-receive-packet-v1";
constexpr std::uint32_t kEntry = 0x0802de60, kEnd = 0x0802de9c;
constexpr std::array<std::uint8_t, 60> kCode{
    0x60,0x46,0x01,0x68,0xfb,0x00,0x08,0x1c,0x0c,0x30,0xc0,0x18,
    0x02,0x68,0x10,0x31,0xc9,0x18,0x08,0x68,0x40,0x00,0x12,0x18,
    0x21,0x68,0x28,0x88,0x08,0x80,0x02,0x31,0x21,0x60,0x91,0x42,
    0x05,0xd1,0x61,0x46,0x08,0x68,0x0c,0x30,0xc0,0x18,0x00,0x68,
    0x20,0x60,0x10,0x34,0x02,0x35,0x01,0x37,0x03,0x2f,0xe1,0xdd};

std::uint32_t word(const std::uint8_t* p) {
    return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
        (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}

int publish(std::uint32_t, int thumb, ArmCpuState* cpu) {
    auto* bus = gbarecomp::active_bus();
    // Exact reviewed call state and immutable program descriptor. All four
    // rings, control slots and the captured tuple are disjoint private IWRAM.
    // Resume/partial-loop entries keep the generated implementation.
    if (!bus || !thumb || cpu != &g_cpu || cpu->R[15] != kEntry ||
        g_runtime_resume_pc || !(cpu->cpsr & CPSR_I_BIT) ||
        cpu->R[7] != 0 || cpu->R[4] != 0x03000218 ||
        cpu->R[5] != 0x03000270 || cpu->R[12] != 0x030002b8 ||
        !bus->rom_ptr() || bus->rom_size() != 0x400000 ||
        std::memcmp(bus->rom_ptr() + 0x2de60, kCode.data(), kCode.size()) ||
        word(bus->iwram_ptr() + 0x2b8) != 0x080de17c) return 0;
    for (unsigned port = 0; port != 4; ++port) {
        const auto base = 0x030001c8u + 16u * port;
        const auto* descriptor = bus->rom_ptr() + 0xde188 + port * 8;
        const auto tail = word(bus->iwram_ptr() + 0x218 + port * 16);
        if (word(descriptor) != base || word(descriptor + 4) != 8 ||
            (tail & 1u) || tail < base || tail >= base + 16) return 0;
    }
    // A direct generated caller may still owe instruction cycles. Rendezvous
    // before reading the tuple/rings; an exception retains the ordinary PC.
    gbarecomp::runtime_session_ram_dispatch_boundary(kEntry);

    RuntimeNativeRegion s{};
    std::copy_n(cpu->R, 16, s.cpu.R); s.cpu.cpsr = cpu->cpsr;
    s.waits = g_runtime_waits;
    s.ewram = bus->ewram_ptr(); s.iwram = bus->iwram_ptr();
    s.rom = bus->rom_ptr(); s.rom_bytes = std::uint32_t(bus->rom_size());
    struct Store { std::uint32_t address, value, width; };
    std::array<Store, 12> stores{};
    unsigned store_count = 0;
    const auto alu = [&](unsigned count) {
        s.cycles += count * (1u + s.waits.s16[8]);
        s.instructions += count;
    };
    const auto branch = [&](bool taken) {
        s.cycles += taken ? 3u + s.waits.n16[8] + 2u * s.waits.s16[8]
                          : 1u + s.waits.s16[8];
        ++s.instructions;
    };
    const auto access = [&](std::uint32_t pc, std::uint32_t address,
                            unsigned width, bool load) {
        const auto memory = s.runtime_mem_cycles(address, width, 0);
        s.cycles += (load ? 2u : 1u) + s.waits.n16[8] + memory +
            s.runtime_prefetch_adjust(memory + (load ? 1u : 0u), address, pc, 1);
        ++s.instructions;
        s.cpu.R[15] = pc;
    };
    const auto read32 = [&](std::uint32_t pc, std::uint32_t address) {
        access(pc, address, 4, true); return s.bus_read_u32(address);
    };
    const auto store = [&](std::uint32_t pc, std::uint32_t address,
                           std::uint32_t value, unsigned width) {
        access(pc, address, width, false);
        stores[store_count++] = {address, value, width};
    };
    // Compute the complete semantic packet publication before changing RAM.
    // Timing uses the maintained fetch/prefetch model in original access order;
    // no instruction dispatch, decode, yield, flags or device work is repeated.
    std::uint32_t last_value = 0, last_pointer = 0;
    for (unsigned port = 0; port != 4; ++port) {
        const auto offset = 8u * port, control = 0x03000218u + 16u * port;
        alu(1);
        const auto descriptor = read32(0x0802de62, 0x030002b8);
        alu(4);
        const auto base = read32(0x0802de6c, descriptor + 12 + offset);
        alu(2);
        const auto count = read32(0x0802de72, descriptor + 16 + offset);
        alu(2);
        const auto tail = read32(0x0802de78, control);
        const auto input = 0x03000270u + 2u * port;
        access(0x0802de7a, input, 2, true);
        last_value = s.bus_read_u16(input);
        store(0x0802de7c, tail, last_value, 2);
        alu(1);
        last_pointer = tail + 2;
        store(0x0802de80, control, last_pointer, 4);
        alu(1);
        const bool wrap = last_pointer == base + count * 2;
        branch(!wrap);
        if (wrap) {
            alu(1);
            const auto config = read32(0x0802de88, 0x030002b8);
            alu(2);
            last_value = read32(0x0802de8e, config + 12 + offset);
            store(0x0802de90, control, last_value, 4);
            last_pointer = 0x030002b8;
        }
        alu(4); branch(port != 3);
    }
    if (!gbarecomp::runtime_session_private_memory_begin(s, s.cycles)) return 0;
    for (unsigned i = 0; i != store_count; ++i) {
        const auto& st = stores[i];
        if (st.width == 2) s.bus_write_u16(st.address, st.value);
        else s.bus_write_u32(st.address, st.value);
    }
    s.cpu.R[0] = last_value; s.cpu.R[1] = last_pointer;
    s.cpu.R[2] = 0x03000208; s.cpu.R[3] = 24;
    s.cpu.R[4] = 0x03000258; s.cpu.R[5] = 0x03000278;
    s.cpu.R[7] = 4; s.cpu.R[15] = kEnd;
    s.cpu.cpsr = (s.cpu.cpsr & 0x0fffffffu) | 0x20000000u;
    gbarecomp::runtime_session_private_memory_end(s);
    return 1;
}

struct Register {
    Register() {
        gba_mod_register_function_entry_plugin(kId, kEntry, 1, publish);
        gba_mod_set_function_hook_enabled(kId, 1);
    }
} registration;
}

extern "C" void __wrap_gf_tfunc_0802DE60(void) {
    // Catalog changes may clear optional hook enablement. This service is a
    // build-selected simulation profile, independent of user mod selections.
    gba_mod_set_function_hook_enabled(kId, 1);
    if (!gba_mod_function_entry(kEntry, 1, &g_cpu)) __real_gf_tfunc_0802DE60();
}

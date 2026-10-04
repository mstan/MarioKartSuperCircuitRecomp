#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../src/mksc_flash_dispatch.h"

namespace {
std::array<uint8_t, 0x2000> rom;
std::array<uint8_t, 0x1000> stack;
uint32_t dispatched;
void check(bool ok) { if (!ok) std::abort(); }
}

extern "C" uint32_t bus_read_u32(uint32_t addr) {
    const uint8_t* bytes = nullptr;
    if (addr >= 0x08060000 && addr <= 0x08062000 - 4)
        bytes = rom.data() + (addr - 0x08060000);
    else if (addr >= 0x03007000 && addr <= 0x03008000 - 4)
        bytes = stack.data() + (addr - 0x03007000);
    check(bytes != nullptr);
    uint32_t value;
    std::memcpy(&value, bytes, 4);
    return value;
}
extern "C" void runtime_dispatch(uint32_t pc) { dispatched = pc; }

int main() {
    // Synthetic bytes only. Distinct complete templates let the test check
    // identity and bounds without requiring any copyrighted cartridge data.
    uint32_t seed = 0x19650218;
    for (auto& byte : rom) { seed = seed * 1664525u + 1013904223u; byte = seed >> 24; }
    struct Copy { uint32_t source; uint32_t size; };
    for (const Copy copy : {Copy{0x080615D4, 4}, Copy{0x0806169C, 0x24},
                           Copy{0x0806173C, 0x44}, Copy{0x08061AA4, 0x24}}) {
        for (const uint32_t addr : {0x03007000u, 0x030079F8u, 0x03007A28u,
                                   0x03007A38u, 0x03008000u - copy.size}) {
            stack.fill(0xAB);
            std::memcpy(stack.data() + addr - 0x03007000,
                        rom.data() + copy.source - 0x08060000, copy.size);
            // Entry and every potential IRQ return must retain the offset.
            for (uint32_t offset = 0; offset < copy.size; offset += 2) {
                dispatched = 0;
                check(mksc::flash_stack_dispatch(addr + offset, 1) == 1);
                check(dispatched == copy.source + offset);
            }
            check(mksc::flash_stack_dispatch(addr, 0) == 0);
            check(mksc::flash_stack_dispatch(addr + 1, 1) == 0);
            stack[addr - 0x03007000 + copy.size - 1] ^= 1;
            dispatched = 0;
            check(mksc::flash_stack_dispatch(addr, 1) == 0);
            check(dispatched == 0);
        }
    }
    check(mksc::flash_stack_dispatch(0x02000000, 1) == 0);
    check(mksc::flash_stack_dispatch(0x03008000, 1) == 0);
    std::puts("flash dispatch: all templates, offsets, reused slots and rejection checks passed");
}

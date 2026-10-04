#pragma once

#include "runtime_arm.h"

namespace mksc {

// The SDK copies these helpers into stack buffers at varying call depths.
// Match the entire live copy, including its literal pool, before dispatching
// the same instruction offset in the ROM's compiled body. All branches stay
// inside the copied span, PC-relative loads refer to copied literals, and no
// instruction exposes its code address as data. Stack and ROM are word aligned.
inline int flash_stack_dispatch(uint32_t pc, int thumb) {
    constexpr uint32_t stack_begin = 0x03007000u;
    constexpr uint32_t stack_end = 0x03008000u;
    if (!thumb || pc < stack_begin || pc >= stack_end || (pc & 1u)) return 0;

    struct Copy { uint32_t source; uint32_t size; };
    static constexpr Copy copies[] = {
        {0x0806173Cu, 0x44u}, // VerifyFlashSector_Core -> VerifyFlashSector
        {0x0806169Cu, 0x24u}, // ReadFlash_Core -> ReadFlash
        {0x08061AA4u, 0x24u}, // VerifyFlashCoreFF -> VerifyFlashErase
        {0x080615D4u, 0x04u}, // ReadFlash1 -> SetReadFlash1
    };
    for (const Copy& copy : copies) {
        uint32_t first = pc - (copy.size - 2u);
        if (first < stack_begin) first = stack_begin;
        first = (first + 3u) & ~3u;
        for (uint32_t start = first; start <= pc && start + copy.size <= stack_end;
             start += 4u) {
            uint32_t offset = 0;
            for (; offset < copy.size; offset += 4u) {
                if (bus_read_u32(start + offset) != bus_read_u32(copy.source + offset)) break;
            }
            if (offset != copy.size) continue;
            runtime_dispatch(copy.source + (pc - start));
            return 1;
        }
    }
    return 0;
}

} // namespace mksc

#include <cstdio>
#include <cstdlib>
#include "../src/mods/mksc_presentation_plugins.cpp"

namespace {
unsigned checks;
void check(bool value) { ++checks; if (!value) std::abort(); }
int previous(uint32_t pc, uint32_t, uint32_t* value) {
    if (pc != 0x1234 || !value) return 0;
    *value = 27;
    return 1;
}
RuntimeThumbAluImmediateOverride wrapped;
int outer(uint32_t pc, uint32_t value, uint32_t* out) {
    return wrapped ? wrapped(pc, value, out) : 0;
}
void verify(bool track, bool shake) {
    uint32_t value = 99;
    g_cpu.R[1] = 3;
    check(g_runtime_thumb_alu_imm_override(kAffineBufferSelectorImmediatePc, 1, &value) == int(track));
    if (track) check(value == 3);
    for (uint32_t pc : kGroundCameraSurfaceMasks) {
        value = 99;
        check(g_runtime_thumb_alu_imm_override(pc, 0x1E, &value) == int(shake));
        if (shake) check(value == 0);
        check(!g_runtime_thumb_alu_imm_override(pc, 0x1D, &value));
        check(!g_runtime_thumb_alu_imm_override(pc, 0x1E, nullptr));
    }
    value = 99;
    check(g_runtime_thumb_alu_imm_override(0x1234, 1, &value) && value == 27);
    check(!g_runtime_thumb_alu_imm_override(0x0804BA50, 0x1E, &value));
}
}

int main() {
    g_runtime_thumb_alu_imm_override = previous;
    activate_track_60fps(); verify(true, false);
    activate_reduced_camera_shake(); verify(true, true);
    reset_presentation_mods(); check(g_runtime_thumb_alu_imm_override == previous);
    activate_reduced_camera_shake(); verify(false, true);
    activate_track_60fps(); verify(true, true);
    // A later handler can wrap this dispatcher. Reset must preserve that
    // handler and forwarding path, and subsequent activation must compose.
    wrapped = g_runtime_thumb_alu_imm_override;
    g_runtime_thumb_alu_imm_override = outer;
    reset_presentation_mods(); verify(false, false);
    activate_reduced_camera_shake(); verify(false, true);
    check(g_runtime_thumb_alu_imm_override == outer);
    g_runtime_thumb_alu_imm_override = wrapped;
    reset_presentation_mods(); check(g_runtime_thumb_alu_imm_override == previous);
    g_runtime_thumb_alu_imm_override = nullptr;
    std::printf("presentation plugins: %u checks passed\n", checks);
}

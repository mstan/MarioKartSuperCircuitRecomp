#include "mod_runtime.h"
#include "runtime_arm.h"

namespace {

constexpr uint32_t kAffineBufferSelectorImmediatePc = 0x08030C14u;
constexpr uint32_t kGroundCameraSurfaceMasks[] = {
    0x0804BA4Eu, 0x0804BBD2u, 0x0804BE7Au, 0x0804BEC6u, 0x0804BF36u};

RuntimeThumbAluImmediateOverride previous_override = nullptr;
bool installed = false;
bool track_60fps = false;
bool reduce_camera_shake = false;

int override_presentation_immediate(uint32_t instruction_pc,
                                    uint32_t original_value,
                                    uint32_t* out_value) {
    if (track_60fps && instruction_pc == kAffineBufferSelectorImmediatePc &&
        original_value == 1u && out_value) {
        // The following guest SUBS becomes selector - selector, keeping the
        // HDMA-visible output buffer selected and freshly generated each frame.
        *out_value = g_cpu.R[1];
        return 1;
    }
    if (reduce_camera_shake && original_value == 0x1Eu && out_value) {
        for (uint32_t pc : kGroundCameraSurfaceMasks) {
            if (instruction_pc == pc) {
                // These five grounded branches belong to camera update
                // 0804B4BC. A zero surface mask skips its binary terrain bob.
                // The preceding airborne-height path and racer state remain
                // untouched, including collision, velocity and jump physics.
                *out_value = 0;
                return 1;
            }
        }
    }
    return previous_override
        ? previous_override(instruction_pc, original_value, out_value)
        : 0;
}

void reset_presentation_mods() {
    track_60fps = false;
    reduce_camera_shake = false;
    if (!installed) return;
    if (g_runtime_thumb_alu_imm_override ==
        override_presentation_immediate) {
        g_runtime_thumb_alu_imm_override = previous_override;
        previous_override = nullptr;
        installed = false;
    }
    // If another trusted handler wraps us, retain the forwarding link. The
    // inactive dispatcher must not erase that handler's chain on reset.
}

void install_presentation_override() {
    if (installed) return;
    previous_override = g_runtime_thumb_alu_imm_override;
    g_runtime_thumb_alu_imm_override = override_presentation_immediate;
    installed = true;
}

void activate_track_60fps() {
    track_60fps = true;
    install_presentation_override();
}

void activate_reduced_camera_shake() {
    reduce_camera_shake = true;
    install_presentation_override();
}

}  // namespace

GBA_MOD_CONSTRUCTOR(mksc_register_presentation_plugins) {
    (void)gba_mod_register_reset_callback(reset_presentation_mods);
    (void)gba_mod_register_activation_plugin(
        "mario-kart-super-circuit.track-60fps", activate_track_60fps);
    (void)gba_mod_register_activation_plugin(
        "mario-kart-super-circuit.reduced-camera-shake", activate_reduced_camera_shake);
}

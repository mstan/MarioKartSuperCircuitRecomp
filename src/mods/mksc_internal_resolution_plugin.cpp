#include "mod_runtime.h"
#include <cstring>

namespace {
constexpr const char* package = "mario-kart-super-circuit.enhancement.internal-resolution";
void reset_resolution() { (void)gba_mod_set_internal_resolution(0); }
void activate_resolution() {
    const char* value = gba_mod_option_value(package, "internal-resolution", "scale");
    if (value && std::strcmp(value, "2") == 0) (void)gba_mod_set_internal_resolution(2);
    else if (value && std::strcmp(value, "3") == 0) (void)gba_mod_set_internal_resolution(3);
    else if (value && std::strcmp(value, "4") == 0) (void)gba_mod_set_internal_resolution(4);
    else if (value && std::strcmp(value, "8") == 0) (void)gba_mod_set_internal_resolution(8);
    else if (value && std::strcmp(value, "16") == 0) (void)gba_mod_set_internal_resolution(16);
    else if (value && std::strcmp(value, "32") == 0) (void)gba_mod_set_internal_resolution(32);
    else if (value && std::strcmp(value, "64") == 0) (void)gba_mod_set_internal_resolution(64);
}
GBA_MOD_CONSTRUCTOR(register_resolution) {
    (void)gba_mod_register_reset_callback(reset_resolution);
    (void)gba_mod_register_activation_plugin("mario-kart-super-circuit.internal-resolution", activate_resolution);
}
}

#include "multiplayer_launch.h"
#include "../src/mksc_extended_view.h"
#include "../src/mksc_flash_dispatch.h"

gbarecomp::GbaNetplayLaunch gba_view_probe_game() {
    g_runtime_ram_dispatch_hook = &mksc::flash_stack_dispatch;
    gbarecomp::GbaNetplayLaunch game;
    game.program_id="mksc-usa-native-probe";
    game.view_policy={true,true,mksc::install_extended_view,nullptr,mksc::reset_extended_view};
    return game;
}

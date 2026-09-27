#include "multiplayer_launch.h"
#include "../src/mksc_extended_view.h"

gbarecomp::GbaNetplayLaunch gba_view_probe_game() {
    gbarecomp::GbaNetplayLaunch game;
    game.program_id="mksc-usa-native-probe";
    game.view_policy={true,true,mksc::install_extended_view,nullptr,mksc::reset_extended_view};
    return game;
}

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "runtime.h"
#include "mksc_extended_view.h"
#include "mksc_flash_dispatch.h"
#if defined(GBAGAME_NETPLAY)
#include "multiplayer_launch.h"
#include "gba_netplay_build_identity.h"
#endif

#if defined(GBAGAME_RECOMP_UI)
#include "game_launcher_boot.h"
#endif

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--help") == 0 ||
            std::strcmp(argv[i], "-h") == 0) {
            std::printf(
                "MarioKartSuperCircuitRecomp [--bios <path>] [--rom <path>] [game.toml]\n");
            return 0;
        }
    }
    g_runtime_ram_dispatch_hook = &mksc::flash_stack_dispatch;
    gbarecomp::RunOptions opts;
    std::vector<std::string> args(argv, argv + argc);
#if defined(GBAGAME_NETPLAY)
    opts.netplay=gbarecomp::make_gba_netplay_launch("mksc-usa",GBARECOMP_NETPLAY_BUILD_ID,
        "aadc8b9f3c947ff6f610b6c8d7fddaaa9bea98b4bd43576df84f26c457ae1f90");
    opts.netplay->view_policy = {true, true, mksc::install_extended_view,
        nullptr, mksc::reset_extended_view};
    try { gbarecomp::parse_gba_netplay_arguments(args,*opts.netplay); }
    catch (const std::exception& e) { std::fprintf(stderr,"netplay: %s\n",e.what()); return 1; }
#endif
    opts.builtin_game_name = "Mario Kart: Super Circuit (USA)";
    opts.builtin_rom_sha1 = "9d327c030c3e2d9007990518594f70c3340ac56f";
    opts.builtin_rom_crc32 = 0xED316E37u;
    opts.mod_game_id = "mario-kart-super-circuit-us";
    opts.mod_owns_adaptive_view = true;
    opts.max_resize_view_width = 480;
    opts.resize_driven_view = true;
    opts.extended_view_init = &mksc::install_extended_view;
    opts.launcher_expose_widescreen = false;
    opts.launcher_expose_adaptive_view = false;
    opts.launcher_expose_sharp_filter = true;
    opts.launcher_default_sharp_filter = true;
    opts.launcher_expose_affine_filter = true;
    opts.launcher_default_affine_filter = true;
    opts.launcher_region = "USA";
    // The launcher reads [rom].path and [bios].path from this file when its
    // per-user cache is empty, so the verified local ROM is preselected.
    opts.launcher_game_config = "game.toml";
    opts.launcher_save_path = "saves/mario_kart_super_circuit_usa.sav";

#if defined(GBAGAME_RECOMP_UI)
    if (game_launcher_preboot(args, opts)) return 0;
#endif
    std::vector<char*> av;
    av.reserve(args.size());
    for (auto& arg : args) av.push_back(arg.data());
    return gbarecomp::run_game(static_cast<int>(av.size()), av.data(), opts);
}

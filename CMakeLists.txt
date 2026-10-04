cmake_minimum_required(VERSION 3.20)
project(MarioKartSuperCircuitRecomp VERSION 0.1.2 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(ANDROID)
    set(_netplay_default OFF)
else()
    set(_netplay_default ON)
endif()
option(GBARECOMP_NETPLAY "Build desktop link-cable netplay" ${_netplay_default})

set(GBARECOMP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/gbarecomp"
    CACHE PATH "Path to this game's gbarecomp worktree")
set(RECOMP_UI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/recomp-ui"
    CACHE PATH "Path to the shared recomp-ui checkout")
set(GBARECOMP_RUNTIME_UI_ROOT "${RECOMP_UI_ROOT}"
    CACHE PATH "recomp-ui root used by the live GBA runtime adapter")
set(GBARECOMP_SELFHEAL_RECOMPILE_DEFAULT ON CACHE BOOL
    "Enable Mario Kart: Super Circuit runtime self-heal sharding by default" FORCE)
set(GBARECOMP_ENABLE_MODS ON CACHE BOOL
    "Enable Mario Kart: Super Circuit's mod catalog and GBA mod packages" FORCE)
if(NOT EXISTS "${GBARECOMP_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR
        "GBARECOMP_ROOT is not a gbarecomp checkout:\n"
        "  ${GBARECOMP_ROOT}\n"
        "Configure with -DGBARECOMP_ROOT=<path> if the local worktree link is absent.")
endif()

add_subdirectory("${GBARECOMP_ROOT}" gbarecomp_build EXCLUDE_FROM_ALL)
enable_testing()
add_executable(mksc_extended_view_tests
    tests/extended_view_tests.cpp
    "${GBARECOMP_ROOT}/tools/bios_smoke/dispatch_stub.cpp"
    src/mksc_extended_view.cpp)
target_include_directories(mksc_extended_view_tests PRIVATE
    src "${GBARECOMP_ROOT}/src/runtime")
target_link_gbarecomp_runtime_stack(mksc_extended_view_tests)
add_test(NAME mksc_extended_view COMMAND mksc_extended_view_tests)

set(GBA_GAME_GENERATED_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/generated" CACHE PATH
    "Generated native corpus (may point at an independent qualification build)")
if(EXISTS "${GBA_GAME_GENERATED_ROOT}/recompiled.cpp")
    message(FATAL_ERROR
        "Stale monolithic generated/recompiled.cpp found. "
        "Run tools/regen.ps1 with the current gbarecomp before building.")
endif()
file(GLOB GAME_GENERATED_SOURCES CONFIGURE_DEPENDS
    "${GBA_GAME_GENERATED_ROOT}/*.c"
    "${GBA_GAME_GENERATED_ROOT}/*.cpp")

add_executable(MarioKartSuperCircuitRecomp
    src/main.cpp
    src/mksc_extended_view.cpp
    src/mods/mksc_adaptive_view_plugin.cpp
    src/mods/mksc_track_60fps_plugin.cpp
    ${GAME_GENERATED_SOURCES})

# Keep every generated guest shard cheaper to compile than the runtime. The
# dispatch table and symbol map remain ordinary small generated units.
set(GAME_RECOMPILED_SOURCES ${GAME_GENERATED_SOURCES})
list(FILTER GAME_RECOMPILED_SOURCES INCLUDE REGEX
    "/recompiled_[0-9]+\\.cpp$")
if(GAME_RECOMPILED_SOURCES)
    set_source_files_properties(${GAME_RECOMPILED_SOURCES} PROPERTIES
        COMPILE_OPTIONS "-O1;-g0")
endif()

target_include_directories(MarioKartSuperCircuitRecomp PRIVATE
    "${GBARECOMP_ROOT}/src/armv4t"
    "${GBARECOMP_ROOT}/src/gba"
    "${GBARECOMP_ROOT}/src/runtime"
    "${GBARECOMP_ROOT}/src/debug")

target_link_libraries(MarioKartSuperCircuitRecomp PRIVATE
    "-Wl,--start-group"
    gbarecomp_runtime
    gbarecomp_debug
    gbarecomp_gba
    gbarecomp_armv4t
    "-Wl,--end-group")

target_compile_definitions(MarioKartSuperCircuitRecomp PRIVATE
    GBARECOMP_DEFAULT_DEBUG_PORT=19872
    GBARECOMP_WINDOW_TITLE="MarioKartSuperCircuitRecomp"
    GBARECOMP_DEFAULT_GAME_CONFIG="game.toml")
if(GBARECOMP_NETPLAY)
    target_link_libraries(MarioKartSuperCircuitRecomp PRIVATE gbarecomp_netplay)
    gbarecomp_target_netplay_identity(MarioKartSuperCircuitRecomp "${CMAKE_CURRENT_SOURCE_DIR}" "${GBA_GAME_GENERATED_ROOT}")
    set_property(SOURCE src/main.cpp APPEND PROPERTY COMPILE_DEFINITIONS GBAGAME_NETPLAY=1)
endif()

# Keep recomp-ui and its Dear ImGui sources isolated from the generated guest
# shards. Only this small bridge and main.cpp know that the launcher exists.
option(GBAGAME_RECOMP_UI "Build the recomp-ui pre-boot launcher" ON)
if(GBAGAME_RECOMP_UI)
    include(${RECOMP_UI_ROOT}/recomp_ui.cmake)
    add_library(game_launcher_ui STATIC src/game_launcher_boot.cpp)
    target_include_directories(game_launcher_ui PRIVATE
        "${GBARECOMP_ROOT}/src/runtime"
        "${GBARECOMP_ROOT}/src/gba"
        src)
    target_compile_definitions(game_launcher_ui PRIVATE
        GBARECOMP_ENABLE_MODS=1)
    recomp_target_launcher_ui(game_launcher_ui
        CONSOLE gba
        BOXART ${CMAKE_CURRENT_SOURCE_DIR}/recomp/launcher/boxart.tga)
    if(GBARECOMP_NETPLAY)
        gbarecomp_target_netplay_launcher(game_launcher_ui)
    endif()
    find_package(SDL2 REQUIRED)
    target_link_libraries(game_launcher_ui PUBLIC SDL2::SDL2)
    if(WIN32)
        target_link_libraries(game_launcher_ui PUBLIC opengl32)
    endif()
    target_link_libraries(MarioKartSuperCircuitRecomp PRIVATE game_launcher_ui)
    set_property(SOURCE src/main.cpp APPEND PROPERTY
        COMPILE_DEFINITIONS GBAGAME_RECOMP_UI)
endif()

if(GBARECOMP_NETPLAY AND NOT ANDROID)
    gbarecomp_target_netplay_view_probe(MarioKartSuperCircuitRecomp tests/netplay_view_probe.cpp)
endif()

# Ship the game-owned, disabled-by-default enhancement packages beside the
# executable so recomp-ui can expose them in the Mods tab.
set(MKSC_PRELOADED_MODS "${CMAKE_CURRENT_SOURCE_DIR}/mods/preloaded")
if(EXISTS "${MKSC_PRELOADED_MODS}/packages")
    add_custom_command(TARGET MarioKartSuperCircuitRecomp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${MKSC_PRELOADED_MODS}"
            "$<TARGET_FILE_DIR:MarioKartSuperCircuitRecomp>/mods"
        COMMENT "Preloading the Mario Kart: Super Circuit mod catalog")
endif()

if(MSVC)
    target_link_options(MarioKartSuperCircuitRecomp PRIVATE /STACK:16777216)
elseif(MINGW OR WIN32)
    target_link_options(MarioKartSuperCircuitRecomp PRIVATE -Wl,--stack,16777216)
endif()

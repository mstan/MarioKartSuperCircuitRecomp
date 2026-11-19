# Building on macOS

Tested on Apple Silicon (arm64) with the Xcode command-line tools, CMake, Ninja
and SDL2 from Homebrew:

```sh
brew install cmake ninja sdl2
git clone --recurse-submodules https://github.com/mstan/MarioKartSuperCircuitRecomp.git
cd MarioKartSuperCircuitRecomp
mkdir -p roms
cp /path/to/your/mksc.gba roms/mario_kart_super_circuit_usa.gba
cp /path/to/your/gba_bios.bin gbarecomp/bios/gba_bios.bin

cmake -S gbarecomp -B gbarecomp/build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build gbarecomp/build --target gba_recompile
tools/regen.sh            # recompiles the BIOS and the ROM (checks the ROM SHA-1)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target MarioKartSuperCircuitRecomp
tools/macos/make_app.sh   # optional: release-macos/MarioKartSuperCircuitRecomp.app
```

You can also run `build/MarioKartSuperCircuitRecomp` directly from the `build`
directory. The recomp-ui launcher asks for the ROM and BIOS on first run and
offers the Mods tab; the .app keeps saves and settings in
`~/Library/Application Support/MarioKartSuperCircuitRecomp`. Set
`ICON=/path/to/AppIcon.icns` when running `make_app.sh` to bundle your own icon
(none is shipped). The ROM-derived `generated/` sources and the bundle are local
build products and are never to be redistributed.

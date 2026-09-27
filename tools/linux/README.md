# Pokémon Emerald - GBA static recompilation (Linux x86_64 AppImage) v@VERSION@

An optimized native port: the game's ARM7TDMI code is statically recompiled to
native code with the [gbarecomp](https://github.com/mstan/gbarecomp) framework;
the real GBA BIOS is recompiled and executed.

## You supply the ROM and BIOS

The game ROM and GBA BIOS are not included. On first run the launcher asks for:

- your legally-obtained **Pokémon Emerald (USA)** ROM (`.gba`), SHA-1
  `9d327c030c3e2d9007990518594f70c3340ac56f`
- a **GBA BIOS** dump (`gba_bios.bin`, 16 KiB).

The file picker uses zenity or kdialog (install one if the picker does not
appear), or pass them once on the command line:

    ./MarioKartSuperCircuitRecomp-linux-x86_64-v@VERSION@.AppImage --rom /path/mario_kart_super_circuit.gba --bios /path/gba_bios.bin

## Running

    chmod +x MarioKartSuperCircuitRecomp-linux-x86_64-v@VERSION@.AppImage
    ./MarioKartSuperCircuitRecomp-linux-x86_64-v@VERSION@.AppImage

Requires FUSE 2 (`libfuse2`) like any AppImage; without it, run with
`--appimage-extract-and-run`. Your settings, saves and mod choices live in
`~/.local/share/MarioKartSuperCircuitRecomp` (override with `MARIOKARTSUPERCIRCUITRECOMP_HOME`).

## Enhancement mods

Open **Mods** to enable the optional Adaptive Widescreen and 60 FPS track
renderer enhancements for single-player play. Netplay has its own view setting.

# Mario Kart: Super Circuit Recomp

> **Experimental preview.** This recompilation is a byproduct of developing
> [gbarecomp](https://github.com/mstan/gbarecomp): the games are the proving
> ground, while the reusable framework is the larger goal. This is not a
> finished commercial port, so expect rough edges and please report problems.
> For more context, read
> [Recomp + AI: 5 Months Later »](https://1379.tech/recomp-ai-5-months-later/).

Static recompilation of **Mario Kart: Super Circuit** for Windows and Linux, with
optional 60 FPS and Adaptive Widescreen mods.

Special thanks to **antimattur** for the Mario Kart: Super Circuit
decompilation work and for discovering the 60 FPS patch; that foundation made
this level of polish possible.

The game ROM and Nintendo GBA BIOS are **not included**. You must provide your
own legally obtained dumps.

## Status

The current release is `v0.1.2` for Windows x64 and Linux x86_64. The game
boots and runs through menus, cup selection, races, and results, and two
players can link over the Internet or LAN with emulated link-cable netplay
(see [`docs/NETPLAY.md`](docs/NETPLAY.md)). `v0.1.2` fixes netplay sessions
closing on both players shortly after connecting.

This is still an experimental preview; back up important saves and please
report repeatable gameplay or presentation edge cases.

## Quick start

1. Download the Windows zip or Linux AppImage from [Releases](../../releases).
2. Extract the zip and run `MarioKartSuperCircuitRecomp.exe`, or mark the
   AppImage executable and run it.
3. In the launcher, select your **Mario Kart: Super Circuit (USA)** ROM and
   retail GBA BIOS.
4. Configure display, audio, controls, and mods, then select **Play**.

The launcher remembers valid files after the first setup. Enable **Skip
launcher on boot** if you want later launches to go directly into the game.

## Included mods

Both enhancements are optional and disabled by default:

- **60 FPS Track Rendering** updates race presentation at 60 FPS while
  preserving the game's underlying logic and timing, based on the patch
  discovered by **antimattur**.
- **Adaptive Widescreen** renders additional race content at the sides instead
  of stretching the original 240×160 image.

Open the launcher's **Mods** page to enable either feature. Because both are
experimental, please report repeatable visual or gameplay regressions with the
course, mode, and character used.

## Features

- Native Windows x64 application and Linux x86_64 AppImage
- ROM and BIOS setup through the shared
  [recomp-ui](https://github.com/mstan/recomp-ui) launcher
- Optional 60 FPS and Adaptive Widescreen mods
- Two-player link-cable netplay with rollback or input delay
- Keyboard and modern game-controller support
- Windowed and fullscreen play with sharp scaling and optional affine
  filtering
- In-game settings menu
- Cartridge saves and save states

## Controls

| GBA control | Keyboard |
|---|---|
| D-Pad | Arrow keys |
| A / B | X / Z |
| Start | Enter |
| Select | Right Shift |
| L / R | C / V |

Use **Shift+F1-F9** to save a state and **F1-F9** to load one. Controls can be
changed from the launcher.

## Building from source

Windows development requires CMake, Ninja, MSYS2 MinGW64, and SDL2:

```powershell
git clone --recurse-submodules `
  https://github.com/mstan/MarioKartSuperCircuitRecomp.git
cd MarioKartSuperCircuitRecomp

cmake -S gbarecomp -B gbarecomp/build -G Ninja
cmake --build gbarecomp/build --target gba_recompile
pwsh tools/regen.ps1
cmake -S . -B build -G Ninja
cmake --build build --target MarioKartSuperCircuitRecomp
```

Generation requires the supported ROM revision and a retail GBA BIOS. Their
identities and local development paths are documented in
[`baserom.md`](baserom.md) and [`game.toml`](game.toml). ROM-derived generated
code, copyrighted inputs, saves, and build output remain local and are never
included in releases.

Contributors can run `pwsh tools/test-attract-gameplay.ps1` for the automated
native acceptance routes and `pwsh tools/make_release.ps1 -Version 0.1.2` to
build a sanitized Windows package. The Linux AppImage build is documented in
[`tools/linux/BUILD.md`](tools/linux/BUILD.md).

### Decomp symbol names (optional)

The generated code and every debug surface — hang traces, dispatch-miss
reports, the PC sampler, the TCP `symbol` query — can be named from the
[jellees/mksc](https://github.com/jellees/mksc) decompilation, which is pinned
as a submodule at `third_party/mksc`. The tracked files under
[`symbols/`](symbols/README.md) are already imported, so `tools/regen.ps1`
picks them up automatically; `-NoSymbols` regenerates without them.

Credit to **antimattur** for the Mario Kart: Super Circuit decompilation work
that made these symbol imports possible.

To re-import after the decomp advances upstream, build it under WSL with
`tools/decomp/provision.sh` and `tools/decomp/build.sh` (root-free: no
devkitPro or apt install needed) and run
`gbarecomp/tools/symbol_import/import_decomp_symbols.py`. The build gates on
the decomp reproducing this project's exact ROM SHA-1. Full procedure:
[`gbarecomp/docs/SYMBOL_OVERLAY.md`](gbarecomp/docs/SYMBOL_OVERLAY.md).

The decomp is used only as a source of facts — names, addresses, sizes, and
the code/data split its own linker produced. None of its source, comments, or
data enters this repository or the build; the submodule pins a URL and commit
rather than vendoring code. Upstream ships no license file, which is why the
boundary is drawn that tightly.

## License

PolyForm Noncommercial 1.0.0 — see [`LICENSE`](LICENSE). Third-party
components retain their own licenses.

## Legal

This is an unofficial, non-commercial preservation and research project. It
is not affiliated with or endorsed by Nintendo. Mario Kart and related names,
characters, artwork, and game data are trademarks or copyrights of their
respective owners.

No copyrighted game ROM or Nintendo BIOS data is distributed by this project.

---

Part of the **R.A.I.D. — Retro AI Development** static-recompilation
community.

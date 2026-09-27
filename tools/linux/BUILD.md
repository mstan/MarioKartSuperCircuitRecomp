# Building the Linux AppImage

Use the game repository's pinned gbarecomp and recomp-ui checkouts, including
their submodules. Recompile the retail BIOS and regenerate the verified USA
game's native sources with `tools/regen.ps1` first. No ROM or BIOS is published.

Run from Linux or WSL with Docker and rsync installed:

```sh
bash tools/linux/make_appimage.sh --version 0.1.0 \
  --game "$PWD" --engine /path/to/pinned/gbarecomp \
  --ui /path/to/pinned/recomp-ui --out "$PWD/release-stage" --jobs 8 \
  --private /path/to/private-test-images
```

The optional private test directory contains `gba_bios.bin` and
`mario_kart_super_circuit_usa.gba`. They are mounted read-only for the packaged
1500-frame smoke test and never enter the AppImage. A release should always run
that test. The Ubuntu 22.04 builder bundles the pinned SDL fork and launcher
dependencies, with a persistent build cache in
`~/.cache/mariokartsupercircuitrecomp-release`.

Windows packaging uses `tools/make_release.ps1 -Version 0.1.0` with optional
`-GbarecompRoot` and `-RecompUiRoot` paths. Use the same generated native sources
for both platforms and compare their generated
`MarioKartSuperCircuitRecomp-netplay/gba_netplay_build_identity.h` files before
publishing. Qualify the actual packaged executables together in both rollback
and input-delay modes.

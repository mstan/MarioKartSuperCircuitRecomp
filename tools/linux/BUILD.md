# Building the Linux AppImage

Use the game repository's pinned gbarecomp and recomp-ui checkouts, including
their submodules. Recompile the retail BIOS and regenerate the verified USA
game's native sources with `tools/regen.ps1` first. No ROM or BIOS is published.

Run from Linux or WSL with Docker and rsync installed:

```sh
bash tools/linux/make_appimage.sh --version 0.1.14 \
  --game "$PWD" --engine /path/to/pinned/gbarecomp \
  --ui /path/to/pinned/recomp-ui --out "$PWD/release-stage" --jobs 2 \
  --private /path/to/private-test-images
```

The optional private test directory contains `gba_bios.bin` and
`mario_kart_super_circuit_usa.gba`. They are mounted read-only for the packaged
6000-frame smoke test and never enter the AppImage. A release should always run
that test. The Ubuntu 22.04 builder bundles the pinned SDL fork and launcher
dependencies, with a persistent build cache in
`~/.cache/mariokartsupercircuitrecomp-release`. The container is limited to the
requested CPU count and 4 GiB of memory.

When the source directories are staged without Git metadata (for example,
Windows worktrees passed through WSL), supply `--title-source`,
`--engine-source` and `--ui-source` as exact public GitHub `/tree/<40-digit-SHA>`
URLs, plus `--engine-rev`. The notice collector uses those frozen identities
without writing into the read-only source mounts.

Windows packaging uses `tools/make_release.ps1 -Version 0.1.14` with optional
`-GbarecompRoot` and `-RecompUiRoot` paths. Use the same generated native sources
for both platforms and compare their generated
`MarioKartSuperCircuitRecomp-netplay/gba_netplay_build_identity.h` files before
publishing. Qualify the actual packaged executables together in both rollback
and input-delay modes.

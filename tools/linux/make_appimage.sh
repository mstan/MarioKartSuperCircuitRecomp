#!/usr/bin/env bash
# Host side of the Linux release: build the builder image and run
# build_appimage.sh in it. Runs on Linux or WSL (Docker required).
#
#   make_appimage.sh --version X.Y.Z --game DIR --engine DIR --ui DIR --out DIR \
#                    [--private DIR] [--jobs N] [--engine-rev REV]
#
# --private (optional) holds gba_bios.bin + mario_kart_super_circuit_usa.gba for the smoke
# test; it is mounted read-only and never enters the image.
set -euo pipefail
VERSION="" GAME="" ENGINE="" UI="" OUT="" PRIVATE="" JOBS=4 ENGINE_REV="" GENERATED_BIOS=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --version) VERSION="$2"; shift 2 ;;
        --game) GAME="$2"; shift 2 ;;
        --engine) ENGINE="$2"; shift 2 ;;
        --ui) UI="$2"; shift 2 ;;
        --out) OUT="$2"; shift 2 ;;
        --private) PRIVATE="$2"; shift 2 ;;
        --jobs) JOBS="$2"; shift 2 ;;
        --engine-rev) ENGINE_REV="$2"; shift 2 ;;
        --generated-bios) GENERATED_BIOS="$2"; shift 2 ;;
        *) echo "unknown argument $1"; exit 2 ;;
    esac
done
for v in VERSION GAME ENGINE UI OUT; do
    [[ -n "${!v}" ]] || { echo "--${v,,} is required"; exit 2; }
done
[[ -n "$GENERATED_BIOS" ]] || GENERATED_BIOS="$ENGINE/src/runtime/generated_bios"
for name in bios_recompiled.cpp bios_recompiled.h bios_dispatch_table.cpp bios_symbol_map.cpp; do
    test -f "$GENERATED_BIOS/$name" || { echo "Missing audited generated BIOS input: $GENERATED_BIOS/$name"; exit 1; }
done
IMAGE=mariokartsupercircuitrecomp-linux-builder
docker build -q -t "$IMAGE" "$GAME/tools/linux" >/dev/null
mkdir -p "$OUT"
# Build tree on the Linux filesystem (fast, persistent between releases).
CACHE="${MARIOKARTSUPERCIRCUITRECOMP_BUILD_CACHE:-$HOME/.cache/mariokartsupercircuitrecomp-release}"
mkdir -p "$CACHE"
# Stage the sources on the Linux filesystem first: bind-mounting multi-GB
# Windows (/mnt/<drive>) trees into Docker Desktop routes every read through
# its file-sharing layer (slow, and it balloons com.docker.backend's memory).
# ROMs, BIOS dumps and build outputs are never staged.
STAGE="$CACHE/src"
mkdir -p "$STAGE"
EXCLUDES=(--exclude '.git' --exclude 'build/' --exclude 'build-*/' --exclude 'release-stage/'
          --exclude 'recomp_cache/' --exclude '*.gba' --exclude '*.sav' --exclude '*.state*'
          --exclude 'gba_bios.bin' --exclude 'android/app/build/' --exclude 'android/build/'
          --exclude 'android/.gradle/' --exclude 'android/app/.cxx/')
rsync -a --copy-dirlinks --delete "${EXCLUDES[@]}" --exclude '/gbarecomp/' --exclude '/recomp-ui/' --exclude 'third_party/mksc/' --exclude 'docs/screenshots/' \
    "$GAME/" "$STAGE/game/"
rsync -a --copy-dirlinks --delete "${EXCLUDES[@]}" --exclude 'tools/gbaref/' --exclude 'tools/_toolchain_cache/' \
    --exclude '.local/' --exclude 'oracle/' "$ENGINE/" "$STAGE/engine/"
rsync -a --delete "${EXCLUDES[@]}" "$UI/" "$STAGE/ui/"
rsync -a --delete "$GENERATED_BIOS/" "$STAGE/bios/"
MOUNTS=(-v "$STAGE/bios:/src/bios:ro" -v "$STAGE/game:/src/game:ro" -v "$STAGE/engine:/src/engine:ro" -v "$STAGE/ui:/src/ui:ro"
        -v "$OUT:/out")
if [[ -n "$PRIVATE" ]]; then
    # Smoke-test ROM/BIOS: a private copy on the Linux side, removed on exit.
    PRIV_STAGE="$(mktemp -d "$CACHE/private.XXXXXX")"
    trap 'rm -rf "$PRIV_STAGE"' EXIT
    cp "$PRIVATE/gba_bios.bin" "$PRIVATE/mario_kart_super_circuit_usa.gba" "$PRIV_STAGE/"
    MOUNTS+=(-v "$PRIV_STAGE:/private:ro")
fi
# Build as the invoking user so /out stays owned by them.
[[ -n "$ENGINE_REV" ]] || ENGINE_REV="$(git -C "$ENGINE" rev-parse --short=12 HEAD)"
docker run --rm --user "$(id -u):$(id -g)" -e HOME=/tmp -e VERSION="$VERSION" -e JOBS="$JOBS" \
    -e ENGINE_REV="$ENGINE_REV" \
    -v "$CACHE:/build" "${MOUNTS[@]}" "$IMAGE" \
    nice -n 10 bash /src/game/tools/linux/build_appimage.sh

#!/usr/bin/env bash
# Build MarioKartSuperCircuitRecomp-linux-x86_64-v<VERSION>.AppImage inside the builder image
# (tools/linux/Dockerfile). Invoked by tools/make_release.ps1; mounts:
#
#   /src/game    this repo (generated C already regenerated for the release)
#   /src/engine  the pinned gbarecomp checkout
#   /src/ui      the pinned recomp-ui checkout
#   /out         release-stage output directory
#   /private     OPTIONAL, read-only: gba_bios.bin + mario_kart_super_circuit_usa.gba, used only
#                for the post-build smoke test; never copied into the image.
#
# Environment: VERSION (required), JOBS (default 8).
set -euo pipefail
: "${VERSION:?VERSION is required}"
JOBS="${JOBS:-8}"
GAME=/src/game ENGINE=/src/engine UI=/src/ui OUT=/out
NAME="MarioKartSuperCircuitRecomp-linux-x86_64-v${VERSION}"
export CC=gcc-12 CXX=g++-12

echo "== SDL2 (engine-pinned fork)"
SDL_SRC="$ENGINE/platform/android/third_party/SDL"
test -f "$SDL_SRC/CMakeLists.txt" || { echo "SDL submodule missing: $SDL_SRC"; exit 1; }
cmake -S "$SDL_SRC" -B /build/sdl -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/build/sdl-install -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST=OFF \
    -DSDL_TESTS=OFF > /build/sdl.log 2>&1 || { tail -40 /build/sdl.log; exit 1; }
ninja -C /build/sdl -j "$JOBS" install > /build/sdl-build.log 2>&1 || { tail -40 /build/sdl-build.log; exit 1; }

echo "== MarioKartSuperCircuitRecomp"
cmake -S "$GAME" -B /build/mksc -G Ninja -DCMAKE_BUILD_TYPE=Release \
    "-DCMAKE_CXX_FLAGS_RELEASE=-O3 -DNDEBUG" -DGBARECOMP_NETPLAY=ON -DCMAKE_PREFIX_PATH=/build/sdl-install \
    -DGBARECOMP_ROOT="$ENGINE" -DRECOMP_UI_ROOT="$UI" -DGBARECOMP_RUNTIME_UI_ROOT="$UI" \
    -DGBARECOMP_BUILD_ORACLE=OFF > /build/mksc-configure.log 2>&1 \
    || { tail -40 /build/mksc-configure.log; exit 1; }
ninja -C /build/mksc -j "$JOBS" MarioKartSuperCircuitRecomp > /build/mksc-build.log 2>&1 \
    || { grep -E "error|FAILED" /build/mksc-build.log | head -40; exit 1; }

echo "== AppDir"
APPDIR=/build/AppDir
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/lib" "$APPDIR/usr/share/mariokartsupercircuitrecomp/mods" \
         "$APPDIR/usr/share/applications" "$APPDIR/usr/share/icons/hicolor/256x256/apps"
install -m 755 /build/mksc/MarioKartSuperCircuitRecomp "$APPDIR/usr/bin/MarioKartSuperCircuitRecomp"
strip "$APPDIR/usr/bin/MarioKartSuperCircuitRecomp"
test -d /build/mksc/assets/img || { echo "recomp-ui launcher assets missing"; exit 1; }
cp -R /build/mksc/assets "$APPDIR/usr/bin/assets"
# Checked-in catalog only (never a build dir's remembered selections).
cp -R "$GAME/mods/preloaded/packages" "$APPDIR/usr/share/mariokartsupercircuitrecomp/mods/packages"
cp "$GAME/LICENSE" "$APPDIR/usr/share/mariokartsupercircuitrecomp/LICENSE"
cp "$GAME/tools/linux/README.md" "$APPDIR/usr/share/mariokartsupercircuitrecomp/README.md"
cat "$GAME/docs/NETPLAY.md" >> "$APPDIR/usr/share/mariokartsupercircuitrecomp/README.md"
sed -i "s/@VERSION@/${VERSION}/g" "$APPDIR/usr/share/mariokartsupercircuitrecomp/README.md"
install -m 755 "$GAME/tools/linux/AppRun" "$APPDIR/AppRun"
cp "$GAME/tools/linux/mariokartsupercircuitrecomp.desktop" "$APPDIR/mariokartsupercircuitrecomp.desktop"
cp "$GAME/tools/linux/mariokartsupercircuitrecomp.desktop" "$APPDIR/usr/share/applications/"
cp "$GAME/tools/linux/mariokartsupercircuitrecomp.svg" "$APPDIR/mariokartsupercircuitrecomp.svg"
cp "$APPDIR/mariokartsupercircuitrecomp.svg" "$APPDIR/usr/share/icons/hicolor/256x256/apps/"
ln -sf mariokartsupercircuitrecomp.svg "$APPDIR/.DirIcon"

# Bundle every resolved library except those the host must provide: glibc,
# the C++ runtime (host copies are newer and GPU drivers link them), and the
# graphics / windowing / audio / system-service stacks.
# Families by prefix (libX11, libXau, libxcb-*, libwayland-*, ...) and single
# libraries by exact name (libz must not swallow libzstd, etc.).
HOST_FAMILIES='^(linux-vdso|ld-linux|libGL|libEGL|libOpenGL|libGLES|libX|libxcb|libwayland|libxkbcommon|libdecor|libasound|libpulse|libpipewire|libspa|libdbus|libudev|libsystemd|libglib|libgobject|libgio|libgmodule|libpcre|libgcrypt|libgpg-error|libdrm|libgbm)'
HOST_EXACT='^(libc|libm|libdl|librt|libpthread|libresolv|libutil|libgcc_s|libstdc\+\+|libz|libexpat|libffi|libuuid|libcap|liblz4|liblzma|libzstd|libbsd|libmd|libapparmor|libasyncns|libsndfile|libFLAC|libvorbis|libvorbisenc|libopus|libogg|libmpg123|libmp3lame|libbz2)\.so'
LD_LIBRARY_PATH=/build/sdl-install/lib ldd "$APPDIR/usr/bin/MarioKartSuperCircuitRecomp" | awk '/=> \//{print $1, $3}' |
while read -r soname path; do
    if [[ "$soname" =~ $HOST_FAMILIES || "$soname" =~ $HOST_EXACT ]]; then continue; fi
    cp -L "$path" "$APPDIR/usr/lib/$soname"
    echo "bundled $soname"
done
patchelf --set-rpath '$ORIGIN/../lib' "$APPDIR/usr/bin/MarioKartSuperCircuitRecomp"
for so in "$APPDIR"/usr/lib/*.so*; do patchelf --set-rpath '$ORIGIN' "$so"; done
test -f "$APPDIR/usr/lib/libSDL2-2.0.so.0" || { echo "SDL2 was not bundled"; exit 1; }

echo "== BYOR guard (no ROM / BIOS in the image)"
python3 - "$APPDIR" <<'PY'
import os, sys
root, bad = sys.argv[1], []
for d, _, files in os.walk(root):
    for f in files:
        p = os.path.join(d, f)
        if os.path.islink(p):
            continue
        low = f.lower()
        size = os.path.getsize(p)
        if low.endswith((".gba", ".agb", ".sav")) or low == "gba_bios.bin" or \
           (size == 16384 and low.endswith(".bin")):
            bad.append(p)
            continue
        if size >= 0xC0:
            with open(p, "rb") as fh:
                head = fh.read(0xC0)
            if head[0xA0:0xB0] == b"MARIO KARTSCAMKE":   # GBA cartridge header
                bad.append(p)
if bad:
    sys.exit("private assets in the AppDir: " + ", ".join(bad))
print("clean")
PY

echo "== appimagetool"
mkdir -p "$OUT"
ARCH=x86_64 /opt/appimage/appimagetool/AppRun --no-appstream \
    --runtime-file /opt/appimage/runtime-x86_64 "$APPDIR" "$OUT/$NAME.AppImage" \
    > /build/appimagetool.log 2>&1 || { tail -30 /build/appimagetool.log; exit 1; }
chmod +x "$OUT/$NAME.AppImage"

echo "== smoke test (the packaged AppImage, headless)"
if [[ -f /private/gba_bios.bin && -f /private/mario_kart_super_circuit_usa.gba ]]; then
    SMOKE=$(mktemp -d)
    trap 'rm -rf "$SMOKE"' EXIT
    ( cd "$SMOKE" && MARIOKARTSUPERCIRCUITRECOMP_HOME="$SMOKE/home" APPIMAGE_EXTRACT_AND_RUN=1 \
        timeout 600 "$OUT/$NAME.AppImage" --no-launcher --no-window --frames 1500 \
        --bios /private/gba_bios.bin --rom /private/mario_kart_super_circuit_usa.gba \
        --save-path "$SMOKE/home/smoke.sav" > "$SMOKE/run.log" 2>&1 ) || { tail -40 "$SMOKE/run.log"; exit 1; }
    grep -E "cpu_backend|self_heal_coverage" "$SMOKE/run.log" || true
    grep -q "self_heal_coverage=FULLY_STATIC" "$SMOKE/run.log" \
        || { tail -40 "$SMOKE/run.log"; echo "smoke test FAILED"; exit 1; }
    test -L "$SMOKE/home/MarioKartSuperCircuitRecomp" && test -d "$SMOKE/home/mods/packages" \
        || { echo "per-user data dir not set up"; exit 1; }
    echo "smoke test passed"
else
    echo "SKIPPED (no /private assets mounted)"
fi
ls -la "$OUT/$NAME.AppImage"

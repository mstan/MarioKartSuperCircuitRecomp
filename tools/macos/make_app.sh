#!/usr/bin/env bash
# Wraps a built MarioKartSuperCircuitRecomp into a double-clickable macOS .app.
#   usage: tools/macos/make_app.sh [build-dir] [out-dir]   (defaults: build, release-macos)
# No ROM, BIOS, generated code or save data is packaged: the recomp-ui launcher asks
# for them on first run. Per-user files live in
#   ~/Library/Application Support/MarioKartSuperCircuitRecomp
# Optional: ICON=path/to/AppIcon.icns bundles an icon you supply.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD="${1:-$ROOT/build}"; OUT="${2:-$ROOT/release-macos}"
APP="$OUT/MarioKartSuperCircuitRecomp.app"; MACOS="$APP/Contents/MacOS"
[ -x "$BUILD/MarioKartSuperCircuitRecomp" ] || { echo "Build the game first: $BUILD" >&2; exit 1; }
VERSION="$(sed -n 's/^project(MarioKartSuperCircuitRecomp VERSION \([0-9.]*\).*/\1/p' "$ROOT/CMakeLists.txt")"
rm -rf "$APP"; mkdir -p "$MACOS" "$APP/Contents/Resources"
cp "$BUILD/MarioKartSuperCircuitRecomp" "$MACOS/MarioKartSuperCircuitRecomp-bin"
cp -R "$BUILD/mods" "$BUILD/assets" "$MACOS/"
cp "$ROOT/game.toml" "$MACOS/"
cat > "$MACOS/MarioKartSuperCircuitRecomp" <<'EOS'
#!/bin/bash
# Run from Application Support so saves, mods and launcher settings persist.
D="$(cd "$(dirname "$0")" && pwd)"
S="$HOME/Library/Application Support/MarioKartSuperCircuitRecomp"
mkdir -p "$S/saves"; cd "$S"
cp -Rn "$D/mods" "$D/assets" . 2>/dev/null
[ -f game.toml ] || cp "$D/game.toml" game.toml
exec "$D/MarioKartSuperCircuitRecomp-bin" "$@"
EOS
chmod +x "$MACOS/MarioKartSuperCircuitRecomp"
ICONKEY=""
if [ -n "${ICON:-}" ]; then cp "$ICON" "$APP/Contents/Resources/AppIcon.icns"; ICONKEY='<key>CFBundleIconFile</key><string>AppIcon</string>'; fi
cat > "$APP/Contents/Info.plist" <<EOP
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>CFBundleName</key><string>Mario Kart Super Circuit Recomp</string>
<key>CFBundleExecutable</key><string>MarioKartSuperCircuitRecomp</string>
<key>CFBundleIdentifier</key><string>io.github.mstan.mksc-recomp</string>
<key>CFBundlePackageType</key><string>APPL</string>
<key>CFBundleShortVersionString</key><string>${VERSION}</string>
<key>NSHighResolutionCapable</key><true/>
${ICONKEY}
</dict></plist>
EOP
# Ad-hoc sign the executables so Apple Silicon will run the local build.
codesign --force -s - "$MACOS/MarioKartSuperCircuitRecomp-bin"
codesign --force -s - "$APP" || echo "note: ad-hoc signing of the bundle reported a warning (the app still runs locally)"
echo "Built $APP"

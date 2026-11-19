#!/usr/bin/env bash
# POSIX counterpart of tools/regen.ps1 (macOS / Linux).
# Recompiles the retail GBA BIOS and the verified USA ROM into native sources.
#   usage: tools/regen.sh [--rom path] [--no-symbols] [--max-functions N]
# Prerequisites: roms/mario_kart_super_circuit_usa.gba, gbarecomp/bios/gba_bios.bin,
# and gba_recompile built: cmake -S gbarecomp -B gbarecomp/build -G Ninja &&
#                          cmake --build gbarecomp/build --target gba_recompile
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ROM="$ROOT/roms/mario_kart_super_circuit_usa.gba"; MAXF=65536; SYMBOLS=1
while [ $# -gt 0 ]; do
  case "$1" in
    --rom) ROM="$2"; shift 2;;
    --no-symbols) SYMBOLS=0; shift;;
    --max-functions) MAXF="$2"; shift 2;;
    *) echo "unknown option $1" >&2; exit 2;;
  esac
done
ENGINE="$ROOT/gbarecomp"; TOOL="$ENGINE/build/gba_recompile"
[ -x "$TOOL" ] || { echo "Missing $TOOL; build the gba_recompile target first." >&2; exit 1; }
[ -f "$ROM" ] || { echo "Missing ROM: $ROM" >&2; exit 1; }
sha1() { if command -v shasum >/dev/null; then shasum -a 1 "$1" | cut -d' ' -f1; else sha1sum "$1" | cut -d' ' -f1; fi; }
EXPECTED=9d327c030c3e2d9007990518594f70c3340ac56f
ACTUAL="$(sha1 "$ROM")"
[ "$ACTUAL" = "$EXPECTED" ] || { echo "Mario Kart ROM SHA-1 mismatch: got $ACTUAL expected $EXPECTED" >&2; exit 1; }

# The retail BIOS is recompiled into the runtime (game.toml: [bios] hle = false).
[ -f "$ENGINE/bios/gba_bios.bin" ] || { echo "Missing $ENGINE/bios/gba_bios.bin" >&2; exit 1; }
( cd "$ENGINE" && "$TOOL" --bios bios/gba_bios.bin --config bios/gba_bios.toml \
    --out src/runtime/generated_bios )

ARGS=(--rom "$ROM" --config "$ROOT/game.toml")
S="$ROOT/symbols"
if [ "$SYMBOLS" = 1 ] && [ -f "$S/AMKE_symbols.toml" ]; then
  ARGS+=(--config "$S/AMKE_symbols.toml")
  [ -f "$S/imported_symbols.tsv" ] && ARGS+=(--symbols "$S/imported_symbols.tsv")
  [ -f "$S/imported_data_symbols.tsv" ] && ARGS+=(--data-symbols "$S/imported_data_symbols.tsv")
fi
ARGS+=(--out "$ROOT/generated" --max-functions "$MAXF")
"$TOOL" "${ARGS[@]}"

# v0.1.3 saving validation

The engine fix is gbarecomp `6b9667b`: flash512 now identifies as Macronix
MX29L512 (`C2 1C`); flash1m retains `C2 09`. The game's byte-checked RAM hook
handles the SDK's flash helpers at varying stack addresses. Reviewed result
callbacks are also compiled.

Validation used the supported USA cartridge and retail BIOS, fresh save files,
no mods, and disabled self-healing. No ROM, BIOS, saves or generated guest code
are included in the repository or release.

- The flash protocol regression failed with the old ID and passes with the fix,
  covering both densities, snapshot IDs, programming, reload, banking and erase.
- The flash dispatch test passes template matching, overlapping stack slots,
  instruction-offset resume, bounds and altered-byte rejection.
- Both packaged Windows and Linux builds completed the controller-only route
  in `tests/mksc-time-trial-save.input` and displayed **THE RACE RESULTS WERE
  SAVED**. Both ran 9,839 frames with zero dispatch misses, zero interpreted
  instructions and no unmapped/unhandled accesses.
- Both wrote identical 65,536-byte cartridge saves (SHA-256
  `01a710fad33f7241801ad1e3c16b806d359fdeaa13b4912995acae3d7e873186`).
  The Windows package then loaded that file in a fresh process, showed the
  1:22.03 Peach Circuit ghost, and replayed it with zero dispatch misses.
- Windows rollback and delay-sync application checks passed 180 ticks and
  a fresh-process 60-tick paired-session resume. Cross-platform rollback and
  delay-sync cold sessions agreed between the Windows ZIP and Linux AppImage.
  One cross-platform delay-sync resume attempt timed out at checkpoint agreement.
- ZIP integrity and private-image exclusion checks passed. The AppImage's
  packaged 1,500-frame strict-static smoke passed. Both artifacts have the
  same generated netplay build identity.

To replay the save route, set `GBARECOMP_INPUT_REPLAY` to the absolute fixture
path, `GBARECOMP_STRICT_STATIC=1` and `GBARECOMP_SELFHEAL_RECOMPILE=0`, then run
an executable with `--no-launcher --no-window --frames 9839 --rom <ROM>`
`--bios <BIOS> --save-path <new-save> --dump-png <screenshot>`. Use a new save
path so existing records do not affect the menu flow.

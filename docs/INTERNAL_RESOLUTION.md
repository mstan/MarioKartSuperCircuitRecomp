# Higher internal resolution pilot — 2026-10-19

Implements [issue #12](https://github.com/mstan/MarioKartSuperCircuitRecomp/issues/12)
for USA AMKE revision 0. Enable **Higher Internal Resolution** in recomp-ui's
Mods page and select 2x, 3x, 4x, 8x or 16x. It is disabled by default in the catalog;
4x remains the initial selection and recommended maximum. 8x and 16x are
labeled **(experimental)** and may run very slowly. The in-game menu labels
every scale above 4x experimental; these remain optional choices. In v0.1.7, Display offers explicit Native, 16:9 and
Adaptive views.
Default (Mods) retains existing widescreen mod settings. Adaptive follows the
window/display aspect; the mod's corresponding choice is Fit to screen. Netplay
has its own separate view selection.
Adaptive Widescreen is independent and supported. In-game display settings can
change scale for the current run; `--internal-resolution 1..16` overrides the
initial mod choice. New artwork is not generated: sprites, HUD and menus retain
their authored pixels. The perspective road receives actual fractional samples.

The initial pilot used `feature/hd-affine-presentation-20261019` in the engine
and title, with an explicit `GBARECOMP_ROOT`. The v0.1.6 integration pins the
updated engine and includes the newer main-branch fixes. Historical pilot
measurements below name their original fixtures and executables.

The follow-up [8x CPU rendering candidate](HD-PERFORMANCE-2026-10-19.md)
measures a cached HLE affine sampler against its maintained direct LLE build.
It improves that matched Windows route by 61.5%, while remaining below full
speed. The owner approved default promotion and it is integrated on main;
v0.1.8 is the first release containing that optimization.

## Boundary

The unreleased [course-dependent 16:9 follow-up](COURSE-WIDESCREEN-2026-10-28.md)
records the later all-course border audit. Its scope is separate from the
historical pilot measurements below.

The title authorizes mode-1 BG2 sampling before the race HUD appears, including
course previews and the fly-in. Widescreen/HUD placement still uses its stricter
race-layout recognition. The engine consumes each published raster row, preserves layer/window/blend order,
and reconstructs intermediate affine samples into a separate surface. Vertical
subline transforms extrapolate adjacent published rows; this is an optional
presentation approximation. It neither runs extra DMA nor alters guest memory,
timing, native framebuffer or canonical state. Original artwork remains native.

Logical and sampled dimensions are separate. At the observed 3440x1440 monitor,
Fit to screen selects 382x160 logical pixels and uploads 1528x640 at 4x. Native
aspect produces 960x640 at 4x. SDL presents that texture directly, retaining
logical aspect/layout coordinates and existing color grading.

Netplay has a separate local presentation mirror. The canonical PPUs remain
240x160. Enhanced buffers/history do not enter serialization or state hashes;
restore and resize invalidate incomplete presentation. The current netplay
runner accepts the CLI scale; its mod selection is not an online compatibility
profile. Existing strict source/build identity checks remain in place, and the
HLE/LLE compositor selection is included. No cross-build compatibility is claimed.

## Bounded evidence

Inputs were local owned assets and are not committed or bundled:

- ROM SHA-1: `9d327c030c3e2d9007990518594f70c3340ac56f`.
- Retail BIOS SHA-1: `300c20df6731a33952ded8c436f7f186d25d3492`.
- Race checkpoint SHA-256:
  `c62aeb4cfa2739a5541072f7f5c531877c989e7a371f2f2f9535ca258130e9d3`,
  starting at frame 10016, plus `tests/mksc-gameplay-fuzz.input`.

Windowed observations executed 600 presentations, ending at guest frame 10615,
with Adaptive Widescreen, affine filtering and 4x enabled. Each completed
313,374,464 affine samples over 51,272 affine rows. The optimized executable
reported 598 completed HD frames actually presented; the first two presentations
occur before a complete enhanced raster. All runs reported zero dispatch misses,
interpreted instructions, unmapped accesses and unhandled IO. Automated audio
used a dummy device while audio emulation/draining remained active.

| Actual-window observation | Seconds | Presentations/s |
|---|---:|---:|
| Initial scalar coordinates | 12.159244 | 49.345 |
| Per-row decoded tile cache, rejected | 12.357757 | 48.553 |
| Coordinates prepared per subline | 7.373467 | 81.373 |
| Scalar repeat under variable host load | 23.283614 | 25.769 |
| Subline repeat under variable host load | 14.852560 | 40.397 |

The host was not isolated; foreign builds were observed during this work and
absolute timings varied substantially. These samples support retaining the
coordinate change, not a stable 60 FPS promise or a general campaign speedup.
The second pair also favors the change, by approximately 57% in presentations/s.
No average across different host-load conditions is reported.

The coordinate change removes six signed divisions per affine output sample
from the emitted hot loop. Quotients are now computed once per subline while
preserving signed truncation and operation order. All observed final 4x images
are byte-identical, SHA-256:
`357cc290821465538870d43ac1b878752a15cee0729e77c28ab5039a81315db3`.
Frozen scalar executable SHA-256:
`abf51563204c536679e2e703985f4b9cbf5f1dab4a17e366108891fdcb4d6f83`.
Optimized HLE pilot executable SHA-256:
`3cde3112463694ceb01e300d3c734d880f93611e7cd363bc9b6ed0fa6ffc4e57`.

A phase-instrumented, native-aspect 4x diagnostic put approximately 83% of loop
time in PPU rendering. It is not compared as a performance acceptance run.
This directed work at the renderer. Generated CPU bodies remain at the original
O1 default; `MKSC_GENERATED_OPTIMIZATION=O2` is available for a future separately
measured experiment. No Xbox compiler gain is assumed to transfer to this title.

## Determinism and checks

The high-resolution, bus and netplay-mirror fixtures pass under both configure
selections, `GBARECOMP_SCANLINE_IMPLEMENTATION=HLE` and `LLE`. The existing title
widescreen/HUD test also passes. The HLE selection uses exact tile-span decoding
and native-color composition; the original LLE compositor remains in source.
An actual LLE title run also completed 600 presentations with the saved 4x mod
selection and no CLI scale override, presenting 598 complete enhanced frames.
Its final image matches the HLE/scalar image hash above and its state has the
same inactive-RTC-only difference. LLE executable SHA-256:
`5a91b0e86448cae08f03800dce5837aa18961ab7bc967f61c669bee57951b9fe`.

Real-game 1x versus 4x and scalar versus optimized diagnostic snapshots match
exactly in CPU0, IO_0, AUD0, SAV0, PPU0 and META. BUS0 differs only at its inactive
offline RTC timestamp (`current_seconds`, bytes 395310..395317 in this snapshot
schema). The changed bytes were 395310 and 395311; RTC active is false. Raw files
are retained. Whole snapshot file equality is not claimed or silently normalized.

The paired-session fixture uses the deterministic netplay clock. Its raw-state
SHA-256 is `cd07805d04de8780ea50d4477d353ce84049a687779f3dde4cdb26b176299d8f`;
both checksums of its source archive were verified before local test extraction.
Production archive/build identity checks were unchanged. Native, 16:9, 21:9,
32:9, adaptive, native 4x, 16:9 2x and adaptive 4x each ran 72 frames with every
canonical session byte identical to the native reference on every frame. Each
non-reference case restored twelve frames and replayed to the identical state.
Adaptive widths covered 240, 284, 382, 569, 317 and 576. Captures show an actual
race, including margin road content and edge HUD. This is a same-build local
differential, not an Internet transport or cross-platform qualification.

Local raw logs, images, snapshots, executable hashes and observation JSON live
under `build-hd/qa/`. This is a narrow race pilot, not an all-course/mode audit.
Unverified scope includes broad campaign behavior, other ROM revisions, other
GPUs/OSes and long online matches. Source portability uses ordinary C++/SDL;
no new assembly, SIMD requirement or GPU readback behavior was introduced.

## Reproduce

Configure this title against the updated engine using `-DGBARECOMP_ROOT=<engine>`
and the ordinary verified generated ROM/BIOS corpus. The pilot reused the
previously qualified, unedited 64-shard corpus, not handwritten generated fixes.
Build `MarioKartSuperCircuitRecomp`, `mksc_extended_view_tests`,
`mksc_margin_objects_tests`, and optionally
`MarioKartSuperCircuitRecompViewProbe` for the local paired differential.

Run one observation with the actual local paths:

```powershell
python tools/check-internal-resolution.py --exe build/MarioKartSuperCircuitRecomp.exe `
  --rom <AMKE0-ROM> --bios <retail-BIOS> --state <race.state> `
  --config game.toml --output build/qa/4x --scale 4 --frames 600 --window `
  --replay tests/mksc-gameplay-fuzz.input
```

The tool mutes only its child, isolates cartridge saves, preserves logs, validates
asset hashes and coverage, and checks sampled dimensions and actual HD presents.
`--catalog-selection` omits the CLI scale to test saved mod activation. Owner
interactive builds use normal audio by default.

## Course intro and roadside objects follow-up

Owner QA found that the fly-in initially used native road samples and switched
abruptly when the HUD appeared. The sampling permission incorrectly depended
on LAP/TIME tiles. It now depends on the affine display mode, independently of
HUD placement. `tools/check-course-intro.py` drives the ordinary cup selection
inputs, records native and enhanced frames, and checks increasing affine work
through the visible pre-HUD fly-in. An optional checkpoint must precede frame
8200 on that route. The checked 8360..8640 sequence stays at 960x640 with fresh
affine samples; all 23 captured native image hashes match the baseline. The
baseline had no enhanced affine samples through frame 8640.

The original game also omits roadside objects outside its narrow draw region.
Expanding an existing OAM sprite alone cannot recover an object never submitted
to OAM. `mksc_margin_objects.cpp` reads AMKE0's resident actor lists, camera,
sprite cells and mip selections, projects missing opaque billboards, and supplies
pixels only in the margins. Static scenery uses the expanded geometric frustum
instead of the original checkpoint draw slice. Dynamic actors retain their
scene policy. Matching existing OAM pieces are reused, including their unwrapped
local X beyond the signed nine-bit boundary at 256. No guest memory, OAM, ROM,
AI, timing or native center pixels are modified. Presentation caches invalidate
on restore, including a restore to the same frame, and on width changes.

The verified AMKE0 layout has race state at `03002E44`, camera at `03003668`,
and active lists at race offsets `6F4`, `704` and `714`. The original draw path
at `08049B0C` calls projection `0802F638` and horizontal clipping `0802F674`.
Cell center decoding preserves the original scaled-X asymmetry at `0802F91C`.
These are ROM/layout facts; generated CPU bodies remain unedited.

The focused tests cover unseen objects on both sides, original and expanded
edges, X=255/256, real mip/palette changes, native OAM reuse, malformed/cyclic
lists, same-frame restore, resize and exact canonical device-state equality.
Both title tests pass. The shared high-resolution, bus and presentation-mirror
tests also pass under the HLE and LLE build selections.

An ordinary acceleration/steering input route found a never-submitted tree at
frame 9016, projected X=-66. At the actual 382x160 adaptive view, before/after
4x images add 528 sampled tree pixels at the expanded left edge, with zero
changed pixels in the original 240-pixel center. The fixture SHA-256 is
`42e434b6ef63e3a9e11ae5600fd9c28699554c3829607d3c079160850fc2c559`.
An additional ordinary-input pose at frame 9092 has a native-projected tree
at X=299, beyond signed OAM X=255. Its comparison adds 2,864 sampled pixels
across the two margins, including that right-side tree, with zero center
changes. Its fixture SHA-256 is
`72908de65df2dbba9931102bfa31e895bda927ef7de693301bfe71121f2c2edf`.
These three-presentation comparisons check visibility, not performance.
For the left-edge comparison, CPU0, IO_0, AUD0, SAV0, PPU0 and META match exactly; BUS0 differs only
at the inactive offline RTC timestamp, byte 395310. Raw snapshots are retained.

The final paired-session differential again passes all 72 frames and twelve-frame
restore/replay checks across the views/scales described above, comparing every
canonical byte. Margin diagnostics record actual rendered pixels in the wider
views. Neither this nor the offline RTC observation claims cross-build netplay
compatibility. Actual tree evidence covers Peach Circuit; an all-course prop
audit, other ROM revisions and translucent/OBJ-window billboard cells remain
unverified. The new provider handles opaque billboard cells.

Evidence is under `build-hd/qa/intro-hd-final`, `tree-edge-before`,
`tree-edge-after`, `tree-edge-right-before`, `tree-edge-right-after` and
`netplay-visibility-final`. Follow-up executable SHA-256:
`2ff2d087108dcae3152f2285c3a136be87cb0c3193df775baa31c11415d616fc`.

## Local test handoff

`build-pilot-visibility-20261019/MarioKartSuperCircuitRecomp.exe` is the updated
Windows pilot with both follow-up fixes, its required DLLs, recomp-ui assets
and all three mod packages. `build-pilot-20261019` is the earlier pilot and
does not contain these fixes.
Its saved selection enables 4x and Adaptive Widescreen; Track Rendering 60 FPS
stays disabled. Volume is 100 and no dummy audio setting is persisted. Local
ROM/BIOS paths are preselected without copying those inputs. Saves are local
to the pilot directory. The source catalogs remain disabled by default.

The original staged executable passed a muted 120-presentation smoke test without an
added MinGW DLL search path and without a CLI scale override. It presented 118
complete 1528x640 enhanced frames and reported zero misses/interpreter/unhandled
IO. The updated visibility pilot independently passed the same 120-presentation
packaging check from the left-edge fixture, using its saved mod selection,
with 118 complete 1528x640 enhanced frames and zero coverage/IO failures.
Required DLL imports and the absence of bundled ROM/BIOS bytes were verified.
The updated pilot also cold-booted without a savestate or input replay for 600
paced presentations, ending at guest frame 764 with zero coverage/IO failures.
All 600 presents used the completed 1528x640 surface. No affine samples are
expected on the title screens reached by that no-input boot.
Owner launcher interaction and live in-game scale editing remain manual QA.
The pilot itself was a local test handoff. The v0.1.6 packages are built
separately from the integrated source, with clean player settings.

Initial implementation commits: engine `60892e4`, title `a21cccb`.
Follow-up commits are on the same feature branches; engine diagnostic command:
`e211f81`. Beads: `beads-7j3s`.

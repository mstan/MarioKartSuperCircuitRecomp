# 8x CPU rendering candidate, 2026-10-19

Owner feedback: v0.1.7 is too slow at 8x and 16x. This candidate replaces the
HD renderer's repeated tile/map/palette walks with a persistent indexed affine
surface. Changed tiles/map cells update at each published scanline; palette
colors refresh every row. The integer bilinear filter processes three separate
channels in one portable 64-bit calculation. Ordinary rows expand once and copy
their subrows. Guest CPU execution and canonical rendering/state are unchanged.

The existing native tile-span compositor was already HLE in v0.1.7. These changes
optimize its separate high-resolution presentation service. Select the maintained
direct sampler with `GBARECOMP_HD_SAMPLER_IMPLEMENTATION=LLE`; the candidate uses
`HLE`. Selection is fixed at build time, reported as `hd_sampler_backend`, and
included in strict netplay source identity. No cross-build compatibility is
claimed, and canonical simulation/profile checks are unchanged.

## Matched Windows measurement

AMD Ryzen 7 9800X3D, Windows, D3D11; installed GPUs include RTX 3080 Ti and the
integrated Radeon. No GPU selection/isolated-host guarantee is inferred.
Release build, existing 64-shard generated corpus at O1, runtime at O3.
Owned AMKE0 ROM and retail BIOS, fixed logical 382x160 view, 8x raster
3056x1280, frontlit color model and affine filtering. Actual hidden 382x160 SDL
window, muted audio output with normal audio work, uncapped/vsync off, frozen RTC.
Expensive diagnostics disabled; phase counters enabled. Load the same frame-9000
checkpoint and present 300 frames through 9299. Frame dumps and startup are
outside the reported bounded interval. This small destination window does not
establish fullscreen performance on the owner's 3440x1440 monitor.

| Sampler | FPS | Bounded seconds | Inclusive scanline CPU ms/frame |
| --- | ---: | ---: | ---: |
| Direct LLE | 20.403 | 14.703795 | 38.70 |
| Cached HLE | 32.941 | 9.107101 | 21.92 |

The matched implementation pair shows **61.5% higher FPS**, still below full
speed. The HLE observation precedes the LLE observation. Host load varies: the
initial public-v0.1.7 observation was 15.749 FPS, so quoting the apparent 109%
gain over that older observation would overstate the better-matched result.
This is one stationary Peach Circuit route, not a gameplay-wide guarantee.
No automatic timing matrix or repeat-until-pass campaign was run.

The `render_scanline` phase is included in `guest_us`: treating all `guest_us`
as guest CPU execution would misidentify the bottleneck. Presentation still
costs roughly 5.5 ms/frame in the HLE run, including grading/upload. A GPU affine
service remains the larger next opportunity if 8x must reach full speed.

HLE executable SHA-256:
`7e4d3bc6d6c2a4559028a368216fe3a63d87511ed62ea21514f20a207ce41beb`.
LLE executable SHA-256:
`d243d2b9e0698f9a06fee3293bc08dad71aa55a1cb1d9e48895a3287d3f5ee61`.
Evidence: `build-release/qa/hd-perf-20261019/{inline,direct}.json`, corresponding
`*-8x/run.log`, phase CSVs, final states and images. Earlier `baseline`,
`indexed-ready` and `packed` observations retain the investigation steps.

## Correctness and handoff

Both final images match the v0.1.7 image exactly. PNG SHA-256:
`7f01b261b116e0aa36937750adc3682e905342d8ac109adf2d2abb865448c833`.
The complete final single-player snapshot matches in all measured builds:
`579999e6a3822043d738a015f72e815255537ad6003842bd600103a63ba19f39`.
All runs report zero static dispatch misses, interpreted instructions, healed
native calls, unmapped accesses and unhandled IO. A current image was inspected:
road, sprites, margin scenery and HUD are present without garbling.

Existing high-resolution, bus and presentation-mirror tests pass in the cached
HLE and direct LLE configurations. The raster fixture adds map/layout changes
and filter channel saturation/rounding, while retaining per-row source writes,
blend/HUD checks and reset/restore/resize coverage. Both title visibility tests
pass. Both samplers compile for wasm; browser presentation still uses the
existing native-density shared-memory protocol. Full non-Windows builds and
performance remain unqualified for this candidate.

The actual two-machine game view probe compares every canonical byte for all
72 frames of each existing native/wide/adaptive case and enhanced scales through
16x, including 12-frame restore/replay for each nonreference case. It passes
with the cached HLE sampler. Evidence: `hd-perf-20261019/paired.log`. This is a
local deterministic differential, not an actual transport or cross-build test.

The owner candidate must use the measured HLE executable above, fresh boot,
8x Adaptive/Fit to screen and normal audio. Keep the existing owner's saves and
prior playtest directory intact. The owner confirmed improved speed and
authorized default promotion. Engine `ec778f1` and title `9bc2520` are integrated
on main; the cached HLE sampler is the shared build default. The public v0.1.7
download remains unchanged. Central issue:
`beads-bce3`; other selected games in that issue are still pending.

## Default rollout and row-reuse follow-up

The owner-approved cached implementation is published on engine main
(`ec778f1`, documentation follow-up `b9508a2`) and title main (`9bc2520`,
documentation follow-up `2fc91af`). Pin-only updates are published on the default
branches of all 24 maintained game repositories in this workspace. No other
game is qualified by association, and no game is forced to enable HD. Explicit
cached LLE selections remain respected. Public v0.1.7 remains unchanged.

The further candidate caches the exact semantic inputs of completed enhanced
rows and copies an existing row only when every input matches. These include
source-surface versions, affine history, the complete palette, layers, windows,
black masks, blending and filtering. Field comparisons do not hash padding or
retain guest pointers. Metadata is stamped for the next completed latch;
repeated/partial rows and rows replaced by native/foreign-overlay content
cannot reuse stale pixels. Restore/invalidation and resize require a complete
new raster. This is private presentation work; strict netplay identities and
canonical state checks remain in force. Cross-build compatibility is not
claimed.

### Measurements on the older NAS

Intel i5-4670 (4 cores/4 threads), GTX 760, Windows 10 Pro build 19044. The NAS
runs the same owned binaries/assets, 300 presents from frame 9000 through 9299,
8x 3056x1280 raster, fixed logical width 382, filtering/frontlit grading,
muted output with audio work retained, uncapped/vsync off and fixed RTC.
Processes ran below normal priority; the SSH-session SDL renderer reports
accelerated OpenGL, not D3D11. This small-window measurement is not a guarantee
for an interactive NAS desktop or fullscreen. Uploaded inputs were hash-checked;
private ROM/BIOS/state files are never included in a public build.

| Implementation | FPS | Bounded seconds | Inclusive scanline CPU ms/frame |
| --- | ---: | ---: | ---: |
| Direct LLE | 11.048 | 27.154001 | 68.99 |
| Cached HLE, published default | 14.509 | 20.676553 | 47.96 |
| Cached HLE plus row reuse, candidate | 15.581 | 19.253608 | 42.34 |

Cached HLE gains **31.3%** over direct LLE here. Row reuse gains another **7.4%**.
On the desktop, its separate matched continuation measured 29.104 to 31.905 FPS
(+9.6%); compare that pair directly rather than combining it with the earlier
32.941 FPS observation under different host load. Both hosts report 4,436
reused rows of 25,628 affine rows, with samples reduced from 626,553,344 to
518,102,016. Every completed measured run has zero dispatch/interpreter/heal
or unmapped/unhandled-IO activity and the same image/state hashes listed above.
The NAS candidate image was inspected and is clear.

The first NAS game's successful exit was misread by the PowerShell harness
because its managed Process.ExitCode was null. The corrected harness reads the
native process exit code; the replacement direct observation is reported above.
The initial harness result remains setup evidence and is excluded from timing.
No run was repeated to select a better score.

A separate partial-upload experiment reached 32.118 FPS on the desktop and
16.140 on the NAS, versus row-only 31.905/15.581. Those small additional changes
are uncertain, and the experiment adds upload history memory and complexity.
Its source change was removed; the candidate below contains row reuse only.
GPU affine sampling/filtering is a future investigation, not a delivered gain.

Row-only executable SHA-256:
`1c86c82910c0a47fa50636cd3c21a6e8f76520f0de4f51d6d89c118482378127`.
Its focused high-resolution, bus and presentation-mirror tests pass in HLE and
LLE builds, and both samplers compile for wasm. Added cases exercise row reuse,
palette changes, ordinary-row replacement, repeated changed rows and partial
rasters after invalidation. Owner playtesting is pending for this additional
candidate, which is not included in the published default pins.

Private evidence beneath `build-release/qa/hd-perf-20261019/`: `row-reuse.json`,
`cached-control.json`, their `*-8x` output folders, and `nas-evidence-full.zip`.
The archive preserves `hardware.json`, `observations-v2.json`,
`observations-v3.json`, per-run logs, phase CSVs, final states and images.
`out2-direct`, `out2-cached` and `out3-row-only` are the reported NAS runs;
`out2-candidate` retains the rejected combined upload experiment.

The row-only real-game view probe also passes all ten native/wide/adaptive
cases through 16x: 72 frames each with identical whole canonical state, plus
12-frame restore/replay for every nonreference case. Evidence:
`paired-row-reuse.log`. This is a local deterministic differential; no actual
network-transport or cross-build compatibility claim is made. Candidate engine
commit: `fecdb8deb7c61e3bd50a642e33859cc41e37d36a`. This branch pins that candidate;
published default branches retain `ec778f1` pending the new owner playtest.

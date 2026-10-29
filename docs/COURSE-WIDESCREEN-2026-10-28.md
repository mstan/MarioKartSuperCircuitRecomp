# Course-dependent 16:9 borders

This follow-up covers Mario Kart: Super Circuit USA AMKE revision 0. It fixes
the race-layout policy used by Adaptive Widescreen and fixed 16:9. Scenery is
rendered from the game's actual maps and affine transforms in the additional
columns; the image is not stretched or cropped to hide borders.

The old policy recognized Peach Circuit's scenery registers: a 512-pixel BG2
map and priority 3 for BG1, BG2 and BG3. Other courses use 256-pixel maps,
different BG3 dimensions, and different priorities. Riverside Park, Boo Lake,
Cheese Land, Luigi Circuit, Sky Garden and Rainbow Road illustrate those
differences. The old policy pillarboxed 33 of the 40 race courses in the
recorded poses; seven already expanded correctly.

The policy now checks the exact HUD background layout and live LAP/TIME tiles.
It leaves the sky layers' dimensions and priorities to the course content.
Battle scenes use their portrait/item-box frame and battle mode instead of
LAP/TIME. Their HUD balloons move with the left-anchored portraits; world
balloons retain their projection. Menus, previews, results and battle rankings
keep native framing. The existing pause and state-epoch invalidation paths
remain covered by the provider tests.

No shared-engine change, new rendering backend, CPU feature requirement or
resolution-cap change is involved. The title retains the resident GPU path,
portable CPU rendering and the existing 64x limit. The supported ROM identity
remains SHA-1 `9d327c030c3e2d9007990518594f70c3340ac56f`.

## Inventory and audit scope

The inventory comes from the ROM's standard course-order table at `080E7464`,
Extra order table at `080E74C4`, and battle order table at `080E7524`, resolving
metadata through `080E7FEC`. The pinned decomp's `sub_8033BAC`, `sub_8033BBC`,
`sub_8033BDC`, and `loadtrack_8031990` establish that mapping. The inventory is
20 standard race courses, 20 Extra race courses, and four battle arenas.

Cheat-assisted selectors change the course/type before the guest loads assets.
Each case verifies the actual loaded metadata pointer. Extra-course unlock
requirements are bypassed and are not claimed as tested. Each race capture
covers the start, 180 frames of acceleration, and 160 frames of steering.
Images cover road/horizon continuity, changing camera angles, both margins,
and HUD/minimap placement. This is a border-policy audit, not a full race,
collision, exit, item, scoring or all-prop-mechanics qualification.

The battle fixtures run two native GBAs in a real local cable domain. Both
drivers are initialized before loading the selected arena. The selector sets
the battle type and course; it does not set a win, loss, balloon-loss or result
flag. Single-driver rankings and offline two-driver communication-error screens
were explicitly rejected as gameplay evidence. Internet transport and extended
matches are outside this audit.

The initial accepted binary also lacked address-taken course actor callbacks.
The audit records those as a separate baseline defect, fixed through reviewed
`game.toml` metadata and the ordinary regeneration pipeline. The callback ROM
references and entry instructions are in [COURSE-CALLBACKS.json](COURSE-CALLBACKS.json).
Generated C++ was not edited. Diagnostic interpreter runs were used to find
missing entries and do not count as passing native qualification.

## Evidence

The accepted starting identities are title `8d05c71db6ca1d97a261081324e6471411cfc184`,
engine `a385e0b6d3f6c6a95c4ee6aeb873c158cee7300e`, and ARM core
`63dd32dd210a8bf18bbd11933393a7b87f9b3bf6`.

The local evidence index is `C:/Temp/Codex/mksc-courses-20261028`. It retains the
course inventory, per-course results, commands, exact executable hashes,
private reproduction checkpoints, raw images, contact sheets, current-build
restore/replay comparisons and measured PC/SERVER capacity receipts. Native
race captures use 284x160 at 1x with the software presenter. Resident-GPU race
captures use 568x320 at 2x with OpenGL, and compare GPU readback with the final
image. Battle captures use the separate local presentation mirror.

Matched comparisons check CPU, RAM, IO, audio, saves and device timing. Offline
snapshots also contain a cropped presentation latch, which changes when the
HUD is moved; its pixels are recorded separately. Paired-session comparisons
check the entire canonical two-GBA snapshot because presentation lives in a
separate mirror. Current-build restore/replay uses newly saved checkpoints;
backward save-state compatibility is not a release requirement.

Validation completion and final source/binary identities are recorded in the
local audit manifest and Beads `beads-5ae.1.6` / `beads-5ae.1.7`. No performance
gain, other ROM revision, other GPU/OS qualification, merge or release is
implied by this narrowly scoped correctness audit.

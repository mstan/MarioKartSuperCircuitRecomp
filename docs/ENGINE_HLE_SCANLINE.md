# Opt-in shared scanline experiment

This draft retains v0.1.4's saving, checkpoint and speedometer changes and pins
a shared-framework experiment based on the same maintained `29efc028` floor.
The build defaults to LLE. Select HLE explicitly with:

```powershell
cmake -S . -B build-scanline-hle -DGBARECOMP_SCANLINE_IMPLEMENTATION=HLE
cmake --build build-scanline-hle --target MarioKartSuperCircuitRecomp
```

The replacement operates at the shared scanline caller: native backgrounds use
tile spans; native and adaptive composition retain GBA 555 color until final
RGB publication. Every adaptive scene/provider lookup remains unchanged. No
Mario Kart address intercept or track-60fps feature is required.

Current title `b89fbb4` and framework base `29efc028` preserve upstream PRs 6–9.
Both production SDL selections build, sharing unchanged generated guest objects;
existing PPU checks pass, including actual wide-center/bitmap/overlay cases.
The adaptive-view display package is a separate existing option and remains
opt-in. Runtime CLI flags do not override a disabled package feature. Its fixed
view cap is 240, while adaptive view supports up to 480; real desktop fullscreen
or an actual client resize selects the extended width.

Historical native race evidence used earlier title `79dffea`/framework `e3c834d`:
one valid 1200-frame pair showed 14.09% more FPS, 12.35% less loop time. That is
not relabeled as current-pin/adaptive evidence. The current adaptive pair is
pending; rejected width-240 setup arms do not count as adaptive gains. All runs
preserve normal SDL render/audio/present work and isolate player saves. Owner
play and any scoped Windows default remain pending. No merge/default is implied.

The pinned framework experiment preserves this title's older caller floor.
Mainline shared code is reviewed separately in gbarecomp PR 29; do not silently
replace this pin with a newer ABI or merge the old floor wholesale into main.
Private ROMs, generated guest code, state fixtures and captures stay local.
## Current real-adaptive result (2026-10-07)

One corrected current-pin pair used real SDL borderless desktop 3440x1440 and
actual logical width 382x160, not a forced drawable. Both arms completed the
same active frames 10016 to 11215, 1200 presents, 321101 steps and 2116299579
cycles, final PC `0x0806134a`, with zero dispatch misses/interpreted/healed,
unmapped or unhandled-I/O counts. LLE loop time was 3.782089 s (317.285 FPS);
HLE was 3.538319 s (339.144 FPS): 6.889% more FPS and 6.445% less loop time.
This is a modest gain below the roughly 10% planning goal, not a target-met or
automatic-default claim. No repeat was made. Ordinary render/audio/present work
was retained, observer OFF, uncapped pacing only. Previously active foreign
PSX build contention is disclosed; no isolated-host inference is made.

One HLE extended race image is clear: kart, road/scenery and HUD render normally.
Normal-paced real-adaptive HLE and the LLE alternative are staged with exact
binary hashes/arguments in the private review manifest. Owner value/feel decision
is pending, and title PR 10 remains draft/default LLE as requested. No game is
launched while the owner is away. The historical native 14.09% FPS result stays
separate; the earlier wrong-width arms are setup evidence only.
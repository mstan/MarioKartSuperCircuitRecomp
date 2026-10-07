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
# Benchmark

Only measured numbers. Each table says when and how it was measured.

## The original, baseline (F0, 2026-11-11)

- `flare-engine@56c747a` with `tools/bench/original.patch`, release
  build (g++ 13.3, `-std=c++98 -O2`: the engine adds `-O2` after the
  `-O3` of the CMake release type), software renderer, SDL dummy video
  and audio drivers, no audio, in this container (4 cores of an Intel
  Xeon at 2.10 GHz).
- Window 1280x720; the engine renders at 1706x960 internally.
- Started by `tools/bench/measure.sh`: a new game of the first class
  (`FLARE_NEWGAME=0`) on the given map, no input, seed 1.
- 600 logic frames: 10 s at 60 fps. Times include the start of the
  program and the load of the map. Two runs, one after the other.

| Scenario | CPU s, capped | Peak MB | Wall s, capped | Wall s, uncapped |
|----------|---------------|---------|----------------|------------------|
| title screen | 4.48 / 4.76 | 73.3 / 73.4 | 10.13 / 10.15 | 4.04 / 3.96 |
| `frontier_outpost` (first map) | 9.71 / 9.53 | 268.0 / 268.8 | 11.10 / 11.08 | 8.70 / 8.71 |
| `frontier_plains` (56 enemies) | 6.64 / 6.81 | 489.5 / 485.5 | 11.46 / 11.51 | 4.89 / 4.84 |
| `averguard_complex` (180 enemies) | 6.45 / 6.73 | 432.6 / 434.1 | 11.26 / 11.31 | 4.56 / 4.81 |
| `cave1` (138 enemies) | 7.46 / 7.71 | 487.1 / 487.3 | 11.38 / 11.43 | 5.00 / 5.21 |

- "capped" is the game as it runs, at 60 fps; "uncapped" runs the same
  600 frames with no frame cap (`FLARE_FAST`).
- The enemy counts are the lines of enemies in the map files, not the
  enemies alive in the run.
- Size of the patched binary: 2235736 bytes (2220736 unpatched).
- Build of the original: 43 s of wall time with `make -j4`.

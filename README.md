# Flare in Atmos

A port of a slice of [Flare](https://flarerpg.org) (Free Libre Action
Roleplaying Engine), a 2D action RPG, to
[Atmos](https://github.com/fsantanna/ceu-atmos), over SDL2.

The goal is a study: in the C++ original, behavior is kept in state
enums, timers, and flags that are tested every frame; here each entity,
power, hazard, effect, and screen is one task, written as a sequence.

## Target

| What | Where | License |
|------|-------|---------|
| Engine | `flareteam/flare-engine@56c747a` | GPL-3 |
| Game data | `flareteam/flare-game@af6eee6` | CC-BY-SA 3.0 |

- First version: the game screen with the `alpha_demo` campaign over
  the art of `fantasycore`.
- Second version: the Empyrean Campaign with its menus.

The plan is in `fsantanna/ceu-atmos`, `.claude/plans/261111-flare.md`.

## Licenses and credits

- The code of this port derives from the engine of Flare, so it is
  GPL-3 (`LICENSE`).
- The art and data of Flare are not in this repository and are not
  modified: they are read from a checkout of `flare-game` and keep
  their license, CC-BY-SA 3.0, and their credits (`CREDITS.txt` in
  `flare-game`, `CREDITS.engine.txt` in `flare-engine`).
- Flare is by Clint Bellanger, Justin Jacobs (dorkster), and the
  contributors listed in those files.

## Use of AI

This port is written with an AI assistant (Claude Code), with Francisco
Sant'Anna directing the work and reviewing it. The assistant writes the
code, the tests, the tools, and the documents of this repository.

In respect of the
[AI policy of Flare](https://github.com/flareteam/flare-engine/blob/master/AI_POLICY.md):

- this is a separate project, and nothing of it is sent upstream;
- no art, sound, or game data is generated: the port only reads the
  original data.

## Milestones

- [x] F0: repository, licenses, this readme; the original built,
      patched, and measured in the container: `tools/bench/` and
      `tst/orig/` (`tools/bench/test.sh`), `doc/fidelity.md`,
      `doc/bench.md`.
- [x] F1: the INI reader and the mods (`src/ini.atm`): all text files
      of `alpha_demo` and `fantasycore` (and of `empyrean_campaign`)
      read exactly as the original reads them (`tools/compare-ini.sh`).
- [x] F2: the engine (`engine/flare.h`, `engine/engine.atm`): window,
      images, fonts, sounds, and the test mode with no screen, with the
      same variables, generator, and input as the patched original;
      animations (`src/anim.atm`) loaded exactly as the original loads
      them (`tools/compare-anim.sh`).
- [x] F3: a map drawn (`engine/map.h`, `src/map.atm`): tilesets,
      layers, animated tiles, the isometric view, and the camera; the 24
      maps of `alpha_demo` give the same pixels as the original
      (`tools/compare-map.sh`). Parallax layers and the fog of war, used
      only by the Empyrean Campaign, are left for F9.
- [x] F4: the hero (`src/hero.atm`, `src/play.atm`, `engine/collision.h`):
      the input, moving, the collision, and the camera, with the same
      trace as the original, frame by frame, on scripted runs in the
      first map (`tools/compare.sh`). The path finding is used only by
      enemies and by moving with the mouse: it comes with the enemies, in
      F5.
- [ ] F5 (in course): enemies (`src/ai.atm`, `src/enemy.atm`), powers
      (`src/power.atm`), hazards and hits (`src/combat.atm`), effects
      (`src/effect.atm`), loot (`src/loot.atm`), path finding
      (`engine/astar.h`). The run `fight` (an antlion fights the hero to
      its death) is the same as the original, with its draws.

## Tests

    tst/run.sh               # the tests of the port, with no screen
    ASAN=1 tst/run.sh        # the same, with the address sanitizer

## The original, for comparison

    tools/bench/setup.sh     # clone, patch, and build it in ../flare-orig
    tools/bench/test.sh      # its scripted runs against the stored traces
    tools/bench/measure.sh   # its baseline

Building it needs cmake, g++, and the development files of SDL2,
SDL2_image, SDL2_mixer, and SDL2_ttf.

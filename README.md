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

- [ ] F0: repository, licenses, this readme; the original built,
      patched, and measured in the container.

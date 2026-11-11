# Fidelity

The goal (`doc/decisions.md`): with a fixed seed and scripted input,
the port prints the same trace as the patched original.

## The patched original

`tools/bench/setup.sh` builds `flare-engine@56c747a` with
`tools/bench/original.patch`, outside the repository (`../flare-orig`
by default). `tools/bench/run.sh` runs it with no screen and no audio,
in a fresh home, with fixed settings (window 1280x720, software
renderer, `mouse_move=0`, `mouse_aim=1`).

The patch adds `src/Bench.h`, included before every source file
(`-include`), and a few hooks. All is driven by environment variables:

| Variable | Effect |
|----------|--------|
| `FLARE_SEED=<n>` | seed of the generator that replaces `rand()` (`run.sh`: 1) |
| `FLARE_FAST=1` | no frame cap: one logic frame and one render per loop (`run.sh`: on) |
| `FLARE_FRAMES=<n>` | exit after n logic frames |
| `FLARE_INPUT=<file>` | scripted input, by logic frame |
| `FLARE_TRACE=<file>` | the trace (`-` for stdout) |
| `FLARE_DUMP=1` | also the hero in every frame |
| `FLARE_NEWGAME=<i>` | skip the title: a new game of class i (`run.sh`: 0) |
| `FLARE_MAP=<file>` | the first map of that game, instead of `maps/spawn.txt` |
| `FLARE_SHOT=<n>:<file>` | save the screen at logic frame n, as BMP |

## The generator

`rand()` and `srand()` are macros for a 64-bit LCG: the state goes
`s = s * 6364136223846793005 + 1442695040888963407` (mod 2^64), and the
result is `s >> 33`, from 0 to `RAND_MAX` (2^31-1, as in glibc). The
seed is the state. Every use of chance in the engine goes through it
unchanged, so the port must call it in the same order: also to pick a
sound of steps, of a hit, or of a death, with or without audio.

## Scripted input

One event per line, `<frame> <command> <args>`, applied before the
input of that logic frame is handled (frames count from 0). Lines that
start with `#` are comments; the first may set variables for
`tools/bench/test.sh`: `# env: FLARE_FRAMES=240 FLARE_DUMP=1`.

| Command | Event |
|---------|-------|
| `move <x> <y>` | the mouse moves to x, y |
| `down <b>`, `up <b>` | mouse button b (1 left, 3 right) |
| `key <name>`, `keyup <name>` | a key, by its SDL scancode name (`W`, `Space`, `1`) |
| `quit` | the window closes |

Mouse coordinates are those of the internal render, 1706x960 with the
settings of `run.sh` (the dummy driver does not scale them). The hero is
drawn at the center, near (853, 480).

## The trace

One event per line: the logic frame, the kind, then its fields. Floats
are printed with `%.9g`, enough to tell any two 32-bit floats apart.
Entities have ids: 0 is the hero, and the others are numbered from 1 in
the order they are first seen, again from 1 at each map.

| Line | When |
|------|------|
| `<f> map <file>` | a map starts to load |
| `<f> event <activate> <x> <y> <w> <h> <components>` | an event of the map runs its components (after its delay and chance) |
| `<f> spawn <id> <name> <state> <x> <y> <hp>` | an entity is seen for the first time, at the end of a frame |
| `<f> state <id> <state> <x> <y>` | its state (`cur_state`) changed since the last frame |
| `<f> hit <id> <hp>` | its hp went down since the last frame |
| `<f> power <id> <power> <x> <y>` | it asks to activate a power, at a target |
| `<f> loot <item> <quantity> <x> <y>` | loot is put on the ground |
| `<f> hero <x> <y> <state> <hp> <direction>` | with `FLARE_DUMP`, every frame |

The states are those of `StatBlock`: 0 stance, 1 move, 2 power, 3
spawn, 4 block, 5 hit, 6 dead, 7 critdead. The activation of an event
is that of `Event`: 0 trigger, 1 interact, 2 map exit, 3 leave, 4 load,
5 clear, 6 static.

`spawn`, `state`, and `hit` are found by comparing every entity at the
end of each logic frame of the game with the frame before, so they show
what changed in a frame, not the order inside it. `power`, `event`,
`loot`, and `map` are printed where they happen, in order.

## The stored runs

`tst/orig/<name>.in` and `.trace`: the input and the trace of the
original. `tools/bench/test.sh` runs the original on each and compares;
`-u` stores them again. Two runs with the same seed and input give the
same trace.

| Run | What |
|-----|------|
| `walk` | the first map: walk with W and D, one attack in the air |
| `fight` | `cave_of_living_bones1`: walk to an antlion, attack it until the hero dies |

## Differences

None measured yet: the port does not run.

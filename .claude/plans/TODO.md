# TODO: what is pending in the session of the Flare port

Kept current by the session: an item leaves when it is settled, and one
comes whenever something is left open. Last review: 2026-11-13, through
the whole conversation.

## A. Waiting for an answer of the author

1. **The remote of the compiler checkout.** The compiler moved to
   `ia-atmos/lang`; `git remote set-url origin
   https://github.com/ia-atmos/lang` in `../ceu-atmos` was denied by the
   permission check of the session. Recommended: the author runs it, or
   allows it. Meanwhile the old address redirects; nothing breaks. The
   same for this repository, now `ia-atmos/sdl-flare` (was
   `fsantanna/flare`, then `ia-atmos/flare`): the `origin` of this
   checkout still names `fsantanna/flare`, and pushes go through the
   redirect. Recommended: `git remote set-url origin
   https://github.com/ia-atmos/sdl-flare`.
2. **The friction of `pin` with module tables** (item 40 of the TODO of
   `ia-atmos/lang`): a pool's tasks in a global table, a per-map context
   in a module, a task in its own `pub`. Recommended, in order: the error
   naming the destination of the escape (cheap); a binding of a module
   field for the lifetime of a block. Meanwhile: arguments, or one
   pinned context table (`doc/compiler.md`).

3a. **When to relax exactness for expressiveness** (the rule of the
    author of 2026-11-14, relayed by the compiler session: allowed once a
    port is fully faithful, keeping the intent of the original). Where
    Flare stands: the 30 stored runs (walks, a fight, duels and stands in
    13 maps) give the same trace as the original, draws included, and the
    enemies and hazards the same pixels; but the slice is not complete
    (F5 to F7 open), so the rule of the plan still holds: a difference
    is a defect. Recommended: relax only after F7 (the alpha demo played
    by script, identical end to end), with a tag on the faithful commit,
    one small commit per relaxation, and a list of deliberate
    differences in `doc/fidelity.md`; not mixed with the move to
    `env-sdl`. Candidates then: the AI timers and flags kept as state
    only to keep the order of the draws (`doc/state.md`), and the hidden
    draws of the original. To ask the author before any of it.

## B. Known work, not done

3. F5, left: the game over screen; combat text; the powers of types
   transform and block (the hero's).
4. F6: items and inventory, the HUD and menus of the slice, loot picked
   up; on screen: the loot on the ground (`loot_animation` of items),
   the layers of the hero (equipment), NPCs (the captured trader of
   `cave1`), the animations of effects on entities (shield, heal).
5. F7: the alpha demo played by script, end to end; the benchmark of the
   port against `doc/bench.md` (no number of the port is measured yet);
   the documents.
6. F8 and F9, as in the plan (the Empyrean Campaign: its menus, parallax
   layers, fog of war).
7. A leak of 132 KB at the end of the game (`ATMOS_ASAN=1` on `fight`),
   from the animation sets of `anim.cache`: to find the cycle.
8. `await(n, :e)` of the compiler may replace the task `Frames` and the
   frame counters of the port.
9. Mark F5 done in the README when it closes.

10a. To come: move the platform part of the layer (window, images,
    fonts, sounds, input, the test mode of `engine/flare.h` and
    `engine/engine.atm`) to the shared environment `ia-atmos/env-sdl`,
    when asked (Flare is planned third, after Pingus and SNKRX), with
    the golden outputs identical; the game part (`astar.h`,
    `collision.h`, `map.h`, `geom.h`) stays. Meanwhile, keep new
    platform code apart from game code, and answer the questions of the
    session `env-sdl` about the layer.

## C. Reported or decided, not acknowledged

10. The three items of the plan marked to confirm (slice, data read as
    is, fidelity by trace) were settled as the plan recommended, after
    "Faça tudo o que for preciso. Vá em frente" (`doc/decisions.md`).
11. Scope of F5 by the data of the alpha demo: its enemies use only
    melee and ranged AI powers (no on_hit, half_dead, on_death, debuff),
    and none can flee; those parts are not ported. Drawing loot, the
    hero's layers, NPCs, and effect animations moved from F5 to F6.
12. The patch of the original gained `FLARE_DUMPE`, `FLARE_NOHUD`, and
    `FLARE_NOHERO` (trace and screenshots only; no change of behavior).
13. The compiler: the values of `until <cnd>, <vals>` are evaluated with
    the condition, unlike the reading of the manual
    (`doc/compiler.md`); not known whether the compiler session took it.
14. The golden output of the runs was committed corrupted once (two
    writers at the same time) and restored in `fe3e438`; the suite and
    its update are now run one after the other.

## D. Older

15. The permission to install packages (SDL2 development files, `time`)
    was asked and given at the start; settled, kept here for the record
    until acknowledged as seen.

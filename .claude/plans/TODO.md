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
   same for this repository, moved to `ia-atmos/flare` (GitHub says so
   on push): the `origin` of this checkout still names `fsantanna/flare`.
2. **The friction of `pin` with module tables** (item 40 of the TODO of
   `ia-atmos/lang`): a pool's tasks in a global table, a per-map context
   in a module, a task in its own `pub`. Recommended, in order: the error
   naming the destination of the escape (cheap); a binding of a module
   field for the lifetime of a block. Meanwhile: arguments, or one
   pinned context table (`doc/compiler.md`).

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

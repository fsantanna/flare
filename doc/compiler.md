# The compiler

What the port found in `fsantanna/ceu-atmos`: problems, limits, and what
would help. Each item says where it showed and what the port does
meanwhile.

## A local function cannot call itself

- Where: `ini.resolve` (`src/ini.atm`), a recursive helper inside a
  function, to add a mod after the mods it requires.
- `val func g (n) { ... g(n - 1) ... }` inside a function fails with
  `undeclared variable` at the inner `g`: the name is declared only after
  its body. And `func g` there is an error, as the manual says, since `g`
  would be a variable of the program.
- `var g = nil` then `set g = func (n) { ... g(n - 1) ... }` works, but
  leaks: the function captures the variable that holds it, a cycle that
  reference counting does not reclaim (`ASAN=1 tst/run.sh` reported it).
- Meanwhile: a function of the program (`func g` outside functions),
  with what it would capture passed as arguments.
- What would help: `val func g` visible in its own body, with no cycle.
- Fixed in `ceu-atmos@5b25d89`: `val func` and `val task` are visible in
  their own bodies, with no capture. `ini.resolve` uses it again. Two
  local functions that call each other still need a function of the
  program.

## The values of `until` are evaluated with its condition

- Where: the actions of the hero (`src/hero.atm`): `until h.next, h.next.act`
  failed with `expected table : got nil` when `h.next` was nil.
- The manual says that `until <cnd>, ...` is `if <cnd> { break (... ||
  <cnd>) }`, which reads as if the values were evaluated only when the
  condition holds; they are evaluated before.
- Meanwhile: `if h.next { break h.next.act }`.
- What would help: the manual saying so, or the values evaluated only
  on a break.

## Pinned values cannot reach the tables of a module

- Not a bug: the rule of the manual (`invalid escape : pinned value`).
  Three shapes the port wanted, and what it does instead:
  - The tasks of the pool of the enemies in a field of the global `play`,
    for the other modules to walk them in order. Meanwhile: `pin ens = []`
    next to the pool, passed as an argument (`play.trace`, `play.focus`,
    `play_spawns`, `hazards_logic`).
  - The hero, the enemies, and the hazards of a map as context of
    `combat`, set once per map. Meanwhile: arguments of `hazards_logic`.
  - A task in its own `pub` (`task = task`), to reach the task from its
    `pub`. Meanwhile: who emits keeps the task from the walk of the pool.
- The error gives only the line of the escape; finding what escaped took
  a while when the pin and the escaping `set` are far apart.
- What would help: a binding of a module field for the lifetime of a
  block, weak references to tasks, a task allowed in its own `pub`, or
  the error naming the destination. Sent to the ceu-atmos session.
- Answer of the ceu-atmos session (2026-11-12): language decisions, for
  the author; the third port with this friction. What works today: one
  pinned context table in the block of the pool (`pin ens = []` and the
  other tables each on its own line, then `pin ctx = [ens = ens, ...]`),
  passed as one argument, or functions defined inside that block. A task
  in its own `pub` works only for a task pinned at the top level.

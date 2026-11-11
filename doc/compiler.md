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

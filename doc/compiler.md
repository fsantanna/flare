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
- Meanwhile: `var g = nil` then `set g = func (n) { ... g(n - 1) ... }`.
- What would help: `val func g` visible in its own body.

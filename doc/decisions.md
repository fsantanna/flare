# Decisions

The plan (`fsantanna/ceu-atmos`, `.claude/plans/261111-flare.md`) left
three items to confirm. The author told this session to go ahead, so
they are settled as the plan recommended:

- **Slice**: the game screen first, with the `alpha_demo` campaign over
  the art of `fantasycore`, menus reduced; then the Empyrean Campaign
  with its menus.
- **Data**: the original INI files are read as they are, by a reader of
  the port, so that the original mods work unchanged. No converter.
- **Fidelity**: with a fixed seed and scripted input, the port prints
  the same trace of events as the patched original. If that proves out
  of reach, fall back to timings, formulas, and distributions, and say
  so in `doc/fidelity.md`.

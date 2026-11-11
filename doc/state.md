# State variables

Every state enum, timer, and flag of the C++ that the port covers, and
what became of it. Allowed as state: positions, stats, the inventory,
the statuses of the campaign, and what the HUD shows.

## The hero (`Avatar`, F4)

| Original | Kind | In the port |
|----------|------|-------------|
| `stats.cur_state`: `ENTITY_STANCE`, `ENTITY_MOVE` | enum, `switch` per frame | the action that runs: the tasks `Stance` and `Move`, each a loop over the steps of the hero that ends with the next action (`src/hero.atm`) |
| `set_dir_timer` | timer, ticked per frame, reset when the keys are released | the task `Turning`: after a turn, 6 steps or the release of the keys |
| `teleport_camera_lock` | flag | `camera_lock` of the hero, until the camera is near (kept: it gates the input) |
| `activeAnimation`: `cur_frame`, `sub_frame`, `sub_frame_f`, `times_played`, `reverse_playback` | counters, advanced per frame | a cursor (`anim.cursor`), advanced by the hero in each frame: what frame to draw, as a position |
| `stats.pos`, `stats.direction` | position | `x`, `y`, `dir` of the hero (positions) |

`state` of the hero is kept only for the trace and the drawing: no code
tests it to know what to do.

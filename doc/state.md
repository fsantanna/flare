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

## The enemies (`EntityBehavior`, F5)

| Original | Kind | In the port |
|----------|------|-------------|
| `stats.cur_state`: `ENTITY_STANCE`, `MOVE`, `POWER`, `HIT`, `DEAD` | enum, `switch` in `updateState` | the actions `EStance`, `EPower`, `EHit`, `EDead` (`src/ai.atm`), each a task over the steps of the enemy, ending with the next action |
| `activated_power`, `instant_power` | pointer, flag | the argument of `EPower` |
| `corpse` | flag | `corpse` of the enemy, set at the end of `EDead` (kept: the frame logic skips a corpse) |
| `in_combat`, `join_combat`, `los`, `encountered`, `fleeing`, `move_to_safe_dist`, `collided` | flags, per frame | fields of the enemy: the perception of the frame, tested in the order of the original, which decides the order of the draws |
| `cooldown`, `cooldown_los`, `turn_timer`, `waypoint_timer`, `flee_timer`, `flee_cooldown_timer`, `path_found_fail_timer`, the cooldowns of the AI powers | timers, ticked per frame | timers of the enemy (`timer`, `timer_tick` in `src/ai.atm`): still state |
| `effects` (`EffectManager::effect_list`, with a timer per effect) | list walked per frame | a task per effect in a pool of its entity (`src/effect.atm`), which sets the status while alive |
| hazards (`HazardManager::h`, each with `lifespan`, `delay_frames`) | list walked per frame | a list of the frame, in reverse order as the original (`src/combat.atm`) |

## The hero, in combat (F5)

| Original | Kind | In the port |
|----------|------|-------------|
| `cur_state` `ENTITY_POWER`, `ENTITY_HIT` | enum | the actions `HPower` and `HHit` (`src/hero.atm`) |
| `action_queue` | vector, filled by the action bar | `actions` of the hero, taken in its frame |
| `power_cast_timers`, `stats.cooldown` | timers | `casting` and `cooldown` of the hero: still state |

What stays as state is what the original tests in a fixed order each
frame, with draws of the generator between the tests: making it a task
that awakes on its own would change the order of the draws.

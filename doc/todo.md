# What comes next

The milestones are in the README; this is the working list of the one
in course, with what was found in the original along the way.

## F5: enemies, powers, hazards, effects

Target runs: `hero-cave` (enemies standing while the hero walks), then
`fight` (an antlion fights the hero to its death). Both are stored in
`tst/orig`; add them to `tst/runs.txt` when they pass.

1. (done) Enemy groups of a map (`[enemy]` sections of the alpha demo are
   groups): `Map::loadEnemyGroup`, `MapRenderer::pushEnemyGroup` and
   `enemyGroupPlaceEnemy`, `EnemyGroupManager` (`enemies/*.txt` by
   category, level, and rarity: common 6, uncommon 3, rare 1). In
   `cave1`, 69 groups draw 414 numbers at the load: per group a chance
   (`percentChanceF`), a count (`randBetween`), a position in the area
   (`rand() % w`, `rand() % h`, also for an area of 1), the enemy
   (`getRandomEnemy`), a `Map_Enemy` (its constructor draws), and a
   direction if the group has none.
2. (done, in part) `EntityManager::handleNewMap`: the entities from
   `mapr->enemies`, in order; `enemy.stats` reads only name, level, hp,
   and speed of the enemy file so far.

Status (later): the run `fight` passes in full. Left in F5: the powers of
types repeater, spawn, transform, block; on_hit, on_half_dead, on_death,
on_debuff powers; fleeing and wandering checked against runs; the death
of the hero (game over); combat text; the drawing of enemies and hazards.

Status (earlier): `hero-cave` is the same as the original up to frame 222, where
an event of the map (`power=132` of an event with a hotspot) makes event
StatBlocks spawn two hatchlings. In the trace, the ids of the patch go
by first sight of a StatBlock pointer: the two event StatBlocks print as
70 and 71 in the `power` lines, and the spawned hatchlings as 70 and 71
in the `spawn` lines (to look at when events come).
3. `StatBlock` in full (`calcBase`, `applyEffects`, `recalc`, `logic`:
   regeneration), for the hero and the enemies.
4. `EntityBehavior::logic`: the AI per frame, with its draws.
5. `MapCollision::computePath` and the A* containers (the order of ties
   in `AStarContainer` matters).
6. Powers, hazards, effects, `Entity::takeHit`, loot, combat text.

Tools: `FLARE_RNG=1` gives the draws per frame; a build of the original
with backtraces at each draw (see `doc/fidelity.md`, Hidden draws) finds
where they come from.

## Notes on EntityBehavior (read, not yet ported)

- `logic`: a corpse only ticks its timer; an enemy farther than
  `encounter_dist` from the hero (10.77 tiles here: sqrt(8^2 + 20^2)/2,
  with integer divisions of the render by the tile) does nothing until
  `encountered`; then `doUpkeep` (passives, `stats.logic`, teleport),
  `findTarget`, `checkPower`, `checkMove`, `updateState`.
- `findTarget`: target the hero (or allies), line of sight within
  `threat_range` (raycast 0.1), `join_combat` (beacon power, then the
  `on_join_combat` power sets ENTITY_POWER), leaving combat beyond
  `threat_range_far` or at the end of `cooldown_los`; wander points;
  fleeing: `percentChanceF(chance_flee)` only when in combat, in stance,
  between `melee_range` and `flee_range` (= threat_range/2 by default),
  then `randBetween` over the free directions.
- `checkPower`: only in combat, with `stats.cooldown` ended, in stance or
  move: `getAIPower` of half_dead, ranged (beyond melee_range), or melee;
  power needs line of sight if `requires_los`; sets ENTITY_POWER and
  resets the animation.
- `checkMove`: the turn timer (`turn_delay`, capped by 1/real_speed), the
  path (A*, with `percentChance(chance_calc_path += 5)`, -100 after a
  computation), the direction to `pursue_pos`; stance moves on
  `percentChanceF(chance_pursue)` beyond melee range; `faceNextBest` when
  blocked; waypoints.
- `updateState`: advance the animation, then STANCE/MOVE set animations;
  POWER: at the first frame pre chain powers, attack speed, sound,
  state_duration, charge; at the active frame `powers->activate` and the
  cooldown of the AI power; at the last frame `stats.cooldown` begins and
  back to stance; SPAWN until the last frame; HIT until the last frame;
  DEAD/CRITDEAD: die animation, death power at the second-last frame,
  corpse at the last.
- The fight run: the blinker (id 45) uses power 132 at frame 10, at its
  join_combat: the powers (`PowerManager`, `powers/*.txt`) are needed
  before any of this can be compared.

## Notes on powers (read, not yet ported)

- `PowerManager::activate`: traces `power`; the hero needs `requires_mp`;
  a source with `hp <= requires_hp` fails; block powers go to `block`;
  a power with speed and a target at the origin aims at `melee_range` in
  the direction of the source (or `rand() % 8` if the origin is not the
  source); then by type: fixed, missile, repeater, spawn, transform.
- `fixed`: `count` hazards with `delay_frames` stepping by `delay`, then
  `buff`, the sound, and the cost.
- `initHazard`: source type (hero, ally, enemy, or the power's), crit and
  accuracy of the source, damage min/max of the base damage type (or its
  conversion, and elementals of the source when the base is not), the
  animation, direction (directional: towards the target; visual_random:
  `rand() % n` + visual_option), speed, lifespan, active unless
  no_attack, the position (source, target clamped to target_range,
  melee: `calcVector(origin, dir, melee_range)`, melee_unlocked), and
  `getRandomNeighbor` for target_neighbor (a draw).
- `Hazard::logic`: waits `delay_frames`, counts down `lifespan`, advances
  its animation, moves by speed (or follows the source), and dies at a
  wall (`COLLIDE_TYPE_HAZARD`). `isDangerousNow`: active, no delay, and
  its animation at an active frame (or no animation).
- `HazardManager::logic`: removes the expired (in reverse, with expire
  chain powers), takes the new ones, then for each hazard in reverse:
  logic, wall chain powers, and if dangerous, each entity in order (not
  the allies of a hero source, not the enemies of an enemy source), in
  radius, once per hazard: `takeHit`; then the hero for enemy and
  neutral sources. A hit of a hazard that is not multitarget makes it
  inactive and ends it (unless complete_animation).
- `Entity::takeHit` (Entity.cpp:319-600), the draws in order: reflect for
  missiles; overhit if accuracy beats avoidance by more than 100; the
  miss `percentChanceF(true_avoidance)` clamped by min/max avoidance;
  `randBetweenF(min, max)` per damage type (all types, also zero ones);
  absorption `randBetweenF(abs_min, abs_max)` with min/max absorb or
  block; crit `percentChanceF`; crit, overhit, and miss damage
  multipliers `randBetweenF`; `resourceRound`; "miss" if 0; then
  `takeDamage`, the effects of the power, hp and mp steal; a beacon only
  makes the target join combat.

## F6: events of maps (notes, being ported)

- Keys of `[event]` in the alpha demo (count): type, location (526),
  activate (525), soundfx (422), hotspot (251), tooltip (233), repeat
  (212), mapmod (198), loot (134), intermap (56), msg (50),
  requires_status (39), power (32), power_damage (29), set_status (28),
  requires_not_status (25), cooldown (21), power_path (16), intramap
  (14), unset_status (9), shakycam (7), spawn (5), requires_item (5),
  remove_item (4), stash (1), reachable_from (1).
- `loadEvent`: activate on_trigger (0), on_interact (1), on_mapexit (2),
  on_leave (3), on_load (4, not kept), on_clear (5, not kept), static
  (6); location x,y,w,h and its center; hotspot (or "location"); cooldown
  and delay (set, then ended); reachable_from.
- Components, one per key, but a list makes several of the same type:
  mapmod (layer,x,y,tile ...), set/unset/requires(_not)_status (s ...),
  spawn (type,x,y ...), requires/remove_item (id:qty ...), loot (with
  its tables, as parseLoot). soundfx (file, x, y, loop), intermap (map,
  x, y, spawn id), intramap (x, y), power (id), power_damage (min, max),
  power_path (x, y, "hero" | x, y), shakycam (duration), msg, tooltip,
  repeat (bool).
- The trace prints `event <activate> <x> <y> <w> <h> <components>`.
- In hero-cave at frame 222, an on_trigger event spawns two hatchlings
  (EntityManager::spawn, then handleSpawn in the same frame, in state
  SPAWN), whose beacons print `power 70` and `power 71`.

## Open

- `ATMOS_ASAN=1` on the fight run reports 132 KB in 1030 allocations not
  freed at the end, from the animation sets of `anim.cache`
  (`src/anim.atm`). The unit tests of animations do not leak: it comes
  with the game (a cycle through the pubs of tasks kept in global
  tables, such as `combat.last_enemy`, `play.world`?). To find out.

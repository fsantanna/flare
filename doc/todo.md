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

Status: `hero-cave` is the same as the original up to frame 222, where
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

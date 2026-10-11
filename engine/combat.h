
/// Combat, by the rules' values (engine/rules.h), in two steps of the
/// world's tick: before troops move, each vintaine chooses by its mode
/// whether it fights, which enemy, and where its anchor and troops go;
/// after they move, melee troops lock into duels, duels strike and ranged
/// troops shoot, all against the state at the start of the step, and the
/// dead are removed together at its end.

#ifndef vasal__engine__combat_h
#define vasal__engine__combat_h

#include "engine/world.h"

/// Chooses targets by mode and where fighting troops head for.
void vasal_engine_combat_targets(vasal_engine_world_s* const world);

/// Pairs duels, strikes and shoots, and removes the dead.
void vasal_engine_combat_resolve(vasal_engine_world_s* const world);

#endif

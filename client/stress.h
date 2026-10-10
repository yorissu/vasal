
/// The stress test of drawing (client.render#1): opened with ?stress in
/// the address, the client fills its world with 99 more vasals of a full
/// centaine each, 5 of them around the player and the rest across the
/// map, about 10,000 troops in all and over 500 in view, and keeps them
/// marching to new places so the simulation works too. The client then
/// shows how fast it draws.

#ifndef vasal__client__stress_h
#define vasal__client__stress_h

#include "client/game.h"

/// How many vasals stand around the player, in view.
#define vasal_client_stress_near 5U

/// The stress test's state between frames.
typedef struct vasal_client_stress_s {
	/// The state of its fixed sequence of numbers.
	uint32_t random;
	/// The last tick it gave orders at.
	uint32_t tick;
} vasal_client_stress_s;

/// Adds the other vasals and their troops to the game's world.
void vasal_client_stress_start(vasal_client_stress_s* const stress,
	vasal_client_game_s* const game);

/// Gives one other vasal new orders on every tick that has passed since
/// the last call, in turn, to march somewhere near where it stands.
void vasal_client_stress_update(vasal_client_stress_s* const stress,
	vasal_client_game_s* const game);

/// How many troops the world holds.
size_t vasal_client_stress_troops(const vasal_engine_world_s* const world);

#endif

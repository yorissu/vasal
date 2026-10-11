
/// Places come from a fixed sequence of numbers, so every run of the test
/// is the same. A vasal is placed on the nearest passable tile to where
/// the sequence points.

#include "client/stress.h"

// How far from the player the near vasals stand, and how far a vasal
// marches at most, in tiles.
#define _vasal_client_stress_near_distance 14
#define _vasal_client_stress_march 30

// Where the near vasals stand around the player, in steps of the near
// distance.
static const int32_t _vasal_client_stress_around[vasal_client_stress_near][2]
	= { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 }, { 1, 1 } };

// The next number of the sequence.
static uint32_t _vasal_client_stress_next(
		vasal_client_stress_s* const stress) {
	stress->random = (stress->random * 1103515245U) + 12345U;
	return (stress->random >> 8U) & 0xffffffU;
}

// A value kept within 0 and highest.
static int32_t _vasal_client_stress_clamp(const int32_t value,
		const int32_t highest) {
	int32_t kept = value;
	if (kept < 0) {
		kept = 0;
	}
	if (kept > highest) {
		kept = highest;
	}
	return kept;
}

// The passable tile nearest to (x, y), searched in growing squares, or
// (x, y) itself, clamped to the map, when none is within 32 tiles.
static vasal_engine_map_point_s _vasal_client_stress_land(
		const vasal_engine_map_s* const map, const int32_t x,
		const int32_t y) {
	const int32_t right = (int32_t)map->width - 1;
	const int32_t bottom = (int32_t)map->height - 1;
	vasal_engine_map_point_s found = {
		(uint16_t)_vasal_client_stress_clamp(x, right),
		(uint16_t)_vasal_client_stress_clamp(y, bottom)
	};
	for (int32_t r = 0; r <= 32; r++) {
		for (int32_t dy = -r; dy <= r; dy++) {
			for (int32_t dx = -r; dx <= r; dx++) {
				const int32_t tx = _vasal_client_stress_clamp(x + dx, right);
				const int32_t ty = _vasal_client_stress_clamp(y + dy, bottom);
				if (vasal_engine_map_terrain(map, (uint16_t)tx,
						(uint16_t)ty)->passable) {
					found.x = (uint16_t)tx;
					found.y = (uint16_t)ty;
					return found;
				}
			}
		}
	}
	return found;
}

// Adds a vasal with a full centaine at the passable tile nearest to
// (x, y).
static void _vasal_client_stress_add(vasal_engine_world_s* const world,
		const int32_t x, const int32_t y) {
	const size_t vasal = vasal_engine_world_add_vasal(world,
		_vasal_client_stress_land(world->map, x, y));
	if (vasal_engine_world_vasals_max == vasal) {
		return;
	}
	for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
		for (size_t t = 0U; t < vasal_engine_world_troops; t++) {
			(void)vasal_engine_world_add_unit(world, vasal, g,
				t % (size_t)world->rules->unit_count);
		}
	}
}

// spec: client.render#1
void vasal_client_stress_start(vasal_client_stress_s* const stress,
	vasal_client_game_s* const game) {
	vasal_engine_world_s* const world = game->world;
	const vasal_engine_map_s* const map = world->map;
	stress->random = 12345U;
	stress->tick = world->tick;
	const vasal_engine_world_position_s at =
		vasal_client_game_group(game, 0U)->anchor;
	const int32_t x = at.x / vasal_engine_map_precision;
	const int32_t y = at.y / vasal_engine_map_precision;
	for (size_t i = 0U; i < vasal_client_stress_near; i++) {
		_vasal_client_stress_add(world,
			x + (_vasal_client_stress_around[i][0]
				* _vasal_client_stress_near_distance),
			y + (_vasal_client_stress_around[i][1]
				* _vasal_client_stress_near_distance));
	}
	while (world->vasal_count < vasal_engine_world_vasals_max) {
		const uint32_t rx = _vasal_client_stress_next(stress) % map->width;
		const uint32_t ry = _vasal_client_stress_next(stress) % map->height;
		_vasal_client_stress_add(world, (int32_t)rx, (int32_t)ry);
	}
	// The new troops have no positions of the tick before yet.
	game->pending = 0.0f;
	for (size_t v = 0U; v < world->vasal_count; v++) {
		for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
			const vasal_engine_world_group_s* const group =
				&world->vasals[v].groups[g];
			game->previous_anchors[v][g] = group->anchor;
			for (size_t t = 0U; t < group->troop_count; t++) {
				game->previous_troops[v][g][t] = group->troops[t].position;
			}
		}
	}
}

void vasal_client_stress_update(vasal_client_stress_s* const stress,
	vasal_client_game_s* const game) {
	vasal_engine_world_s* const world = game->world;
	const uint32_t others = (uint32_t)world->vasal_count - 1U;
	while ((0U < others) && (stress->tick != world->tick)) {
		stress->tick++;
		// Every vasal but the player's, in turn.
		size_t vasal = (size_t)(stress->tick % others);
		if (vasal >= game->vasal) {
			vasal++;
		}
		const vasal_engine_world_position_s at =
			world->vasals[vasal].groups[0].anchor;
		const int32_t span = (2 * _vasal_client_stress_march) + 1;
		const uint32_t step_x = _vasal_client_stress_next(stress)
			% (uint32_t)span;
		const uint32_t step_y = _vasal_client_stress_next(stress)
			% (uint32_t)span;
		const int32_t dx = (int32_t)step_x - _vasal_client_stress_march;
		const int32_t dy = (int32_t)step_y - _vasal_client_stress_march;
		const vasal_engine_map_point_s to = {
			(uint16_t)_vasal_client_stress_clamp(
				(at.x / vasal_engine_map_precision) + dx,
				(int32_t)world->map->width - 1),
			(uint16_t)_vasal_client_stress_clamp(
				(at.y / vasal_engine_map_precision) + dy,
				(int32_t)world->map->height - 1)
		};
		(void)vasal_engine_world_order_move(world, vasal, 0x1fU, to);
	}
}

size_t vasal_client_stress_troops(const vasal_engine_world_s* const world) {
	size_t count = 0U;
	for (size_t v = 0U; v < world->vasal_count; v++) {
		for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
			count += world->vasals[v].groups[g].troop_count;
		}
	}
	return count;
}

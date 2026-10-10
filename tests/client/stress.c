
/// Tests of the drawing stress test's world.

#include "client/stress.h"

#include "cairo.h"

static vasal_engine_map_s _vasal_tests_client_stress_map;
static vasal_client_game_s _vasal_tests_client_stress_game;

// spec: client.render#1
cairo_test_new(client_stress, ten_thousand_troops_and_five_hundred_near) {
	vasal_engine_map_s* const map = &_vasal_tests_client_stress_map;
	(void)vasal_engine_map_clear(map, 256U, 256U);
	const vasal_engine_map_spawn_s spawn = { { 60U, 120U }, 5U };
	map->spawns[0] = spawn;
	map->spawn_count = 1U;
	vasal_client_game_s* const game = &_vasal_tests_client_stress_game;
	vasal_client_game_start(game, map);
	vasal_client_stress_s stress;
	vasal_client_stress_start(&stress, game);
	const vasal_engine_world_s* const world = game->world;
	cairo_assert_eq(world->vasal_count, vasal_engine_world_vasals_max);
	cairo_assert_ge(vasal_client_stress_troops(world), 10000U);
	// The player's centaine and the near vasals stand within 20 tiles.
	size_t near = 0U;
	const int32_t reach = 20 * vasal_engine_map_precision;
	for (size_t v = 0U; v < world->vasal_count; v++) {
		for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
			const vasal_engine_world_group_s* const group =
				&world->vasals[v].groups[g];
			const int32_t dx = group->anchor.x - (60 * 256) - 128;
			const int32_t dy = group->anchor.y - (120 * 256) - 128;
			if ((dx > -reach) && (dx < reach) && (dy > -reach)
					&& (dy < reach)) {
				near += group->troop_count;
			}
		}
	}
	cairo_assert_ge(near, 500U);
	// Orders keep them marching.
	for (uint32_t i = 0U; i < 10U; i++) {
		vasal_client_game_update(game,
			1.0f / (float)vasal_engine_world_tick_rate);
		vasal_client_stress_update(&stress, game);
	}
	size_t moving = 0U;
	for (size_t v = 0U; v < world->vasal_count; v++) {
		moving += world->vasals[v].groups[0].moving ? 1U : 0U;
	}
	cairo_assert_ge(moving, 5U);
}

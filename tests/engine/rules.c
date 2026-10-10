
/// Tests of the world's hard rules: the size of a centaine, the limits,
/// and that troops stay on the map and off impassable ground.

#include "engine/map.h"
#include "engine/world.h"

#include "cairo.h"

#include <limits.h>

static vasal_engine_map_s _vasal_tests_engine_rules_map;
static vasal_engine_world_s _vasal_tests_engine_rules_world;

// A 60 by 60 map of grass, with a wall of water at x = 30 open at the
// bottom, and a world with one vasal at (10, 10).
static vasal_engine_world_s* _vasal_tests_engine_rules_start(void) {
	vasal_engine_map_s* const map = &_vasal_tests_engine_rules_map;
	(void)vasal_engine_map_clear(map, 60U, 60U);
	const vasal_engine_map_terrain_s water = {
		.name = "water", .color = 0U, .speed = 0U, .passable = false
	};
	map->terrains[1] = water;
	map->terrain_count = 2U;
	for (size_t y = 0U; y < 50U; y++) {
		map->tiles[(y * 60U) + 30U] = 1U;
	}
	vasal_engine_world_s* const world = &_vasal_tests_engine_rules_world;
	vasal_engine_world_start(world, map);
	const vasal_engine_map_point_s spawn = { 10U, 10U };
	(void)vasal_engine_world_add_vasal(world, spawn);
	return world;
}

// Whether every troop of the vasal stands on the map and on passable
// ground.
static bool_t _vasal_tests_engine_rules_placed(
		const vasal_engine_world_s* const world) {
	const vasal_engine_map_s* const map = world->map;
	const int32_t precision = (int32_t)vasal_engine_map_precision;
	for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
		const vasal_engine_world_group_s* const group =
			&world->vasals[0].groups[g];
		for (size_t t = 0U; t < group->troop_count; t++) {
			const vasal_engine_world_position_s at = group->troops[t].position;
			if ((at.x < 0) || (at.y < 0)
					|| (at.x >= ((int32_t)map->width * precision))
					|| (at.y >= ((int32_t)map->height * precision))) {
				return false;
			}
			if (!vasal_engine_map_terrain(map, (uint16_t)(at.x / precision),
					(uint16_t)(at.y / precision))->passable) {
				return false;
			}
		}
	}
	return true;
}

// Orders the vasal's full vintaines to a tile and checks every tick that
// they stay placed.
static bool_t _vasal_tests_engine_rules_march(
		vasal_engine_world_s* const world, const uint16_t x, const uint16_t y,
		const uint32_t ticks) {
	const vasal_engine_map_point_s to = { x, y };
	(void)vasal_engine_world_order_move(world, 0U, 0x3U, to);
	for (uint32_t i = 0U; i < ticks; i++) {
		vasal_engine_world_tick(world);
		if (!_vasal_tests_engine_rules_placed(world)) {
			return false;
		}
	}
	return true;
}

// spec: centaine.size#1
cairo_test_new(engine_rules, five_vintaines_and_a_centenar) {
	cairo_assert_eq(vasal_engine_world_groups, 6U);
	cairo_assert_eq(vasal_engine_world_centenar, 5U);
	vasal_engine_world_s* const world = _vasal_tests_engine_rules_start();
	cairo_assert_false(vasal_engine_world_add_troop(world, 0U,
		vasal_engine_world_groups, 20U));
}

// spec: centaine.size#2
cairo_test_new(engine_rules, vintaine_holds_twenty) {
	vasal_engine_world_s* const world = _vasal_tests_engine_rules_start();
	for (size_t i = 0U; i < 20U; i++) {
		cairo_assert_true(vasal_engine_world_add_troop(world, 0U, 2U, 20U));
	}
	cairo_assert_false(vasal_engine_world_add_troop(world, 0U, 2U, 20U));
	cairo_assert_eq(world->vasals[0].groups[2].troop_count, 20U);
}

// spec: centaine.size#3
cairo_test_new(engine_rules, one_centenar) {
	vasal_engine_world_s* const world = _vasal_tests_engine_rules_start();
	cairo_assert_true(vasal_engine_world_add_troop(world, 0U,
		vasal_engine_world_centenar, 20U));
	cairo_assert_false(vasal_engine_world_add_troop(world, 0U,
		vasal_engine_world_centenar, 20U));
	cairo_assert_eq(world->vasals[0].groups[5].troop_count, 1U);
}

// spec: simulation.limits#1
cairo_test_new(engine_rules, no_more_than_the_limits) {
	vasal_engine_world_s* const world = _vasal_tests_engine_rules_start();
	const vasal_engine_map_point_s spawn = { 12U, 12U };
	for (size_t v = 1U; v < vasal_engine_world_vasals_max; v++) {
		cairo_assert_eq(vasal_engine_world_add_vasal(world, spawn), v);
	}
	cairo_assert_eq(vasal_engine_world_add_vasal(world, spawn),
		vasal_engine_world_vasals_max);
	cairo_assert_eq(world->vasal_count, vasal_engine_world_vasals_max);
	cairo_assert_false(vasal_engine_world_add_troop(world,
		vasal_engine_world_vasals_max, 0U, 20U));
	cairo_assert_eq(vasal_engine_world_vasals_max
		* ((5U * vasal_engine_world_troops) + 1U), 10100U);
	const vasal_engine_map_point_s water = { 30U, 5U };
	const uint32_t next_id = world->next_id;
	cairo_assert_eq(vasal_engine_world_add_vasal(world, water),
		vasal_engine_world_vasals_max);
	cairo_assert_eq(world->next_id, next_id);
}

// spec: map.space#1
cairo_test_new(engine_rules, positions_fit_the_largest_map) {
	cairo_assert_eq(vasal_engine_map_precision, 256U);
	const vasal_engine_world_position_s at = { 0, 0 };
	cairo_assert_ge(sizeof(at.x), 4U);
	cairo_assert_le((int64_t)vasal_engine_map_side_max
		* (int64_t)vasal_engine_map_precision, (int64_t)INT32_MAX);
}

// spec: map.space#2 map.terrain#1
cairo_test_new(engine_rules, troops_stay_on_the_map_and_off_water) {
	vasal_engine_world_s* const world = _vasal_tests_engine_rules_start();
	for (size_t i = 0U; i < 20U; i++) {
		(void)vasal_engine_world_add_troop(world, 0U, 0U, 26U);
		(void)vasal_engine_world_add_troop(world, 0U, 1U, 20U);
	}
	cairo_assert_true(_vasal_tests_engine_rules_march(world, 0U, 0U, 200U));
	cairo_assert_true(_vasal_tests_engine_rules_march(world, 59U, 0U, 1200U));
	cairo_assert_true(_vasal_tests_engine_rules_march(world, 59U, 59U,
		600U));
	cairo_assert_true(_vasal_tests_engine_rules_march(world, 29U, 20U,
		1200U));
}

// spec: map.terrain#3
cairo_test_new(engine_rules, terrain_slows_troops) {
	vasal_engine_world_s* const world = _vasal_tests_engine_rules_start();
	_vasal_tests_engine_rules_map.terrains[0].speed = 60U;
	(void)vasal_engine_world_add_troop(world, 0U, 0U, 20U);
	const int32_t start = world->vasals[0].groups[0].anchor.x;
	const vasal_engine_map_point_s to = { 25U, 10U };
	cairo_assert_true(vasal_engine_world_order_move(world, 0U, 1U, to));
	for (uint32_t i = 0U; i < 50U; i++) {
		vasal_engine_world_tick(world);
	}
	cairo_assert_eq(world->vasals[0].groups[0].anchor.x - start, 600);
}

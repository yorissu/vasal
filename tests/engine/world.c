
/// Tests of the simulated world: orders, movement, pace and formation.

#include "engine/map.h"
#include "engine/world.h"

#include "cairo.h"

static vasal_engine_map_s _vasal_tests_engine_world_map;
static vasal_engine_world_s _vasal_tests_engine_world_world;
static vasal_engine_world_s _vasal_tests_engine_world_twin;

// A 60 by 60 map of grass with a wall of water at x = 30, open at the
// bottom, and a world with one vasal at (10, 10).
static vasal_engine_world_s* _vasal_tests_engine_world_start(
		vasal_engine_world_s* const world) {
	vasal_engine_map_s* const map = &_vasal_tests_engine_world_map;
	(void)vasal_engine_map_clear(map, 60U, 60U);
	const vasal_engine_map_terrain_s water = {
		.name = "water", .color = 0U, .speed = 0U, .passable = false
	};
	map->terrains[1] = water;
	map->terrain_count = 2U;
	for (size_t y = 0U; y < 50U; y++) {
		map->tiles[(y * 60U) + 30U] = 1U;
	}
	vasal_engine_world_start(world, map);
	const vasal_engine_map_point_s spawn = { 10U, 10U };
	(void)vasal_engine_world_add_vasal(world, spawn);
	return world;
}

static void _vasal_tests_engine_world_troops(vasal_engine_world_s* const world,
		const size_t group, const size_t count, const uint16_t speed) {
	for (size_t i = 0U; i < count; i++) {
		(void)vasal_engine_world_add_troop(world, 0U, group, speed);
	}
}

static void _vasal_tests_engine_world_run(vasal_engine_world_s* const world,
		const uint32_t ticks) {
	for (uint32_t i = 0U; i < ticks; i++) {
		vasal_engine_world_tick(world);
	}
}

static int64_t _vasal_tests_engine_world_distance2(
		const vasal_engine_world_position_s a,
		const vasal_engine_world_position_s b) {
	const int64_t dx = (int64_t)a.x - (int64_t)b.x;
	const int64_t dy = (int64_t)a.y - (int64_t)b.y;
	return (dx * dx) + (dy * dy);
}

// spec: simulation.tick#1
cairo_test_new(engine_world, ticks_twenty_times_a_second) {
	cairo_assert_eq(vasal_engine_world_tick_rate, 20U);
}

// spec: map.space#3
cairo_test_new(engine_world, refuses_orders_off_the_map) {
	vasal_engine_world_s* const world =
		_vasal_tests_engine_world_start(&_vasal_tests_engine_world_world);
	_vasal_tests_engine_world_troops(world, 0U, 5U, 20U);
	const vasal_engine_map_point_s off = { 60U, 5U };
	cairo_assert_false(vasal_engine_world_order_move(world, 0U, 1U, off));
	cairo_assert_false(world->vasals[0].groups[0].moving);
}

// spec: vintaines.orders#4
cairo_test_new(engine_world, refuses_orders_to_empty_vintaines) {
	vasal_engine_world_s* const world =
		_vasal_tests_engine_world_start(&_vasal_tests_engine_world_world);
	_vasal_tests_engine_world_troops(world, 0U, 5U, 20U);
	const vasal_engine_map_point_s to = { 20U, 20U };
	cairo_assert_false(vasal_engine_world_order_move(world, 0U, 3U, to));
	cairo_assert_false(world->vasals[0].groups[0].moving);
}

// spec: vintaines.movement#1
cairo_test_new(engine_world, arrives_around_water) {
	vasal_engine_world_s* const world =
		_vasal_tests_engine_world_start(&_vasal_tests_engine_world_world);
	_vasal_tests_engine_world_troops(world, 0U, 20U, 40U);
	const vasal_engine_map_point_s to = { 50U, 10U };
	cairo_assert_true(vasal_engine_world_order_move(world, 0U, 1U, to));
	for (uint32_t i = 0U; i < 2000U; i++) {
		vasal_engine_world_tick(world);
		for (size_t t = 0U; t < 20U; t++) {
			const vasal_engine_world_position_s at =
				world->vasals[0].groups[0].troops[t].position;
			cairo_assert_true(vasal_engine_map_terrain(world->map,
				(uint16_t)(at.x / 256), (uint16_t)(at.y / 256))->passable);
		}
	}
	const vasal_engine_world_group_s* const group =
		&world->vasals[0].groups[0];
	cairo_assert_false(group->moving);
	cairo_assert_eq(group->anchor.x / 256, 50);
	cairo_assert_eq(group->anchor.y / 256, 10);
}

// spec: vintaines.pace#1
cairo_test_new(engine_world, in_step_marches_at_the_slowest_speed) {
	vasal_engine_world_s* const world =
		_vasal_tests_engine_world_start(&_vasal_tests_engine_world_world);
	_vasal_tests_engine_world_troops(world, 0U, 1U, 10U);
	_vasal_tests_engine_world_troops(world, 0U, 1U, 30U);
	const vasal_engine_world_position_s start =
		world->vasals[0].groups[0].anchor;
	const vasal_engine_map_point_s to = { 25U, 10U };
	cairo_assert_true(vasal_engine_world_order_move(world, 0U, 1U, to));
	_vasal_tests_engine_world_run(world, 50U);
	cairo_assert_eq(world->vasals[0].groups[0].anchor.x - start.x, 500);
}

// spec: vintaines.pace#2
cairo_test_new(engine_world, at_the_run_the_fast_go_ahead) {
	vasal_engine_world_s* const world =
		_vasal_tests_engine_world_start(&_vasal_tests_engine_world_world);
	_vasal_tests_engine_world_troops(world, 0U, 1U, 10U);
	_vasal_tests_engine_world_troops(world, 0U, 1U, 30U);
	vasal_engine_world_set_pace(world, 0U, 1U,
		vasal_engine_world_pace_at_the_run);
	const vasal_engine_map_point_s to = { 25U, 10U };
	cairo_assert_true(vasal_engine_world_order_move(world, 0U, 1U, to));
	_vasal_tests_engine_world_run(world, 50U);
	const vasal_engine_world_group_s* const group =
		&world->vasals[0].groups[0];
	cairo_assert_gt(group->troops[1].position.x, group->troops[0].position.x
		+ 512);
	// The slow troop catches up once the fast ones have arrived.
	_vasal_tests_engine_world_run(world, 600U);
	cairo_assert_false(group->moving);
	cairo_assert_lt(_vasal_tests_engine_world_distance2(
		group->troops[0].position, group->anchor), 256 * 256);
}

// spec: vintaines.orders#1
cairo_test_new(engine_world, unselected_vintaines_keep_their_orders) {
	vasal_engine_world_s* const world =
		_vasal_tests_engine_world_start(&_vasal_tests_engine_world_world);
	_vasal_tests_engine_world_troops(world, 0U, 5U, 20U);
	_vasal_tests_engine_world_troops(world, 1U, 5U, 20U);
	const vasal_engine_map_point_s first = { 20U, 20U };
	const vasal_engine_map_point_s second = { 5U, 40U };
	cairo_assert_true(vasal_engine_world_order_move(world, 0U, 3U, first));
	const vasal_engine_map_point_s before =
		world->vasals[0].groups[1].waypoints[0];
	cairo_assert_true(vasal_engine_world_order_move(world, 0U, 1U, second));
	cairo_assert_eq(world->vasals[0].groups[1].waypoints[0].x, before.x);
	cairo_assert_eq(world->vasals[0].groups[1].waypoints[0].y, before.y);
	cairo_assert_true(world->vasals[0].groups[1].moving);
}

// spec: vintaines.orders#3 vintaines.movement#2
cairo_test_new(engine_world, vintaines_form_up_beside_each_other) {
	vasal_engine_world_s* const world =
		_vasal_tests_engine_world_start(&_vasal_tests_engine_world_world);
	_vasal_tests_engine_world_troops(world, 0U, 20U, 40U);
	_vasal_tests_engine_world_troops(world, 1U, 20U, 40U);
	const vasal_engine_map_point_s to = { 15U, 30U };
	cairo_assert_true(vasal_engine_world_order_move(world, 0U, 3U, to));
	_vasal_tests_engine_world_run(world, 1000U);
	const vasal_engine_world_vasal_s* const vasal = &world->vasals[0];
	cairo_assert_ge(_vasal_tests_engine_world_distance2(
		vasal->groups[0].anchor, vasal->groups[1].anchor),
		(int64_t)(5 * 256) * (5 * 256));
	for (size_t i = 0U; i < 20U; i++) {
		for (size_t j = 0U; j < i; j++) {
			cairo_assert_ge(_vasal_tests_engine_world_distance2(
				vasal->groups[0].troops[i].position,
				vasal->groups[0].troops[j].position), 128 * 128);
		}
	}
}

// spec: simulation.ids#1 simulation.determinism#1
cairo_test_new(engine_world, same_orders_same_result_and_unique_ids) {
	vasal_engine_world_s* const worlds[2] = {
		&_vasal_tests_engine_world_world, &_vasal_tests_engine_world_twin
	};
	for (size_t w = 0U; w < 2U; w++) {
		vasal_engine_world_s* const world =
			_vasal_tests_engine_world_start(worlds[w]);
		_vasal_tests_engine_world_troops(world, 0U, 20U, 37U);
		vasal_engine_world_set_pace(world, 0U, 1U,
			vasal_engine_world_pace_at_the_run);
		const vasal_engine_map_point_s to = { 52U, 44U };
		(void)vasal_engine_world_order_move(world, 0U, 1U, to);
		_vasal_tests_engine_world_run(world, 300U);
	}
	const vasal_engine_world_group_s* const a = &worlds[0]->vasals[0].groups[0];
	const vasal_engine_world_group_s* const b = &worlds[1]->vasals[0].groups[0];
	for (size_t i = 0U; i < 20U; i++) {
		cairo_assert_eq(a->troops[i].position.x, b->troops[i].position.x);
		cairo_assert_eq(a->troops[i].position.y, b->troops[i].position.y);
		for (size_t j = 0U; j < i; j++) {
			cairo_assert_neq(a->troops[i].id, a->troops[j].id);
		}
	}
}

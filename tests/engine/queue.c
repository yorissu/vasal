
/// Tests of queued orders: a plain order replaces everything, a queued one
/// comes after what a group already has to do.

#include "engine/map.h"
#include "engine/world.h"

#include "cairo.h"

static vasal_engine_map_s _vasal_tests_engine_queue_map;
static vasal_engine_world_s _vasal_tests_engine_queue_world;

// A 60 by 60 map of grass and a vasal at (10, 10) with 5 troops in each of
// the first two vintaines.
static vasal_engine_world_s* _vasal_tests_engine_queue_start(void) {
	vasal_engine_map_s* const map = &_vasal_tests_engine_queue_map;
	(void)vasal_engine_map_clear(map, 60U, 60U);
	vasal_engine_world_s* const world = &_vasal_tests_engine_queue_world;
	vasal_engine_world_start(world, map, NULL, 1U);
	const vasal_engine_map_point_s spawn = { 10U, 10U };
	(void)vasal_engine_world_add_vasal(world, spawn);
	for (size_t i = 0U; i < 5U; i++) {
		(void)vasal_engine_world_add_troop(world, 0U, 0U, 40U);
		(void)vasal_engine_world_add_troop(world, 0U, 1U, 40U);
	}
	return world;
}

static vasal_engine_world_group_s* _vasal_tests_engine_queue_group(
		vasal_engine_world_s* const world, const size_t group) {
	return &world->vasals[0].groups[group];
}

// Whether a group's anchor is on a tile.
static bool_t _vasal_tests_engine_queue_on(
		const vasal_engine_world_group_s* const group, const uint16_t x,
		const uint16_t y) {
	const int32_t precision = (int32_t)vasal_engine_map_precision;
	return ((group->anchor.x / precision) == (int32_t)x)
		&& ((group->anchor.y / precision) == (int32_t)y);
}

// The tile a group's current order leads to.
static vasal_engine_map_point_s _vasal_tests_engine_queue_end(
		const vasal_engine_world_group_s* const group) {
	return group->waypoints[group->waypoint_count - 1U];
}

// spec: vintaines.queue#1
cairo_test_new(engine_queue, plain_order_replaces) {
	vasal_engine_world_s* const world = _vasal_tests_engine_queue_start();
	const vasal_engine_map_point_s first = { 40U, 10U };
	const vasal_engine_map_point_s second = { 10U, 40U };
	cairo_assert_true(vasal_engine_world_order_move(world, 0U, 1U, first));
	for (size_t i = 0U; i < 10U; i++) {
		vasal_engine_world_tick(world);
	}
	cairo_assert_true(vasal_engine_world_order_move(world, 0U, 1U, second));
	vasal_engine_world_group_s* const group =
		_vasal_tests_engine_queue_group(world, 0U);
	cairo_assert_eq(group->queue_count, 0U);
	cairo_assert_eq(_vasal_tests_engine_queue_end(group).y, 40U);
	for (size_t i = 0U; i < 600U; i++) {
		vasal_engine_world_tick(world);
	}
	cairo_assert_true(_vasal_tests_engine_queue_on(group, 10U, 40U));
}

// spec: vintaines.queue#2
cairo_test_new(engine_queue, queued_order_comes_after) {
	vasal_engine_world_s* const world = _vasal_tests_engine_queue_start();
	const vasal_engine_map_point_s first = { 40U, 10U };
	const vasal_engine_map_point_s second = { 40U, 40U };
	cairo_assert_true(vasal_engine_world_order_move(world, 0U, 1U, first));
	cairo_assert_true(vasal_engine_world_queue_move(world, 0U, 1U, second));
	vasal_engine_world_group_s* const group =
		_vasal_tests_engine_queue_group(world, 0U);
	cairo_assert_eq(group->queue_count, 1U);
	bool_t passed_first = false;
	for (size_t i = 0U; i < 1200U; i++) {
		vasal_engine_world_tick(world);
		passed_first = passed_first
			|| _vasal_tests_engine_queue_on(group, 40U, 10U);
	}
	cairo_assert_true(passed_first);
	cairo_assert_true(_vasal_tests_engine_queue_on(group, 40U, 40U));
	cairo_assert_eq(group->queue_count, 0U);
	cairo_assert_false(group->moving);
}

// spec: vintaines.queue#3
cairo_test_new(engine_queue, plain_order_drops_the_queue) {
	vasal_engine_world_s* const world = _vasal_tests_engine_queue_start();
	const vasal_engine_map_point_s a = { 40U, 10U };
	const vasal_engine_map_point_s b = { 40U, 40U };
	const vasal_engine_map_point_s c = { 5U, 50U };
	cairo_assert_true(vasal_engine_world_order_move(world, 0U, 1U, a));
	cairo_assert_true(vasal_engine_world_queue_move(world, 0U, 1U, b));
	cairo_assert_true(vasal_engine_world_queue_move(world, 0U, 1U, b));
	cairo_assert_true(vasal_engine_world_order_move(world, 0U, 1U, c));
	vasal_engine_world_group_s* const group =
		_vasal_tests_engine_queue_group(world, 0U);
	cairo_assert_eq(group->queue_count, 0U);
	cairo_assert_eq(_vasal_tests_engine_queue_end(group).x, 5U);
	cairo_assert_eq(_vasal_tests_engine_queue_end(group).y, 50U);
}

// spec: vintaines.queue#4
cairo_test_new(engine_queue, every_selected_queues) {
	vasal_engine_world_s* const world = _vasal_tests_engine_queue_start();
	const vasal_engine_map_point_s a = { 40U, 10U };
	const vasal_engine_map_point_s b = { 40U, 40U };
	cairo_assert_true(vasal_engine_world_order_move(world, 0U, 3U, a));
	cairo_assert_true(vasal_engine_world_queue_move(world, 0U, 3U, b));
	cairo_assert_eq(_vasal_tests_engine_queue_group(world, 0U)->queue_count,
		1U);
	cairo_assert_eq(_vasal_tests_engine_queue_group(world, 1U)->queue_count,
		1U);
	cairo_assert_eq(_vasal_tests_engine_queue_group(world, 2U)->queue_count,
		0U);
}

// spec: vintaines.queue#5
cairo_test_new(engine_queue, idle_group_starts_at_once) {
	vasal_engine_world_s* const world = _vasal_tests_engine_queue_start();
	const vasal_engine_map_point_s a = { 40U, 10U };
	cairo_assert_true(vasal_engine_world_queue_move(world, 0U, 1U, a));
	vasal_engine_world_group_s* const group =
		_vasal_tests_engine_queue_group(world, 0U);
	cairo_assert_true(group->moving);
	cairo_assert_eq(group->queue_count, 0U);
	cairo_assert_eq(_vasal_tests_engine_queue_end(group).x, 40U);
}

// spec: vintaines.queue#6
cairo_test_new(engine_queue, full_queue_refuses) {
	vasal_engine_world_s* const world = _vasal_tests_engine_queue_start();
	const vasal_engine_map_point_s a = { 40U, 10U };
	const vasal_engine_map_point_s b = { 40U, 40U };
	cairo_assert_true(vasal_engine_world_order_move(world, 0U, 1U, a));
	for (size_t i = 0U; i < vasal_engine_world_queue_max; i++) {
		cairo_assert_false(vasal_engine_world_queue_full(world, 0U, 1U));
		cairo_assert_true(vasal_engine_world_queue_move(world, 0U, 1U, b));
	}
	cairo_assert_true(vasal_engine_world_queue_full(world, 0U, 1U));
	cairo_assert_true(vasal_engine_world_queue_full(world, 0U, 3U));
	cairo_assert_false(vasal_engine_world_queue_full(world, 0U, 2U));
	cairo_assert_false(vasal_engine_world_queue_move(world, 0U, 3U, b));
	cairo_assert_eq(_vasal_tests_engine_queue_group(world, 0U)->queue_count,
		vasal_engine_world_queue_max);
	cairo_assert_false(_vasal_tests_engine_queue_group(world, 1U)->moving);
}

// spec: vintaines.queue#8
cairo_test_new(engine_queue, pace_leaves_the_queue) {
	vasal_engine_world_s* const world = _vasal_tests_engine_queue_start();
	const vasal_engine_map_point_s a = { 40U, 10U };
	const vasal_engine_map_point_s b = { 40U, 40U };
	cairo_assert_true(vasal_engine_world_order_move(world, 0U, 1U, a));
	cairo_assert_true(vasal_engine_world_queue_move(world, 0U, 1U, b));
	vasal_engine_world_set_pace(world, 0U, 1U,
		vasal_engine_world_pace_at_the_run);
	vasal_engine_world_group_s* const group =
		_vasal_tests_engine_queue_group(world, 0U);
	cairo_assert_eq(group->pace, vasal_engine_world_pace_at_the_run);
	cairo_assert_eq(group->queue_count, 1U);
	cairo_assert_true(group->moving);
}

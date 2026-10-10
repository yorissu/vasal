
/// Tests of path finding: around impassable terrain, fastest rather than
/// shortest, and to the nearest reachable point.

#include "engine/map.h"
#include "engine/path.h"

#include "cairo.h"

static vasal_engine_map_s _vasal_tests_engine_path_map;
static vasal_engine_map_point_s
	_vasal_tests_engine_path_points[vasal_engine_path_points_max];

// A map of grass with a few terrain types: 1 water, 2 tall grass.
static vasal_engine_map_s* _vasal_tests_engine_path_grass(const uint16_t side) {
	vasal_engine_map_s* const map = &_vasal_tests_engine_path_map;
	(void)vasal_engine_map_clear(map, side, side);
	const vasal_engine_map_terrain_s water = {
		.name = "water", .color = 0U, .speed = 0U, .passable = false
	};
	const vasal_engine_map_terrain_s tall = {
		.name = "tall grass", .color = 0U, .speed = 25U, .passable = true
	};
	map->terrains[1] = water;
	map->terrains[2] = tall;
	map->terrain_count = 3U;
	return map;
}

static void _vasal_tests_engine_path_set(const uint16_t x, const uint16_t y,
		const uint8_t terrain) {
	vasal_engine_map_s* const map = &_vasal_tests_engine_path_map;
	map->tiles[((size_t)y * map->width) + x] = terrain;
}

// Whether every tile along the path, from the start through each turning
// point, is passable.
static bool_t _vasal_tests_engine_path_passable(
		const vasal_engine_map_point_s from, const size_t count) {
	vasal_engine_map_point_s at = from;
	for (size_t i = 0U; i < count; i++) {
		const vasal_engine_map_point_s to = _vasal_tests_engine_path_points[i];
		while ((at.x != to.x) || (at.y != to.y)) {
			at.x = (uint16_t)(at.x + ((to.x > at.x) ? 1 : 0)
				- ((to.x < at.x) ? 1 : 0));
			at.y = (uint16_t)(at.y + ((to.y > at.y) ? 1 : 0)
				- ((to.y < at.y) ? 1 : 0));
			if (!vasal_engine_map_terrain(&_vasal_tests_engine_path_map, at.x,
					at.y)->passable) {
				return false;
			}
		}
	}
	return true;
}

// spec: vintaines.movement#1
cairo_test_new(engine_path, goes_around_water) {
	(void)_vasal_tests_engine_path_grass(60U);
	for (uint16_t y = 0U; y < 50U; y++) {
		_vasal_tests_engine_path_set(30U, y, 1U);
	}
	vasal_engine_path_prepare(&_vasal_tests_engine_path_map);
	const vasal_engine_map_point_s from = { 10U, 10U };
	const vasal_engine_map_point_s to = { 50U, 10U };
	const size_t count = vasal_engine_path_find(from, to,
		_vasal_tests_engine_path_points, vasal_engine_path_points_max);
	cairo_assert_gt(count, 1U);
	cairo_assert_eq(_vasal_tests_engine_path_points[count - 1U].x, 50U);
	cairo_assert_eq(_vasal_tests_engine_path_points[count - 1U].y, 10U);
	cairo_assert_true(_vasal_tests_engine_path_passable(from, count));
}

// spec: map.terrain#4
cairo_test_new(engine_path, takes_the_faster_way) {
	(void)_vasal_tests_engine_path_grass(64U);
	for (uint16_t y = 0U; y < 40U; y++) {
		for (uint16_t x = 10U; x < 54U; x++) {
			_vasal_tests_engine_path_set(x, y, 2U);
		}
	}
	vasal_engine_path_prepare(&_vasal_tests_engine_path_map);
	const vasal_engine_map_point_s from = { 5U, 20U };
	const vasal_engine_map_point_s to = { 58U, 20U };
	const size_t count = vasal_engine_path_find(from, to,
		_vasal_tests_engine_path_points, vasal_engine_path_points_max);
	cairo_assert_gt(count, 1U);
	uint16_t lowest = 0U;
	for (size_t i = 0U; i < count; i++) {
		if (_vasal_tests_engine_path_points[i].y > lowest) {
			lowest = _vasal_tests_engine_path_points[i].y;
		}
	}
	// Around the tall grass, through the short grass below it.
	cairo_assert_ge(lowest, 40U);
}

// spec: map.terrain#2
cairo_test_new(engine_path, ends_at_the_nearest_reachable_point) {
	(void)_vasal_tests_engine_path_grass(60U);
	for (uint16_t y = 20U; y < 40U; y++) {
		for (uint16_t x = 20U; x < 40U; x++) {
			_vasal_tests_engine_path_set(x, y, 1U);
		}
	}
	vasal_engine_path_prepare(&_vasal_tests_engine_path_map);
	const vasal_engine_map_point_s from = { 5U, 30U };
	const vasal_engine_map_point_s to = { 30U, 30U };
	const size_t count = vasal_engine_path_find(from, to,
		_vasal_tests_engine_path_points, vasal_engine_path_points_max);
	cairo_assert_gt(count, 0U);
	// The lake spans tiles 20 to 39 around the goal (30, 30); the nearest
	// passable tiles are 10 tiles from it, on its east or south shore.
	const vasal_engine_map_point_s end =
		_vasal_tests_engine_path_points[count - 1U];
	const int32_t dx = (int32_t)end.x - 30;
	const int32_t dy = (int32_t)end.y - 30;
	cairo_assert_eq((dx * dx) + (dy * dy), 100);
	cairo_assert_true(_vasal_tests_engine_path_passable(from, count));
}

// spec: vintaines.movement#1
cairo_test_new(engine_path, refuses_an_impassable_start) {
	(void)_vasal_tests_engine_path_grass(20U);
	_vasal_tests_engine_path_set(3U, 3U, 1U);
	vasal_engine_path_prepare(&_vasal_tests_engine_path_map);
	const vasal_engine_map_point_s from = { 3U, 3U };
	const vasal_engine_map_point_s to = { 10U, 10U };
	cairo_assert_eq(vasal_engine_path_find(from, to,
		_vasal_tests_engine_path_points, vasal_engine_path_points_max), 0U);
}

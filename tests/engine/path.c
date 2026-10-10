
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

// Whether every tile along the path, from the start through each point,
// is passable: each straight line between tile centers sampled every 1/16
// of a tile.
static bool_t _vasal_tests_engine_path_passable(
		const vasal_engine_map_point_s from, const size_t count) {
	vasal_engine_map_point_s at = from;
	for (size_t i = 0U; i < count; i++) {
		const vasal_engine_map_point_s to = _vasal_tests_engine_path_points[i];
		const int32_t ax = ((int32_t)at.x * 16) + 8;
		const int32_t ay = ((int32_t)at.y * 16) + 8;
		const int32_t bx = ((int32_t)to.x * 16) + 8;
		const int32_t by = ((int32_t)to.y * 16) + 8;
		const int32_t dx = (bx > ax) ? (bx - ax) : (ax - bx);
		const int32_t dy = (by > ay) ? (by - ay) : (ay - by);
		const int32_t samples = ((dx > dy) ? dx : dy) + 1;
		for (int32_t s = 0; s <= samples; s++) {
			const int32_t x = (ax + (((bx - ax) * s) / samples)) / 16;
			const int32_t y = (ay + (((by - ay) * s) / samples)) / 16;
			if (!vasal_engine_map_terrain(&_vasal_tests_engine_path_map,
					(uint16_t)x, (uint16_t)y)->passable) {
				return false;
			}
		}
		at = to;
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

// Whether a straight line from a tile's center to another's crosses the
// tiles of a rectangle, sampled every 1/16 of a tile.
static bool_t _vasal_tests_engine_path_crosses(
		const vasal_engine_map_point_s from, const vasal_engine_map_point_s to,
		const uint16_t left, const uint16_t top, const uint16_t right,
		const uint16_t bottom) {
	const int32_t ax = ((int32_t)from.x * 16) + 8;
	const int32_t ay = ((int32_t)from.y * 16) + 8;
	const int32_t bx = ((int32_t)to.x * 16) + 8;
	const int32_t by = ((int32_t)to.y * 16) + 8;
	const int32_t dx = (bx > ax) ? (bx - ax) : (ax - bx);
	const int32_t dy = (by > ay) ? (by - ay) : (ay - by);
	const int32_t samples = ((dx > dy) ? dx : dy) + 1;
	for (int32_t i = 0; i <= samples; i++) {
		const int32_t x = (ax + (((bx - ax) * i) / samples)) / 16;
		const int32_t y = (ay + (((by - ay) * i) / samples)) / 16;
		if ((x >= (int32_t)left) && (x <= (int32_t)right)
				&& (y >= (int32_t)top) && (y <= (int32_t)bottom)) {
			return true;
		}
	}
	return false;
}

// Whether every line of the path, from the start on, is clear.
static bool_t _vasal_tests_engine_path_clear(
		const vasal_engine_map_point_s from, const size_t count) {
	const int32_t half = vasal_engine_map_precision / 2;
	vasal_engine_map_point_s at = from;
	for (size_t i = 0U; i < count; i++) {
		const vasal_engine_map_point_s to = _vasal_tests_engine_path_points[i];
		if (!vasal_engine_path_clear(
				((int32_t)at.x * vasal_engine_map_precision) + half,
				((int32_t)at.y * vasal_engine_map_precision) + half,
				((int32_t)to.x * vasal_engine_map_precision) + half,
				((int32_t)to.y * vasal_engine_map_precision) + half)) {
			return false;
		}
		at = to;
	}
	return true;
}

// spec: vintaines.movement#5
cairo_test_new(engine_path, open_ground_is_one_straight_line) {
	(void)_vasal_tests_engine_path_grass(64U);
	vasal_engine_path_prepare(&_vasal_tests_engine_path_map);
	const vasal_engine_map_point_s from = { 3U, 5U };
	const vasal_engine_map_point_s to = { 58U, 40U };
	const size_t count = vasal_engine_path_find(from, to,
		_vasal_tests_engine_path_points, vasal_engine_path_points_max);
	cairo_assert_eq(count, 1U);
	cairo_assert_eq(_vasal_tests_engine_path_points[0].x, 58U);
	cairo_assert_eq(_vasal_tests_engine_path_points[0].y, 40U);
}

// spec: vintaines.movement#5 vintaines.movement#1
cairo_test_new(engine_path, straight_lines_go_through_the_gap) {
	(void)_vasal_tests_engine_path_grass(64U);
	// A lake from x = 24 to 39, with a gap of 3 rows at y = 22 to 24.
	for (uint16_t y = 4U; y < 44U; y++) {
		for (uint16_t x = 24U; x < 40U; x++) {
			if ((y < 22U) || (y > 24U)) {
				_vasal_tests_engine_path_set(x, y, 1U);
			}
		}
	}
	vasal_engine_path_prepare(&_vasal_tests_engine_path_map);
	const vasal_engine_map_point_s from = { 4U, 8U };
	const vasal_engine_map_point_s to = { 60U, 40U };
	const size_t count = vasal_engine_path_find(from, to,
		_vasal_tests_engine_path_points, vasal_engine_path_points_max);
	// Into the gap, through it and out: no tile-by-tile staircase.
	cairo_assert_le(count, 3U);
	cairo_assert_eq(_vasal_tests_engine_path_points[count - 1U].x, 60U);
	cairo_assert_eq(_vasal_tests_engine_path_points[count - 1U].y, 40U);
	cairo_assert_true(_vasal_tests_engine_path_clear(from, count));
	cairo_assert_true(_vasal_tests_engine_path_passable(from, count));
}

// spec: vintaines.movement#5
cairo_test_new(engine_path, straight_lines_never_cut_a_corner) {
	(void)_vasal_tests_engine_path_grass(32U);
	// Two lakes touching at one corner, between (15, 15) and (16, 16).
	for (uint16_t y = 4U; y < 16U; y++) {
		for (uint16_t x = 4U; x < 16U; x++) {
			_vasal_tests_engine_path_set(x, y, 1U);
			_vasal_tests_engine_path_set((uint16_t)(x + 12U),
				(uint16_t)(y + 12U), 1U);
		}
	}
	vasal_engine_path_prepare(&_vasal_tests_engine_path_map);
	const int32_t precision = vasal_engine_map_precision;
	cairo_assert_false(vasal_engine_path_clear(15 * precision,
		17 * precision, 17 * precision, 15 * precision));
	cairo_assert_true(vasal_engine_path_clear(1 * precision, 1 * precision,
		30 * precision, 1 * precision));
	const vasal_engine_map_point_s from = { 14U, 17U };
	const vasal_engine_map_point_s to = { 17U, 14U };
	const size_t count = vasal_engine_path_find(from, to,
		_vasal_tests_engine_path_points, vasal_engine_path_points_max);
	cairo_assert_gt(count, 1U);
	cairo_assert_true(_vasal_tests_engine_path_clear(from, count));
}

// spec: map.terrain#8
cairo_test_new(engine_path, straight_lines_keep_out_of_slow_ground) {
	(void)_vasal_tests_engine_path_grass(64U);
	for (uint16_t y = 10U; y < 38U; y++) {
		for (uint16_t x = 26U; x < 38U; x++) {
			_vasal_tests_engine_path_set(x, y, 2U);
		}
	}
	vasal_engine_path_prepare(&_vasal_tests_engine_path_map);
	const vasal_engine_map_point_s from = { 4U, 24U };
	const vasal_engine_map_point_s to = { 60U, 24U };
	const size_t count = vasal_engine_path_find(from, to,
		_vasal_tests_engine_path_points, vasal_engine_path_points_max);
	cairo_assert_gt(count, 1U);
	cairo_assert_le(count, 4U);
	vasal_engine_map_point_s at = from;
	for (size_t i = 0U; i < count; i++) {
		// Not through the inside of the patch; brushing its edge may be
		// faster than going wide.
		cairo_assert_false(_vasal_tests_engine_path_crosses(at,
			_vasal_tests_engine_path_points[i], 27U, 11U, 36U, 36U));
		at = _vasal_tests_engine_path_points[i];
	}
}

// spec: map.terrain#7
cairo_test_new(engine_path, finds_a_road_regions_away) {
	(void)_vasal_tests_engine_path_grass(64U);
	const vasal_engine_map_terrain_s road = {
		.name = "road", .color = 0U, .speed = 100U, .passable = true
	};
	_vasal_tests_engine_path_map.terrains[3] = road;
	_vasal_tests_engine_path_map.terrain_count = 4U;
	// Tall grass everywhere, and a road down from the start, along y = 40
	// (two regions away) and up to the goal.
	for (uint16_t y = 0U; y < 64U; y++) {
		for (uint16_t x = 0U; x < 64U; x++) {
			_vasal_tests_engine_path_set(x, y, 2U);
		}
	}
	for (uint16_t x = 2U; x < 62U; x++) {
		_vasal_tests_engine_path_set(x, 40U, 3U);
	}
	for (uint16_t y = 6U; y <= 40U; y++) {
		_vasal_tests_engine_path_set(2U, y, 3U);
		_vasal_tests_engine_path_set(61U, y, 3U);
	}
	vasal_engine_path_prepare(&_vasal_tests_engine_path_map);
	const vasal_engine_map_point_s from = { 2U, 6U };
	const vasal_engine_map_point_s to = { 61U, 6U };
	const size_t count = vasal_engine_path_find(from, to,
		_vasal_tests_engine_path_points, vasal_engine_path_points_max);
	cairo_assert_gt(count, 2U);
	uint16_t lowest = 0U;
	for (size_t i = 0U; i < count; i++) {
		if (_vasal_tests_engine_path_points[i].y > lowest) {
			lowest = _vasal_tests_engine_path_points[i].y;
		}
	}
	cairo_assert_eq(lowest, 40U);
}

#define _vasal_tests_engine_path_side 256U
#define _vasal_tests_engine_path_tiles                                        \
	(_vasal_tests_engine_path_side * _vasal_tests_engine_path_side)
#define _vasal_tests_engine_path_open_max (_vasal_tests_engine_path_tiles * 8U)

static uint32_t _vasal_tests_engine_path_random_state = 12345U;
static uint32_t _vasal_tests_engine_path_best[_vasal_tests_engine_path_tiles];
static uint64_t
	_vasal_tests_engine_path_open[_vasal_tests_engine_path_open_max];

// A number from a fixed sequence.
static uint32_t _vasal_tests_engine_path_random(void) {
	_vasal_tests_engine_path_random_state =
		(_vasal_tests_engine_path_random_state * 1103515245U) + 12345U;
	return (_vasal_tests_engine_path_random_state >> 8U) & 0xffffffU;
}

static uint32_t _vasal_tests_engine_path_cost(const int32_t x,
		const int32_t y) {
	if ((x < 0) || (y < 0) || (x >= (int32_t)_vasal_tests_engine_path_side)
			|| (y >= (int32_t)_vasal_tests_engine_path_side)) {
		return 0U;
	}
	return vasal_engine_path_step_cost((uint16_t)x, (uint16_t)y);
}

// The time to walk the best tile-by-tile path between two tiles, in
// steps of 8 directions that cut no corners: each step costs its length
// times the mean step cost of its two tiles, in 1/16 of a tile. Dijkstra
// over the whole map, with a heap of cost << 32 | tile.
static uint64_t _vasal_tests_engine_path_optimum(
		const vasal_engine_map_point_s from,
		const vasal_engine_map_point_s to) {
	static const int32_t dx[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
	static const int32_t dy[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
	const uint32_t side = _vasal_tests_engine_path_side;
	uint64_t* const open = _vasal_tests_engine_path_open;
	for (size_t i = 0U; i < _vasal_tests_engine_path_tiles; i++) {
		_vasal_tests_engine_path_best[i] = 0xffffffffU;
	}
	size_t count = 1U;
	open[0] = ((uint64_t)from.y * side) + from.x;
	_vasal_tests_engine_path_best[((size_t)from.y * side) + from.x] = 0U;
	const uint32_t goal = ((uint32_t)to.y * side) + to.x;
	while (0U < count) {
		const uint64_t top = open[0];
		count--;
		size_t at = 0U;
		const uint64_t last = open[count];
		for (size_t child = 1U; child < count; child = (at * 2U) + 1U) {
			if (((child + 1U) < count) && (open[child + 1U] < open[child])) {
				child++;
			}
			if (open[child] >= last) {
				break;
			}
			open[at] = open[child];
			at = child;
		}
		open[at] = last;
		const uint32_t tile = (uint32_t)(top & 0xffffffffU);
		const uint32_t cost = (uint32_t)(top >> 32U);
		if (tile == goal) {
			return cost;
		}
		if (cost > _vasal_tests_engine_path_best[tile]) {
			continue;
		}
		const int32_t x = (int32_t)(tile % side);
		const int32_t y = (int32_t)(tile / side);
		for (size_t d = 0U; d < 8U; d++) {
			const int32_t nx = x + dx[d];
			const int32_t ny = y + dy[d];
			const bool_t diagonal = (0 != dx[d]) && (0 != dy[d]);
			if ((0U == _vasal_tests_engine_path_cost(nx, ny)) || (diagonal
					&& ((0U == _vasal_tests_engine_path_cost(nx, y))
						|| (0U == _vasal_tests_engine_path_cost(x, ny))))) {
				continue;
			}
			const uint32_t sum = _vasal_tests_engine_path_cost(x, y)
				+ _vasal_tests_engine_path_cost(nx, ny);
			const uint32_t next = cost
				+ (diagonal ? ((sum * 113U) / 10U) : (sum * 8U));
			const uint32_t index = ((uint32_t)ny * side) + (uint32_t)nx;
			if (next < _vasal_tests_engine_path_best[index]) {
				_vasal_tests_engine_path_best[index] = next;
				size_t up = count;
				count++;
				const uint64_t entry = ((uint64_t)next << 32U) | index;
				while ((0U < up) && (open[(up - 1U) / 2U] > entry)) {
					open[up] = open[(up - 1U) / 2U];
					up = (up - 1U) / 2U;
				}
				open[up] = entry;
			}
		}
	}
	return 0xffffffffU;
}

// The time to walk a path from its start through its points, sampled
// every 1/16 of a tile, in 1/16 of a tile.
static uint64_t _vasal_tests_engine_path_walk(
		const vasal_engine_map_point_s from, const size_t count) {
	uint64_t time = 0U;
	vasal_engine_map_point_s at = from;
	for (size_t i = 0U; i < count; i++) {
		const vasal_engine_map_point_s to = _vasal_tests_engine_path_points[i];
		const int64_t ax = ((int64_t)at.x * 16) + 8;
		const int64_t ay = ((int64_t)at.y * 16) + 8;
		const int64_t bx = ((int64_t)to.x * 16) + 8;
		const int64_t by = ((int64_t)to.y * 16) + 8;
		const int64_t square = ((bx - ax) * (bx - ax))
			+ ((by - ay) * (by - ay));
		int64_t length = 0;
		while (((length + 1) * (length + 1)) <= square) {
			length++;
		}
		if (0 < length) {
			uint64_t sum = 0U;
			for (int64_t s = 0; s < length; s++) {
				const int64_t x = ax + ((((bx - ax) * s) + ((bx - ax) / 2))
					/ length);
				const int64_t y = ay + ((((by - ay) * s) + ((by - ay) / 2))
					/ length);
				sum += _vasal_tests_engine_path_cost((int32_t)(x / 16),
					(int32_t)(y / 16));
			}
			time += sum;
		}
		at = to;
	}
	return time;
}

// spec: vintaines.movement#4 vintaines.movement#6
cairo_test_new(engine_path, two_levels_lose_little_against_the_best_path) {
	const uint16_t side = (uint16_t)_vasal_tests_engine_path_side;
	vasal_engine_map_s* const map = _vasal_tests_engine_path_grass(side);
	map->terrains[0].speed = 80U;
	// Round lakes and patches of tall grass, from a fixed sequence.
	_vasal_tests_engine_path_random_state = 12345U;
	for (size_t i = 0U; i < 200U; i++) {
		const int32_t cx = (int32_t)(_vasal_tests_engine_path_random() % side);
		const int32_t cy = (int32_t)(_vasal_tests_engine_path_random() % side);
		const int32_t r =
			2 + (int32_t)(_vasal_tests_engine_path_random() % 12U);
		const uint8_t terrain =
			(0U != (_vasal_tests_engine_path_random() % 3U)) ? 1U : 2U;
		for (int32_t y = cy - r; y <= (cy + r); y++) {
			for (int32_t x = cx - r; x <= (cx + r); x++) {
				const bool_t inside = (x >= 0) && (y >= 0)
					&& (x < (int32_t)side) && (y < (int32_t)side)
					&& ((((x - cx) * (x - cx)) + ((y - cy) * (y - cy)))
						<= (r * r));
				if (inside) {
					_vasal_tests_engine_path_set((uint16_t)x, (uint16_t)y,
						terrain);
				}
			}
		}
	}
	vasal_engine_path_prepare(map);
	// The first pair bends far from the coarse path: west of a patch of
	// tall grass, with the coarse path through it.
	vasal_engine_map_point_s from = { 226U, 178U };
	vasal_engine_map_point_s to = { 215U, 227U };
	for (size_t query = 0U; query < 12U; query++) {
		const size_t count = vasal_engine_path_find(from, to,
			_vasal_tests_engine_path_points, vasal_engine_path_points_max);
		cairo_assert_gt(count, 0U);
		const vasal_engine_map_point_s end =
			_vasal_tests_engine_path_points[count - 1U];
		if ((end.x == to.x) && (end.y == to.y)) {
			cairo_assert_true(_vasal_tests_engine_path_clear(from, count));
			const uint64_t best = _vasal_tests_engine_path_optimum(from, to);
			const uint64_t walk = _vasal_tests_engine_path_walk(from, count);
			cairo_assert_le(walk * 100U, best * 105U);
		}
		do {
			from.x = (uint16_t)(_vasal_tests_engine_path_random() % side);
			from.y = (uint16_t)(_vasal_tests_engine_path_random() % side);
		} while (0U == _vasal_tests_engine_path_cost(from.x, from.y));
		do {
			to.x = (uint16_t)(_vasal_tests_engine_path_random() % side);
			to.y = (uint16_t)(_vasal_tests_engine_path_random() % side);
		} while (0U == _vasal_tests_engine_path_cost(to.x, to.y));
	}
}

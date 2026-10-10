
/// Two-level A*. Each region is split into its components: the groups of
/// passable tiles connected within the region, so water or forest that cuts
/// a region in two makes two components. The coarse search runs over
/// components: two are linked when a legal step crosses from one into the
/// other. A component costs, per axis, the average step cost of its
/// cheapest way across from one side to the other, so a road through slow
/// ground makes it cheap along the road only; a component with no way
/// across costs the average step cost of its tiles. The fine search runs
/// only over the tiles of the corridor: the regions of the coarse path and
/// those up to two regions from it, mapped to slots so its buffers stay
/// small. Steps go in 8 directions; a diagonal step needs both tiles
/// beside it passable, so paths never cut corners through water or forest.
/// Last, the path is pulled straight: a turning point is dropped when the
/// straight line that replaces it crosses only passable tiles and takes no
/// longer to walk.

#include "engine/path.h"

#define _vasal_engine_path_columns_max                                        \
	(vasal_engine_map_side_max / vasal_engine_path_region_side)
#define _vasal_engine_path_regions_max                                        \
	(_vasal_engine_path_columns_max * _vasal_engine_path_columns_max)
#define _vasal_engine_path_region_tiles                                       \
	(vasal_engine_path_region_side * vasal_engine_path_region_side)
// The most components a region has; more are merged into the last one.
#define _vasal_engine_path_components 16U
#define _vasal_engine_path_groups_max                                         \
	(_vasal_engine_path_regions_max * _vasal_engine_path_components)
#define _vasal_engine_path_corridor_max 768U
// How many regions on each side of the coarse path the corridor takes in:
// with one, the fine search misses ways that bend away from it further.
#define _vasal_engine_path_margin 2
#define _vasal_engine_path_nodes_max                                          \
	(_vasal_engine_path_corridor_max * _vasal_engine_path_region_tiles)
#define _vasal_engine_path_heap_max (_vasal_engine_path_nodes_max * 2U)
#define _vasal_engine_path_none 0xffffffffU
#define _vasal_engine_path_no_slot 0xffffU
#define _vasal_engine_path_no_component 0xffU
// The component of a passable tile not yet given one.
#define _vasal_engine_path_unassigned 0xfeU
#define _vasal_engine_path_straight 100U
#define _vasal_engine_path_diagonal 141U
// Fractions of a straight line are counted in 1/65536.
#define _vasal_engine_path_whole 65536U

// A node in the open list, by priority (cost so far plus estimate).
typedef struct _vasal_engine_path_entry_s {
	uint32_t priority;
	uint32_t node;
} _vasal_engine_path_entry_s;

// The 8 directions: east, then clockwise.
static const int32_t _vasal_engine_path_dx[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
static const int32_t _vasal_engine_path_dy[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };

static const vasal_engine_map_s* _vasal_engine_path_map = NULL;
static uint32_t _vasal_engine_path_columns = 0U;
static uint32_t _vasal_engine_path_rows = 0U;
// The cheapest straight step on the map, for estimates that never
// overestimate.
static uint32_t _vasal_engine_path_cheapest = 1U;

// The component of every tile, or none when it is impassable, so it also
// answers whether a tile is passable without looking up its terrain.
static uint8_t _vasal_engine_path_tile_component
	[vasal_engine_map_side_max * vasal_engine_map_side_max];

// Per group (a region's component, numbered region * components +
// component): the step cost of crossing it west to east and north to
// south, and per direction a bit for each component of the neighbouring
// region it links to.
static uint32_t _vasal_engine_path_group_cost
	[_vasal_engine_path_groups_max][2];
// Whether a group's cost along an axis is already that of a way across,
// while preparing.
static uint8_t _vasal_engine_path_crossed[_vasal_engine_path_groups_max][2];
static uint16_t _vasal_engine_path_group_links
	[_vasal_engine_path_groups_max][8];

// Coarse search state.
static uint32_t _vasal_engine_path_group_g[_vasal_engine_path_groups_max];
static uint32_t _vasal_engine_path_group_parent
	[_vasal_engine_path_groups_max];
static uint8_t _vasal_engine_path_group_state[_vasal_engine_path_groups_max];
static uint32_t _vasal_engine_path_chain[_vasal_engine_path_groups_max];

// The corridor: which slot each region has, and which region each slot.
static uint16_t _vasal_engine_path_region_slot[_vasal_engine_path_regions_max];
static uint32_t _vasal_engine_path_slot_region[_vasal_engine_path_corridor_max];
static uint32_t _vasal_engine_path_slot_count = 0U;

// Fine search state, per corridor tile.
static uint32_t _vasal_engine_path_g[_vasal_engine_path_nodes_max];
static uint32_t _vasal_engine_path_parent[_vasal_engine_path_nodes_max];
static uint8_t _vasal_engine_path_state[_vasal_engine_path_nodes_max];
// The fine path, from the start on, and the time to walk it up to each of
// its tiles.
static vasal_engine_map_point_s
	_vasal_engine_path_trail[_vasal_engine_path_nodes_max];
static uint64_t _vasal_engine_path_elapsed[_vasal_engine_path_nodes_max];

// The open list, a binary heap shared by both searches.
static _vasal_engine_path_entry_s
	_vasal_engine_path_heap[_vasal_engine_path_heap_max];
static uint32_t _vasal_engine_path_heap_count = 0U;

static bool_t _vasal_engine_path_push(const uint32_t priority,
		const uint32_t node) {
	if (_vasal_engine_path_heap_max <= _vasal_engine_path_heap_count) {
		return false;
	}
	uint32_t at = _vasal_engine_path_heap_count;
	_vasal_engine_path_heap_count++;
	while (0U < at) {
		const uint32_t up = (at - 1U) / 2U;
		if (_vasal_engine_path_heap[up].priority <= priority) {
			break;
		}
		_vasal_engine_path_heap[at] = _vasal_engine_path_heap[up];
		at = up;
	}
	_vasal_engine_path_heap[at].priority = priority;
	_vasal_engine_path_heap[at].node = node;
	return true;
}

static _vasal_engine_path_entry_s _vasal_engine_path_pop(void) {
	const _vasal_engine_path_entry_s top = _vasal_engine_path_heap[0];
	_vasal_engine_path_heap_count--;
	const _vasal_engine_path_entry_s last =
		_vasal_engine_path_heap[_vasal_engine_path_heap_count];
	uint32_t at = 0U;
	bool_t sinking = true;
	while (sinking) {
		const uint32_t left = (at * 2U) + 1U;
		if (left >= _vasal_engine_path_heap_count) {
			sinking = false;
		} else {
			uint32_t child = left;
			if (((left + 1U) < _vasal_engine_path_heap_count)
					&& (_vasal_engine_path_heap[left + 1U].priority
						< _vasal_engine_path_heap[left].priority)) {
				child = left + 1U;
			}
			if (_vasal_engine_path_heap[child].priority < last.priority) {
				_vasal_engine_path_heap[at] = _vasal_engine_path_heap[child];
				at = child;
			} else {
				sinking = false;
			}
		}
	}
	_vasal_engine_path_heap[at] = last;
	return top;
}

static bool_t _vasal_engine_path_passable(const int32_t x, const int32_t y) {
	const vasal_engine_map_s* const map = _vasal_engine_path_map;
	if ((x < 0) || (y < 0) || (x >= (int32_t)map->width)
			|| (y >= (int32_t)map->height)) {
		return false;
	}
	const size_t tile = ((size_t)y * (size_t)map->width) + (size_t)x;
	return _vasal_engine_path_no_component
		!= _vasal_engine_path_tile_component[tile];
}

uint32_t vasal_engine_path_step_cost(const uint16_t x, const uint16_t y) {
	const vasal_engine_map_terrain_s* const terrain =
		vasal_engine_map_terrain(_vasal_engine_path_map, x, y);
	if (!terrain->passable) {
		return 0U;
	}
	return 10000U / (uint32_t)terrain->speed;
}

// Whether a step in direction d from (x, y) is allowed: the tile it lands
// on is passable and, for a diagonal step, both tiles beside it too.
static bool_t _vasal_engine_path_can_step(const int32_t x, const int32_t y,
		const size_t d) {
	const int32_t dx = _vasal_engine_path_dx[d];
	const int32_t dy = _vasal_engine_path_dy[d];
	if (!_vasal_engine_path_passable(x + dx, y + dy)) {
		return false;
	}
	return ((0 == dx) || (0 == dy))
		|| (_vasal_engine_path_passable(x + dx, y)
			&& _vasal_engine_path_passable(x, y + dy));
}

// The region a tile is in.
static uint32_t _vasal_engine_path_region_of(const int32_t x,
		const int32_t y) {
	return (((uint32_t)y / vasal_engine_path_region_side)
			* _vasal_engine_path_columns)
		+ ((uint32_t)x / vasal_engine_path_region_side);
}

// The index of a tile in the map's tile array.
static size_t _vasal_engine_path_tile(const int32_t x, const int32_t y) {
	return ((size_t)y * (size_t)_vasal_engine_path_map->width) + (size_t)x;
}

// A region's column and row.
static int32_t _vasal_engine_path_region_x(const uint32_t region) {
	const uint32_t x = region % _vasal_engine_path_columns;
	return (int32_t)x;
}

static int32_t _vasal_engine_path_region_y(const uint32_t region) {
	const uint32_t y = region / _vasal_engine_path_columns;
	return (int32_t)y;
}

// The length of a step in direction d: 100 straight, 141 diagonal.
static uint32_t _vasal_engine_path_length(const size_t d) {
	uint32_t length = _vasal_engine_path_diagonal;
	if ((0 == _vasal_engine_path_dx[d]) || (0 == _vasal_engine_path_dy[d])) {
		length = _vasal_engine_path_straight;
	}
	return length;
}

// The bit of a component in a link mask.
static uint16_t _vasal_engine_path_bit(const uint32_t component) {
	const uint32_t bit = (uint32_t)1U << component;
	return (uint16_t)bit;
}

// The group (region and component) of a passable tile.
static uint32_t _vasal_engine_path_group_of(const int32_t x,
		const int32_t y) {
	return (_vasal_engine_path_region_of(x, y)
			* _vasal_engine_path_components)
		+ (uint32_t)_vasal_engine_path_tile_component
			[_vasal_engine_path_tile(x, y)];
}

// The bounds of a region's tiles: left and top inclusive, right and bottom
// exclusive.
typedef struct _vasal_engine_path_bounds_s {
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;
} _vasal_engine_path_bounds_s;

// Gives the passable tiles connected to (x, y) within the bounds the
// component, by flood fill.
static void _vasal_engine_path_fill(const int32_t x, const int32_t y,
		const uint8_t component, const _vasal_engine_path_bounds_s* const
			bounds) {
	static vasal_engine_map_point_s queue[_vasal_engine_path_region_tiles];
	_vasal_engine_path_tile_component[_vasal_engine_path_tile(x, y)] =
		component;
	size_t head = 0U;
	size_t tail = 1U;
	queue[0].x = (uint16_t)x;
	queue[0].y = (uint16_t)y;
	while (head < tail) {
		const int32_t qx = (int32_t)queue[head].x;
		const int32_t qy = (int32_t)queue[head].y;
		head++;
		for (size_t d = 0U; d < 8U; d++) {
			const int32_t nx = qx + _vasal_engine_path_dx[d];
			const int32_t ny = qy + _vasal_engine_path_dy[d];
			const bool_t inside = (nx >= bounds->left) && (ny >= bounds->top)
				&& (nx < bounds->right) && (ny < bounds->bottom);
			if (!inside || !_vasal_engine_path_can_step(qx, qy, d)) {
				continue;
			}
			const size_t neighbour = _vasal_engine_path_tile(nx, ny);
			if (_vasal_engine_path_unassigned
					== _vasal_engine_path_tile_component[neighbour]) {
				_vasal_engine_path_tile_component[neighbour] = component;
				queue[tail].x = (uint16_t)nx;
				queue[tail].y = (uint16_t)ny;
				tail++;
			}
		}
	}
}

// The bounds of a region's tiles.
static _vasal_engine_path_bounds_s _vasal_engine_path_region_bounds(
		const uint32_t region) {
	const vasal_engine_map_s* const map = _vasal_engine_path_map;
	const int32_t side = (int32_t)vasal_engine_path_region_side;
	_vasal_engine_path_bounds_s bounds;
	bounds.left = _vasal_engine_path_region_x(region) * side;
	bounds.top = _vasal_engine_path_region_y(region) * side;
	bounds.right = ((bounds.left + side) < (int32_t)map->width)
		? (bounds.left + side) : (int32_t)map->width;
	bounds.bottom = ((bounds.top + side) < (int32_t)map->height)
		? (bounds.top + side) : (int32_t)map->height;
	return bounds;
}

// Gives every passable tile of a region its component.
static void _vasal_engine_path_split(const uint32_t region) {
	const _vasal_engine_path_bounds_s bounds =
		_vasal_engine_path_region_bounds(region);
	uint8_t next = 0U;
	for (int32_t y = bounds.top; y < bounds.bottom; y++) {
		for (int32_t x = bounds.left; x < bounds.right; x++) {
			if (_vasal_engine_path_unassigned
					== _vasal_engine_path_tile_component
						[_vasal_engine_path_tile(x, y)]) {
				_vasal_engine_path_fill(x, y, next, &bounds);
				if ((_vasal_engine_path_components - 1U) > next) {
					next++;
				}
			}
		}
	}
}

// Links a passable tile's group to the groups that legal steps from it
// land in, in neighbouring regions.
static void _vasal_engine_path_link(const int32_t x, const int32_t y,
		const uint32_t group) {
	const int32_t side = (int32_t)vasal_engine_path_region_side;
	for (size_t d = 0U; d < 8U; d++) {
		if (!_vasal_engine_path_can_step(x, y, d)) {
			continue;
		}
		const int32_t nx = x + _vasal_engine_path_dx[d];
		const int32_t ny = y + _vasal_engine_path_dy[d];
		const int32_t rdx = (nx / side) - (x / side);
		const int32_t rdy = (ny / side) - (y / side);
		const uint8_t component = _vasal_engine_path_tile_component
			[_vasal_engine_path_tile(nx, ny)];
		for (size_t e = 0U; e < 8U; e++) {
			if ((_vasal_engine_path_dx[e] == rdx)
					&& (_vasal_engine_path_dy[e] == rdy)) {
				_vasal_engine_path_group_links[group][e] |=
					_vasal_engine_path_bit(component);
			}
		}
	}
}

// The direction of a step, from its offsets.
static size_t _vasal_engine_path_direction(const int32_t dx,
		const int32_t dy) {
	size_t found = 0U;
	for (size_t d = 0U; d < 8U; d++) {
		if ((_vasal_engine_path_dx[d] == dx)
				&& (_vasal_engine_path_dy[d] == dy)) {
			found = d;
		}
	}
	return found;
}

// The tile at a distance along an axis (0: x, 1: y) and across it.
static vasal_engine_map_point_s _vasal_engine_path_on_axis(
		const uint32_t axis, const int32_t along, const int32_t across) {
	vasal_engine_map_point_s tile = { (uint16_t)along, (uint16_t)across };
	if (1U == axis) {
		tile.x = (uint16_t)across;
		tile.y = (uint16_t)along;
	}
	return tile;
}

// No way across, in _vasal_engine_path_cross.
#define _vasal_engine_path_no_way 0xffffffffffffffffU

// A line of tiles across an axis (0: x, 1: y), at a distance along it,
// from low to high across it.
typedef struct _vasal_engine_path_span_s {
	uint32_t axis;
	int32_t  along;
	int32_t  low;
	int32_t  high;
} _vasal_engine_path_span_s;

// The cost of the cheapest way to a tile across a line from the line
// before it, given the cheapest ways to each tile of that line, or no way.
static uint64_t _vasal_engine_path_cross_tile(
		const _vasal_engine_path_span_s* const span, const int32_t across,
		const uint64_t* const before) {
	const vasal_engine_map_point_s tile =
		_vasal_engine_path_on_axis(span->axis, span->along, across);
	const uint64_t cost = vasal_engine_path_step_cost(tile.x, tile.y);
	uint64_t best = _vasal_engine_path_no_way;
	for (int32_t side = -1; side <= 1; side++) {
		const int32_t from = across - side;
		if ((from < span->low) || (from >= span->high)) {
			continue;
		}
		const uint64_t way = before[(size_t)from - (size_t)span->low];
		const vasal_engine_map_point_s back =
			_vasal_engine_path_on_axis(span->axis, span->along - 1, from);
		const size_t d = _vasal_engine_path_direction(
			(int32_t)tile.x - (int32_t)back.x,
			(int32_t)tile.y - (int32_t)back.y);
		if ((_vasal_engine_path_no_way != way)
				&& _vasal_engine_path_can_step((int32_t)back.x,
					(int32_t)back.y, d)) {
			const uint64_t total = way + ((cost
				* _vasal_engine_path_length(d)) / _vasal_engine_path_straight);
			best = (total < best) ? total : best;
		}
	}
	return best;
}

// Sets, for every component of a region, the step cost of its cheapest
// way across along an axis (0: west to east, 1: north to south), counting
// only ways that move forward with every step, leaving the average cost
// already set when it has no such way. A dynamic program over the lines
// across the axis, one after the other.
// spec: map.terrain#4 map.terrain#7
static void _vasal_engine_path_cross(const uint32_t region,
		const uint32_t axis) {
	const _vasal_engine_path_bounds_s bounds =
		_vasal_engine_path_region_bounds(region);
	const int32_t first = (0U == axis) ? bounds.left : bounds.top;
	const int32_t last = (0U == axis) ? bounds.right : bounds.bottom;
	_vasal_engine_path_span_s span;
	span.axis = axis;
	span.low = (0U == axis) ? bounds.top : bounds.left;
	span.high = (0U == axis) ? bounds.bottom : bounds.right;
	const size_t count = (size_t)span.high - (size_t)span.low;
	uint64_t before[vasal_engine_path_region_side];
	uint64_t now[vasal_engine_path_region_side];
	for (size_t i = 0U; i < vasal_engine_path_region_side; i++) {
		before[i] = _vasal_engine_path_no_way;
		now[i] = _vasal_engine_path_no_way;
	}
	for (span.along = first; span.along < last; span.along++) {
		for (size_t i = 0U; i < count; i++) {
			const int32_t across = span.low + (int32_t)i;
			const vasal_engine_map_point_s tile =
				_vasal_engine_path_on_axis(axis, span.along, across);
			now[i] = _vasal_engine_path_no_way;
			if (!_vasal_engine_path_passable((int32_t)tile.x,
					(int32_t)tile.y)) {
				// No way onto water.
			} else if (span.along == first) {
				now[i] = vasal_engine_path_step_cost(tile.x, tile.y);
			} else {
				now[i] = _vasal_engine_path_cross_tile(&span, across, before);
			}
		}
		for (size_t i = 0U; i < count; i++) {
			before[i] = now[i];
		}
	}
	const uint64_t length = (uint64_t)last - (uint64_t)first;
	for (size_t i = 0U; i < count; i++) {
		const vasal_engine_map_point_s tile = _vasal_engine_path_on_axis(axis,
			last - 1, span.low + (int32_t)i);
		if (_vasal_engine_path_no_way == before[i]) {
			continue;
		}
		const uint32_t component = _vasal_engine_path_tile_component
			[_vasal_engine_path_tile((int32_t)tile.x, (int32_t)tile.y)];
		const uint32_t group = (region * _vasal_engine_path_components)
			+ component;
		const uint32_t cost = (uint32_t)(before[i] / length);
		uint32_t* const crossing = &_vasal_engine_path_group_cost[group][axis];
		if ((0U == _vasal_engine_path_crossed[group][axis])
				|| (cost < *crossing)) {
			*crossing = cost;
			_vasal_engine_path_crossed[group][axis] = 1U;
		}
	}
}

// spec: vintaines.movement map.terrain
void vasal_engine_path_prepare(const vasal_engine_map_s* const map) {
	_vasal_engine_path_map = map;
	const uint32_t side = vasal_engine_path_region_side;
	_vasal_engine_path_columns = ((uint32_t)map->width + side - 1U) / side;
	_vasal_engine_path_rows = ((uint32_t)map->height + side - 1U) / side;

	uint32_t fastest = 1U;
	for (size_t i = 0U; i < map->terrain_count; i++) {
		if (map->terrains[i].passable && (map->terrains[i].speed > fastest)) {
			fastest = map->terrains[i].speed;
		}
	}
	_vasal_engine_path_cheapest = 10000U / fastest;

	const size_t tiles = (size_t)map->width * (size_t)map->height;
	for (size_t i = 0U; i < tiles; i++) {
		_vasal_engine_path_tile_component[i] =
			map->terrains[map->tiles[i]].passable
				? _vasal_engine_path_unassigned
				: _vasal_engine_path_no_component;
	}
	const uint32_t regions = _vasal_engine_path_columns
		* _vasal_engine_path_rows;
	for (uint32_t region = 0U; region < regions; region++) {
		_vasal_engine_path_split(region);
	}

	const uint32_t groups = regions * _vasal_engine_path_components;
	static uint64_t sums[_vasal_engine_path_groups_max];
	static uint32_t counts[_vasal_engine_path_groups_max];
	for (uint32_t g = 0U; g < groups; g++) {
		sums[g] = 0U;
		counts[g] = 0U;
		_vasal_engine_path_group_cost[g][0] = 0U;
		_vasal_engine_path_group_cost[g][1] = 0U;
		_vasal_engine_path_crossed[g][0] = 0U;
		_vasal_engine_path_crossed[g][1] = 0U;
		for (size_t d = 0U; d < 8U; d++) {
			_vasal_engine_path_group_links[g][d] = 0U;
		}
	}
	for (int32_t y = 0; y < (int32_t)map->height; y++) {
		for (int32_t x = 0; x < (int32_t)map->width; x++) {
			if (_vasal_engine_path_passable(x, y)) {
				const uint32_t group = _vasal_engine_path_group_of(x, y);
				sums[group] += vasal_engine_path_step_cost((uint16_t)x,
					(uint16_t)y);
				counts[group]++;
				_vasal_engine_path_link(x, y, group);
			}
		}
	}
	for (uint32_t g = 0U; g < groups; g++) {
		if (0U < counts[g]) {
			_vasal_engine_path_group_cost[g][0] =
				(uint32_t)(sums[g] / counts[g]);
			_vasal_engine_path_group_cost[g][1] =
				_vasal_engine_path_group_cost[g][0];
		}
	}
	for (uint32_t region = 0U; region < regions; region++) {
		_vasal_engine_path_cross(region, 0U);
		_vasal_engine_path_cross(region, 1U);
	}
}

// The octile distance between two points, in straight-step units of 100:
// diagonal steps count 141.
static uint32_t _vasal_engine_path_octile(const int32_t dx, const int32_t dy) {
	const uint32_t ax = (uint32_t)((dx < 0) ? -dx : dx);
	const uint32_t ay = (uint32_t)((dy < 0) ? -dy : dy);
	const uint32_t low = (ax < ay) ? ax : ay;
	const uint32_t high = (ax < ay) ? ay : ax;
	return (_vasal_engine_path_straight * (high - low))
		+ (_vasal_engine_path_diagonal * low);
}

// Gives a region a corridor slot, if it has none and there is room.
static void _vasal_engine_path_add_slot(const int32_t rx, const int32_t ry) {
	if ((rx < 0) || (ry < 0) || (rx >= (int32_t)_vasal_engine_path_columns)
			|| (ry >= (int32_t)_vasal_engine_path_rows)
			|| (_vasal_engine_path_corridor_max
				<= _vasal_engine_path_slot_count)) {
		return;
	}
	const uint32_t region = ((uint32_t)ry * _vasal_engine_path_columns)
		+ (uint32_t)rx;
	if (_vasal_engine_path_no_slot != _vasal_engine_path_region_slot[region]) {
		return;
	}
	_vasal_engine_path_region_slot[region] =
		(uint16_t)_vasal_engine_path_slot_count;
	_vasal_engine_path_slot_region[_vasal_engine_path_slot_count] = region;
	_vasal_engine_path_slot_count++;
}

// The goal of a coarse search: the goal group, or none when the goal tile
// is impassable, and the goal region's column and row.
typedef struct _vasal_engine_path_goal_s {
	uint32_t group;
	int32_t  x;
	int32_t  y;
	uint32_t scale;
} _vasal_engine_path_goal_s;

// The step cost of crossing a group in direction d: its cost along the
// axis of d, or the mean of both axes for a diagonal.
static uint32_t _vasal_engine_path_group_step(const uint32_t group,
		const size_t d) {
	const uint32_t* const cost = _vasal_engine_path_group_cost[group];
	uint32_t step = (cost[0] + cost[1]) / 2U;
	if (0 == _vasal_engine_path_dy[d]) {
		step = cost[0];
	} else if (0 == _vasal_engine_path_dx[d]) {
		step = cost[1];
	} else {
		// A diagonal crosses both ways at once.
	}
	return step;
}

// Opens the groups a group links to.
static void _vasal_engine_path_expand_group(const uint32_t group,
		const _vasal_engine_path_goal_s* const goal) {
	const uint32_t region = group / _vasal_engine_path_components;
	const int32_t rx = _vasal_engine_path_region_x(region);
	const int32_t ry = _vasal_engine_path_region_y(region);
	for (size_t d = 0U; d < 8U; d++) {
		const uint16_t links = _vasal_engine_path_group_links[group][d];
		const int32_t nx = rx + _vasal_engine_path_dx[d];
		const int32_t ny = ry + _vasal_engine_path_dy[d];
		const uint32_t first = (((uint32_t)ny * _vasal_engine_path_columns)
			+ (uint32_t)nx) * _vasal_engine_path_components;
		for (uint32_t c = 0U; c < _vasal_engine_path_components; c++) {
			if (0U == (links & _vasal_engine_path_bit(c))) {
				continue;
			}
			const uint32_t next = first + c;
			const uint32_t step = ((_vasal_engine_path_group_step(group, d)
					+ _vasal_engine_path_group_step(next, d)) / 2U)
				* vasal_engine_path_region_side * _vasal_engine_path_length(d)
				/ _vasal_engine_path_straight;
			const uint32_t g = _vasal_engine_path_group_g[group] + step;
			if ((0U == _vasal_engine_path_group_state[next])
					|| ((1U == _vasal_engine_path_group_state[next])
						&& (g < _vasal_engine_path_group_g[next]))) {
				_vasal_engine_path_group_g[next] = g;
				_vasal_engine_path_group_parent[next] = group;
				_vasal_engine_path_group_state[next] = 1U;
				(void)_vasal_engine_path_push(g + (_vasal_engine_path_octile(
					goal->x - nx, goal->y - ny) * goal->scale), next);
			}
		}
	}
}

// Fills the corridor with the regions of the chain of groups ending at
// best, from the start on, then the regions around them, ring by ring out
// to the margin, so that when the corridor is full only its outer edge is
// missing.
// spec: vintaines.movement#6
static void _vasal_engine_path_fill_corridor(const uint32_t best) {
	const uint32_t regions = _vasal_engine_path_columns
		* _vasal_engine_path_rows;
	for (uint32_t r = 0U; r < regions; r++) {
		_vasal_engine_path_region_slot[r] = _vasal_engine_path_no_slot;
	}
	size_t chain = 0U;
	for (uint32_t g = best; _vasal_engine_path_none != g;
			g = _vasal_engine_path_group_parent[g]) {
		_vasal_engine_path_chain[chain] = g / _vasal_engine_path_components;
		chain++;
	}
	_vasal_engine_path_slot_count = 0U;
	for (size_t i = chain; 0U < i; i--) {
		const uint32_t region = _vasal_engine_path_chain[i - 1U];
		_vasal_engine_path_add_slot(_vasal_engine_path_region_x(region),
			_vasal_engine_path_region_y(region));
	}
	for (int32_t ring = 1; ring <= _vasal_engine_path_margin; ring++) {
		for (size_t i = chain; 0U < i; i--) {
			const uint32_t region = _vasal_engine_path_chain[i - 1U];
			const int32_t rx = _vasal_engine_path_region_x(region);
			const int32_t ry = _vasal_engine_path_region_y(region);
			for (int32_t dy = -ring; dy <= ring; dy++) {
				for (int32_t dx = -ring; dx <= ring; dx++) {
					_vasal_engine_path_add_slot(rx + dx, ry + dy);
				}
			}
		}
	}
}

// Finds a way through groups toward the goal tile and fills the corridor
// with the regions along it and their neighbours.
static void _vasal_engine_path_corridor(const uint32_t from,
		const int32_t to_x, const int32_t to_y) {
	const uint32_t groups = _vasal_engine_path_columns
		* _vasal_engine_path_rows * _vasal_engine_path_components;
	for (uint32_t g = 0U; g < groups; g++) {
		_vasal_engine_path_group_state[g] = 0U;
	}
	_vasal_engine_path_goal_s goal;
	goal.group = _vasal_engine_path_passable(to_x, to_y)
		? _vasal_engine_path_group_of(to_x, to_y) : _vasal_engine_path_none;
	goal.x = to_x / (int32_t)vasal_engine_path_region_side;
	goal.y = to_y / (int32_t)vasal_engine_path_region_side;
	goal.scale = vasal_engine_path_region_side * _vasal_engine_path_cheapest
		/ _vasal_engine_path_straight;

	_vasal_engine_path_heap_count = 0U;
	_vasal_engine_path_group_g[from] = 0U;
	_vasal_engine_path_group_parent[from] = _vasal_engine_path_none;
	_vasal_engine_path_group_state[from] = 1U;
	(void)_vasal_engine_path_push(0U, from);
	uint32_t best = from;
	uint32_t best_estimate = _vasal_engine_path_none;
	bool_t searching = true;
	while (searching && (0U < _vasal_engine_path_heap_count)) {
		const uint32_t group = _vasal_engine_path_pop().node;
		if (2U != _vasal_engine_path_group_state[group]) {
			_vasal_engine_path_group_state[group] = 2U;
			const uint32_t region = group / _vasal_engine_path_components;
			const uint32_t estimate = _vasal_engine_path_octile(
				goal.x - _vasal_engine_path_region_x(region),
				goal.y - _vasal_engine_path_region_y(region)) * goal.scale;
			if (estimate < best_estimate) {
				best = group;
				best_estimate = estimate;
			}
			searching = group != goal.group;
			if (searching) {
				_vasal_engine_path_expand_group(group, &goal);
			}
		}
	}
	_vasal_engine_path_fill_corridor(best);
}

// The corridor node of a tile, or none when it is outside the corridor.
static uint32_t _vasal_engine_path_node(const int32_t x, const int32_t y) {
	const uint16_t slot = _vasal_engine_path_region_slot
		[_vasal_engine_path_region_of(x, y)];
	if (_vasal_engine_path_no_slot == slot) {
		return _vasal_engine_path_none;
	}
	return ((uint32_t)slot * _vasal_engine_path_region_tiles)
		+ (((uint32_t)y % vasal_engine_path_region_side)
			* vasal_engine_path_region_side)
		+ ((uint32_t)x % vasal_engine_path_region_side);
}

// The tile of a corridor node.
static vasal_engine_map_point_s _vasal_engine_path_point(const uint32_t node) {
	const uint32_t region = _vasal_engine_path_slot_region
		[node / _vasal_engine_path_region_tiles];
	const uint32_t inner = node % _vasal_engine_path_region_tiles;
	const vasal_engine_map_point_s point = {
		(uint16_t)(((region % _vasal_engine_path_columns)
			* vasal_engine_path_region_side)
			+ (inner % vasal_engine_path_region_side)),
		(uint16_t)(((region / _vasal_engine_path_columns)
			* vasal_engine_path_region_side)
			+ (inner / vasal_engine_path_region_side))
	};
	return point;
}

// The integer square root, rounded down.
static uint64_t _vasal_engine_path_root(const uint64_t value) {
	uint64_t low = 0U;
	uint64_t high = 0xffffffffU;
	while (low < high) {
		const uint64_t middle = (low + high + 1U) / 2U;
		if ((middle * middle) <= value) {
			low = middle;
		} else {
			high = middle - 1U;
		}
	}
	return low;
}

// The distance from a coordinate to the edge of a tile in a direction: the
// far edge for 1, the near edge otherwise.
static int64_t _vasal_engine_path_to_edge(const int32_t at, const int32_t tile,
		const int32_t direction) {
	const int32_t precision = vasal_engine_map_precision;
	int64_t distance = (int64_t)at - ((int64_t)tile * precision);
	if (0 < direction) {
		distance = (((int64_t)tile + 1) * precision) - (int64_t)at;
	}
	return distance;
}

// A straight line followed tile by tile: where it starts, the tile it is
// in, the tile it ends in, which way it goes and how far along each axis,
// the fraction of it (in 1/_vasal_engine_path_whole) where it entered the
// tile it is in, and the time to walk it so far, before scaling by its
// length.
typedef struct _vasal_engine_path_line_s {
	int32_t  from_x;
	int32_t  from_y;
	int32_t  x;
	int32_t  y;
	int32_t  end_x;
	int32_t  end_y;
	int32_t  step_x;
	int32_t  step_y;
	int64_t  span_x;
	int64_t  span_y;
	uint64_t entered;
	uint64_t total;
} _vasal_engine_path_line_s;

// The way from one coordinate to another: 1, -1, or 0 when they are equal.
static int32_t _vasal_engine_path_sign(const int32_t from, const int32_t to) {
	int32_t sign = 0;
	if (to > from) {
		sign = 1;
	} else if (to < from) {
		sign = -1;
	} else {
		// Equal: no way to go.
	}
	return sign;
}

// The fraction of a line, in 1/_vasal_engine_path_whole, at which it has
// run a distance along an axis it spans; all of it when it spans none.
static uint64_t _vasal_engine_path_fraction(const int64_t distance,
		const int64_t span) {
	uint64_t fraction = _vasal_engine_path_whole;
	if (0 < span) {
		fraction = ((uint64_t)distance * _vasal_engine_path_whole)
			/ (uint64_t)span;
	}
	return fraction;
}

// Moves a line into the next tile it crosses, adding the time spent in
// the one it leaves. Returns whether the new tile is passable and, where
// the line passes exactly through a tile corner, both tiles beside it.
static bool_t _vasal_engine_path_advance(
		_vasal_engine_path_line_s* const line) {
	// How far along each axis the line runs from its start to the next
	// edge of the tile it is in.
	const int64_t edge_x =
		_vasal_engine_path_to_edge(line->from_x, line->x, line->step_x);
	const int64_t edge_y =
		_vasal_engine_path_to_edge(line->from_y, line->y, line->step_y);
	// Which edge the line meets first: negative for x, positive for y, 0
	// for both at once, comparing edge_x / span_x with edge_y / span_y.
	int64_t order = 1;
	if (line->x != line->end_x) {
		order = -1;
		if (line->y != line->end_y) {
			order = (edge_x * line->span_y) - (edge_y * line->span_x);
		}
	}
	uint64_t left = _vasal_engine_path_fraction(edge_y, line->span_y);
	if (order <= 0) {
		left = _vasal_engine_path_fraction(edge_x, line->span_x);
	}
	line->total += (uint64_t)vasal_engine_path_step_cost((uint16_t)line->x,
		(uint16_t)line->y) * (left - line->entered);
	line->entered = left;
	bool_t clear = true;
	if (0 == order) {
		clear = _vasal_engine_path_passable(line->x + line->step_x, line->y)
			&& _vasal_engine_path_passable(line->x, line->y + line->step_y);
		line->x += line->step_x;
		line->y += line->step_y;
	} else if (order < 0) {
		line->x += line->step_x;
	} else {
		line->y += line->step_y;
	}
	return clear && _vasal_engine_path_passable(line->x, line->y);
}

// Follows a straight line between two positions, in
// 1/vasal_engine_map_precision of a tile, through every tile it crosses,
// exactly, in whole numbers. Returns whether they are all passable (at a
// tile corner, both tiles beside it too) and sets the time to walk it:
// each tile's step cost times the length of line in it, in
// 1/vasal_engine_map_precision of a tile.
static bool_t _vasal_engine_path_trace(const int32_t from_x,
		const int32_t from_y, const int32_t to_x, const int32_t to_y,
		uint64_t* const time) {
	const vasal_engine_map_s* const map = _vasal_engine_path_map;
	const int32_t precision = vasal_engine_map_precision;
	const int32_t width = (int32_t)map->width * precision;
	const int32_t height = (int32_t)map->height * precision;
	*time = 0U;
	if ((from_x < 0) || (from_y < 0) || (to_x < 0) || (to_y < 0)
			|| (from_x >= width) || (from_y >= height) || (to_x >= width)
			|| (to_y >= height)) {
		return false;
	}
	_vasal_engine_path_line_s line;
	line.from_x = from_x;
	line.from_y = from_y;
	line.x = from_x / precision;
	line.y = from_y / precision;
	line.end_x = to_x / precision;
	line.end_y = to_y / precision;
	line.step_x = _vasal_engine_path_sign(from_x, to_x);
	line.step_y = _vasal_engine_path_sign(from_y, to_y);
	line.span_x = (int64_t)line.step_x * ((int64_t)to_x - (int64_t)from_x);
	line.span_y = (int64_t)line.step_y * ((int64_t)to_y - (int64_t)from_y);
	line.entered = 0U;
	line.total = 0U;
	bool_t clear = _vasal_engine_path_passable(line.x, line.y);
	while (clear && ((line.x != line.end_x) || (line.y != line.end_y))) {
		clear = _vasal_engine_path_advance(&line);
	}
	if (clear) {
		line.total += (uint64_t)vasal_engine_path_step_cost(
			(uint16_t)line.x, (uint16_t)line.y)
			* (_vasal_engine_path_whole - line.entered);
		const int64_t square = (line.span_x * line.span_x)
			+ (line.span_y * line.span_y);
		const uint64_t length = _vasal_engine_path_root((uint64_t)square);
		*time = (line.total * length) / _vasal_engine_path_whole;
	}
	return clear;
}

// spec: vintaines.movement#5 vintaines.movement#7
bool_t vasal_engine_path_clear(const int32_t from_x, const int32_t from_y,
	const int32_t to_x, const int32_t to_y) {
	uint64_t time = 0U;
	return (NULL != _vasal_engine_path_map)
		&& _vasal_engine_path_trace(from_x, from_y, to_x, to_y, &time);
}

// The center of a tile, in 1/vasal_engine_map_precision of a tile.
static int32_t _vasal_engine_path_center(const uint16_t coordinate) {
	return ((int32_t)coordinate * vasal_engine_map_precision)
		+ (vasal_engine_map_precision / 2);
}

// Whether the straight line between two tiles of the trail can replace
// the trail between them: it crosses only passable tiles and takes no
// longer to walk.
static bool_t _vasal_engine_path_direct(const size_t from, const size_t to) {
	const vasal_engine_map_point_s a = _vasal_engine_path_trail[from];
	const vasal_engine_map_point_s b = _vasal_engine_path_trail[to];
	uint64_t time = 0U;
	const bool_t clear = _vasal_engine_path_trace(
		_vasal_engine_path_center(a.x), _vasal_engine_path_center(a.y),
		_vasal_engine_path_center(b.x), _vasal_engine_path_center(b.y),
		&time);
	return clear && (time <= (_vasal_engine_path_elapsed[to]
		- _vasal_engine_path_elapsed[from]));
}

// Whether the trail turns at a tile, or ends there.
static bool_t _vasal_engine_path_turns(const size_t at, const size_t length) {
	if ((at + 1U) >= length) {
		return true;
	}
	const vasal_engine_map_point_s before = _vasal_engine_path_trail[at - 1U];
	const vasal_engine_map_point_s point = _vasal_engine_path_trail[at];
	const vasal_engine_map_point_s after = _vasal_engine_path_trail[at + 1U];
	return (((int32_t)point.x - (int32_t)before.x)
			!= ((int32_t)after.x - (int32_t)point.x))
		|| (((int32_t)point.y - (int32_t)before.y)
			!= ((int32_t)after.y - (int32_t)point.y));
}

// The farthest tile of the trail after from that a straight line from it
// can reach in place of the trail: the last of the turning points that
// can be, one after the other, then the farthest tile of the straight run
// to the first one that can't, found by halving.
static size_t _vasal_engine_path_reach(const size_t from,
		const size_t length) {
	size_t reach = from + 1U;
	size_t blocked = length;
	for (size_t at = from + 2U; (at < length) && (length == blocked); at++) {
		if (_vasal_engine_path_turns(at, length)) {
			if (_vasal_engine_path_direct(from, at)) {
				reach = at;
			} else {
				blocked = at;
			}
		}
	}
	while ((blocked < length) && ((reach + 1U) < blocked)) {
		const size_t middle = (reach + blocked) / 2U;
		if (_vasal_engine_path_direct(from, middle)) {
			reach = middle;
		} else {
			blocked = middle;
		}
	}
	return reach;
}

// Writes the points of the path ending at node, pulled straight, from the
// start on, and returns how many.
// spec: vintaines.movement#5 map.terrain#8
static size_t _vasal_engine_path_points(const uint32_t end,
		vasal_engine_map_point_s* const points, const size_t capacity) {
	size_t length = 0U;
	for (uint32_t n = end; _vasal_engine_path_none != n;
			n = _vasal_engine_path_parent[n]) {
		_vasal_engine_path_trail[length] = _vasal_engine_path_point(n);
		length++;
	}
	for (size_t i = 0U; i < (length / 2U); i++) {
		const vasal_engine_map_point_s swap = _vasal_engine_path_trail[i];
		_vasal_engine_path_trail[i] =
			_vasal_engine_path_trail[length - 1U - i];
		_vasal_engine_path_trail[length - 1U - i] = swap;
	}
	_vasal_engine_path_elapsed[0] = 0U;
	for (size_t i = 1U; i < length; i++) {
		const vasal_engine_map_point_s a = _vasal_engine_path_trail[i - 1U];
		const vasal_engine_map_point_s b = _vasal_engine_path_trail[i];
		uint64_t time = 0U;
		(void)_vasal_engine_path_trace(_vasal_engine_path_center(a.x),
			_vasal_engine_path_center(a.y), _vasal_engine_path_center(b.x),
			_vasal_engine_path_center(b.y), &time);
		_vasal_engine_path_elapsed[i] = _vasal_engine_path_elapsed[i - 1U]
			+ time;
	}
	size_t count = 0U;
	size_t from = 0U;
	while (((from + 1U) < length) && (count < capacity)) {
		from = _vasal_engine_path_reach(from, length);
		points[count] = _vasal_engine_path_trail[from];
		count++;
	}
	return count;
}

// Opens the tiles a step from a corridor tile reaches. Returns false when
// the open list is full.
static bool_t _vasal_engine_path_expand_tile(const uint32_t node,
		const int32_t to_x, const int32_t to_y) {
	const vasal_engine_map_point_s here = _vasal_engine_path_point(node);
	const int32_t x = (int32_t)here.x;
	const int32_t y = (int32_t)here.y;
	bool_t room = true;
	for (size_t d = 0U; room && (d < 8U); d++) {
		if (!_vasal_engine_path_can_step(x, y, d)) {
			continue;
		}
		const int32_t nx = x + _vasal_engine_path_dx[d];
		const int32_t ny = y + _vasal_engine_path_dy[d];
		const uint32_t next = _vasal_engine_path_node(nx, ny);
		if ((_vasal_engine_path_none == next)
				|| (2U == _vasal_engine_path_state[next])) {
			continue;
		}
		const uint32_t g = _vasal_engine_path_g[node]
			+ (vasal_engine_path_step_cost((uint16_t)nx, (uint16_t)ny)
				* _vasal_engine_path_length(d) / _vasal_engine_path_straight);
		if ((0U == _vasal_engine_path_state[next])
				|| (g < _vasal_engine_path_g[next])) {
			_vasal_engine_path_g[next] = g;
			_vasal_engine_path_parent[next] = node;
			_vasal_engine_path_state[next] = 1U;
			const uint32_t h = _vasal_engine_path_octile(to_x - nx, to_y - ny)
				* _vasal_engine_path_cheapest / _vasal_engine_path_straight;
			room = _vasal_engine_path_push(g + h, next);
		}
	}
	return room;
}

// spec: vintaines.movement#1 map.terrain#2 map.terrain#4
size_t vasal_engine_path_find(const vasal_engine_map_point_s from,
	const vasal_engine_map_point_s to, vasal_engine_map_point_s* const points,
	const size_t capacity) {
	const int32_t from_x = (int32_t)from.x;
	const int32_t from_y = (int32_t)from.y;
	if ((NULL == _vasal_engine_path_map) || (0U == capacity)
			|| !_vasal_engine_path_passable(from_x, from_y)) {
		return 0U;
	}
	const int32_t width = (int32_t)_vasal_engine_path_map->width;
	const int32_t height = (int32_t)_vasal_engine_path_map->height;
	const int32_t to_x = ((int32_t)to.x < width) ? (int32_t)to.x : (width - 1);
	const int32_t to_y = ((int32_t)to.y < height) ? (int32_t)to.y
		: (height - 1);

	_vasal_engine_path_corridor(_vasal_engine_path_group_of(from_x, from_y),
		to_x, to_y);
	const uint32_t nodes = _vasal_engine_path_slot_count
		* _vasal_engine_path_region_tiles;
	for (uint32_t n = 0U; n < nodes; n++) {
		_vasal_engine_path_state[n] = 0U;
	}

	const uint32_t start = _vasal_engine_path_node(from_x, from_y);
	_vasal_engine_path_heap_count = 0U;
	_vasal_engine_path_g[start] = 0U;
	_vasal_engine_path_parent[start] = _vasal_engine_path_none;
	_vasal_engine_path_state[start] = 1U;
	(void)_vasal_engine_path_push(0U, start);
	uint32_t best = start;
	uint32_t best_estimate = _vasal_engine_path_none;
	bool_t searching = true;
	while (searching && (0U < _vasal_engine_path_heap_count)) {
		const uint32_t node = _vasal_engine_path_pop().node;
		if (2U != _vasal_engine_path_state[node]) {
			_vasal_engine_path_state[node] = 2U;
			const vasal_engine_map_point_s here =
				_vasal_engine_path_point(node);
			const int32_t x = (int32_t)here.x;
			const int32_t y = (int32_t)here.y;
			const uint32_t estimate =
				_vasal_engine_path_octile(to_x - x, to_y - y);
			if (estimate < best_estimate) {
				best = node;
				best_estimate = estimate;
			}
			searching = ((x != to_x) || (y != to_y))
				&& _vasal_engine_path_expand_tile(node, to_x, to_y);
		}
	}

	const size_t count = _vasal_engine_path_points(best, points, capacity);
	if (0U == count) {
		points[0] = from;
		return 1U;
	}
	return count;
}

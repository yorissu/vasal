
/// Orders and movement. A moving group's anchor walks its path; each troop
/// walks toward its place in a round formation around the anchor. Speeds
/// are scaled by the terrain under the one moving, so a group bunches up in
/// slow ground. A troop never steps onto an impassable tile.

#include "engine/world.h"

// Places of a group's troops around its anchor, in 1/256 of a tile: a
// sunflower pattern, about a tile apart, in a circle under 3 tiles across.
static const vasal_engine_world_position_s
	_vasal_engine_world_formation[vasal_engine_world_troops] = {
	{ 112, 0 }, { -143, 131 }, { 22, -250 }, { 181, 236 }, { -332, -59 },
	{ 314, -200 }, { -105, 391 }, { -200, -386 }, { 435, 159 },
	{ -452, 187 }, { 218, -466 }, { 161, 514 }, { -486, -281 },
	{ 570, -125 }, { -348, 494 }, { -80, -620 }, { 493, 415 }, { -663, 27 },
	{ 484, -482 }, { -32, 700 }
};

// Spots of several groups ordered to one point, in tiles: the first on the
// point, the others around it, apart enough that their troops don't stack.
static const int32_t _vasal_engine_world_spots[vasal_engine_world_groups][2] = {
	{ 0, 0 }, { 6, 0 }, { -6, 0 }, { 0, 6 }, { 0, -6 }, { 6, 6 }
};

// The integer square root, rounded down.
static uint32_t _vasal_engine_world_root(const uint64_t value) {
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
	return (uint32_t)low;
}

// The distance between two positions.
static uint32_t _vasal_engine_world_distance(
		const vasal_engine_world_position_s from,
		const vasal_engine_world_position_s to) {
	const int64_t dx = (int64_t)to.x - (int64_t)from.x;
	const int64_t dy = (int64_t)to.y - (int64_t)from.y;
	const int64_t square = (dx * dx) + (dy * dy);
	return _vasal_engine_world_root((uint64_t)square);
}

// The tile a position is on.
static vasal_engine_map_point_s _vasal_engine_world_tile(
		const vasal_engine_world_position_s position) {
	const int32_t x = position.x / vasal_engine_map_precision;
	const int32_t y = position.y / vasal_engine_map_precision;
	const vasal_engine_map_point_s tile = { (uint16_t)x, (uint16_t)y };
	return tile;
}

// The center of a tile.
static vasal_engine_world_position_s _vasal_engine_world_center(
		const vasal_engine_map_point_s tile) {
	const int32_t half = vasal_engine_map_precision / 2;
	const vasal_engine_world_position_s center = {
		((int32_t)tile.x * vasal_engine_map_precision) + half,
		((int32_t)tile.y * vasal_engine_map_precision) + half
	};
	return center;
}

// The speed, in percent, of the terrain at a position: 0 when it is
// impassable or off the map.
static uint32_t _vasal_engine_world_terrain(
		const vasal_engine_world_s* const world,
		const vasal_engine_world_position_s position) {
	const vasal_engine_map_s* const map = world->map;
	const int32_t width = (int32_t)map->width * vasal_engine_map_precision;
	const int32_t height = (int32_t)map->height * vasal_engine_map_precision;
	if ((position.x < 0) || (position.y < 0) || (position.x >= width)
			|| (position.y >= height)) {
		return 0U;
	}
	const vasal_engine_map_point_s tile = _vasal_engine_world_tile(position);
	const vasal_engine_map_terrain_s* const terrain =
		vasal_engine_map_terrain(map, tile.x, tile.y);
	return terrain->passable ? (uint32_t)terrain->speed : 0U;
}

// Moves a position up to budget toward a target, in a straight line,
// refusing impassable ground: when the straight step is blocked it tries
// along each axis alone. Returns the distance left to the target.
static uint32_t _vasal_engine_world_step(
		const vasal_engine_world_s* const world,
		vasal_engine_world_position_s* const position,
		const vasal_engine_world_position_s target, const uint32_t budget) {
	const int64_t dx = (int64_t)target.x - (int64_t)position->x;
	const int64_t dy = (int64_t)target.y - (int64_t)position->y;
	const uint32_t distance = _vasal_engine_world_distance(*position, target);
	if (0U == distance) {
		return 0U;
	}
	const uint32_t step = (budget < distance) ? budget : distance;
	const vasal_engine_world_position_s straight = {
		position->x + (int32_t)((dx * (int64_t)step) / (int64_t)distance),
		position->y + (int32_t)((dy * (int64_t)step) / (int64_t)distance)
	};
	const vasal_engine_world_position_s along_x = { straight.x, position->y };
	const vasal_engine_world_position_s along_y = { position->x, straight.y };
	if (0U < _vasal_engine_world_terrain(world, straight)) {
		*position = straight;
	} else if (0U < _vasal_engine_world_terrain(world, along_x)) {
		*position = along_x;
	} else if (0U < _vasal_engine_world_terrain(world, along_y)) {
		*position = along_y;
	} else {
		// Blocked on every side toward the target.
	}
	return distance - step;
}

// The slowest and fastest troop of a group.
static void _vasal_engine_world_speeds(
		const vasal_engine_world_group_s* const group,
		uint32_t* const slowest, uint32_t* const fastest) {
	*slowest = 0xffffffffU;
	*fastest = 0U;
	for (size_t i = 0U; i < group->troop_count; i++) {
		const uint32_t speed = group->troops[i].speed;
		*slowest = (speed < *slowest) ? speed : *slowest;
		*fastest = (speed > *fastest) ? speed : *fastest;
	}
}

// Sets a group on its way to a tile.
static void _vasal_engine_world_head_to(vasal_engine_world_group_s* const group,
		const vasal_engine_map_point_s goal) {
	const size_t count = vasal_engine_path_find(
		_vasal_engine_world_tile(group->anchor), goal, group->waypoints,
		vasal_engine_world_waypoints);
	group->waypoint_count = (uint16_t)count;
	group->waypoint_next = 0U;
	group->moving = 0U < count;
}

// Starts a group's next queued order once it is done with its current one.
// spec: vintaines.queue#2
static void _vasal_engine_world_next_order(
		vasal_engine_world_group_s* const group) {
	if (group->moving || (0U == group->queue_count)) {
		return;
	}
	const vasal_engine_map_point_s goal = group->queue[0];
	for (size_t i = 1U; i < group->queue_count; i++) {
		group->queue[i - 1U] = group->queue[i];
	}
	group->queue_count--;
	_vasal_engine_world_head_to(group, goal);
}

// Moves a group's anchor along its path.
static void _vasal_engine_world_move_anchor(
		const vasal_engine_world_s* const world,
		vasal_engine_world_group_s* const group, const uint32_t speed) {
	uint32_t budget = (speed
		* _vasal_engine_world_terrain(world, group->anchor)) / 100U;
	if (0U == budget) {
		// An anchor never rests on impassable ground, but if it does, it
		// leaves at the slowest pace rather than not at all.
		budget = speed / 4U;
	}
	while (group->moving && (0U < budget)) {
		const vasal_engine_world_position_s target = _vasal_engine_world_center(
			group->waypoints[group->waypoint_next]);
		const uint32_t distance =
			_vasal_engine_world_distance(group->anchor, target);
		if (distance <= budget) {
			group->anchor = target;
			budget -= distance;
			group->waypoint_next++;
			group->moving = group->waypoint_next < group->waypoint_count;
		} else {
			const vasal_engine_world_position_s before = group->anchor;
			(void)_vasal_engine_world_step(world, &group->anchor, target,
				budget);
			budget = 0U;
			if ((before.x == group->anchor.x)
					&& (before.y == group->anchor.y)) {
				group->moving = false;
			}
		}
	}
}

// spec: vintaines.pace#1 vintaines.pace#2 vintaines.movement#2
static void _vasal_engine_world_move_group(
		const vasal_engine_world_s* const world,
		vasal_engine_world_group_s* const group) {
	if (0U == group->troop_count) {
		return;
	}
	uint32_t slowest = 0U;
	uint32_t fastest = 0U;
	_vasal_engine_world_speeds(group, &slowest, &fastest);
	const bool_t in_step = vasal_engine_world_pace_in_step == group->pace;
	if (group->moving) {
		_vasal_engine_world_move_anchor(world, group,
			in_step ? slowest : fastest);
	}
	_vasal_engine_world_next_order(group);
	for (size_t i = 0U; i < group->troop_count; i++) {
		vasal_engine_world_troop_s* const troop = &group->troops[i];
		vasal_engine_world_position_s place = {
			group->anchor.x + _vasal_engine_world_formation[i].x,
			group->anchor.y + _vasal_engine_world_formation[i].y
		};
		if (0U == _vasal_engine_world_terrain(world, place)) {
			place = group->anchor;
		}
		const uint32_t speed = in_step ? slowest : (uint32_t)troop->speed;
		uint32_t budget = (speed
			* _vasal_engine_world_terrain(world, troop->position)) / 100U;
		if ((0U == budget) && (0U < speed)) {
			budget = 1U;
		}
		(void)_vasal_engine_world_step(world, &troop->position, place, budget);
	}
}

void vasal_engine_world_start(vasal_engine_world_s* const world,
	const vasal_engine_map_s* const map) {
	world->map = map;
	world->tick = 0U;
	world->next_id = 1U;
	world->vasal_count = 0U;
	vasal_engine_path_prepare(map);
}

// spec: simulation.ids#1
size_t vasal_engine_world_add_vasal(vasal_engine_world_s* const world,
	const vasal_engine_map_point_s at) {
	if ((vasal_engine_world_vasals_max <= world->vasal_count)
			|| !vasal_engine_map_contains(world->map, at.x, at.y)
			|| !vasal_engine_map_terrain(world->map, at.x, at.y)->passable) {
		return vasal_engine_world_vasals_max;
	}
	const size_t index = world->vasal_count;
	vasal_engine_world_vasal_s* const vasal = &world->vasals[index];
	vasal->id = world->next_id;
	world->next_id++;
	for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
		vasal_engine_world_group_s* const group = &vasal->groups[g];
		group->anchor = _vasal_engine_world_center(at);
		group->troop_count = 0U;
		group->waypoint_count = 0U;
		group->waypoint_next = 0U;
		group->queue_count = 0U;
		group->pace = vasal_engine_world_pace_in_step;
		group->moving = false;
	}
	world->vasal_count++;
	return index;
}

bool_t vasal_engine_world_add_troop(vasal_engine_world_s* const world,
	const size_t vasal, const size_t group, const uint16_t speed) {
	if ((vasal >= world->vasal_count)
			|| (group >= vasal_engine_world_groups)) {
		return false;
	}
	vasal_engine_world_group_s* const target =
		&world->vasals[vasal].groups[group];
	size_t capacity = vasal_engine_world_troops;
	if (vasal_engine_world_centenar == group) {
		capacity = 1U;
	}
	if (target->troop_count >= capacity) {
		return false;
	}
	vasal_engine_world_troop_s* const troop =
		&target->troops[target->troop_count];
	troop->position = target->anchor;
	troop->id = world->next_id;
	troop->speed = speed;
	world->next_id++;
	target->troop_count++;
	return true;
}

// A value kept within 0 and highest.
static int32_t _vasal_engine_world_clamp(const int32_t value,
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

// Whether a group in the mask can queue no more orders.
bool_t vasal_engine_world_queue_full(const vasal_engine_world_s* const world,
	const size_t vasal, const uint32_t mask) {
	for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
		const vasal_engine_world_group_s* const group =
			&world->vasals[vasal].groups[g];
		if ((0U != (mask & ((uint32_t)1U << g)))
				&& (vasal_engine_world_queue_max <= group->queue_count)) {
			return true;
		}
	}
	return false;
}

// Gives a move order, replacing the current orders or queued after them.
// spec: map.space#3 vintaines.orders#1 vintaines.orders#3
// spec: vintaines.queue#1 vintaines.queue#3 vintaines.queue#4
// spec: vintaines.queue#5 vintaines.queue#6
static bool_t _vasal_engine_world_order(vasal_engine_world_s* const world,
		const size_t vasal, const uint32_t mask,
		const vasal_engine_map_point_s to, const bool_t queued) {
	const vasal_engine_map_s* const map = world->map;
	if (!vasal_engine_map_contains(map, to.x, to.y)
			|| (queued && vasal_engine_world_queue_full(world, vasal, mask))) {
		return false;
	}
	vasal_engine_world_vasal_s* const owner = &world->vasals[vasal];
	for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
		const bool_t ordered = 0U != (mask & ((uint32_t)1U << g));
		if (ordered && (0U == owner->groups[g].troop_count)) {
			return false;
		}
	}

	size_t spot = 0U;
	for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
		if (0U == (mask & ((uint32_t)1U << g))) {
			continue;
		}
		vasal_engine_world_group_s* const group = &owner->groups[g];
		int32_t x = (int32_t)to.x + _vasal_engine_world_spots[spot][0];
		int32_t y = (int32_t)to.y + _vasal_engine_world_spots[spot][1];
		spot++;
		x = _vasal_engine_world_clamp(x, (int32_t)map->width - 1);
		y = _vasal_engine_world_clamp(y, (int32_t)map->height - 1);
		const vasal_engine_map_point_s goal = { (uint16_t)x, (uint16_t)y };
		if (queued && (group->moving || (0U < group->queue_count))) {
			group->queue[group->queue_count] = goal;
			group->queue_count++;
		} else {
			group->queue_count = 0U;
			_vasal_engine_world_head_to(group, goal);
		}
	}
	return true;
}

bool_t vasal_engine_world_order_move(vasal_engine_world_s* const world,
	const size_t vasal, const uint32_t mask,
	const vasal_engine_map_point_s to) {
	return _vasal_engine_world_order(world, vasal, mask, to, false);
}

bool_t vasal_engine_world_queue_move(vasal_engine_world_s* const world,
	const size_t vasal, const uint32_t mask,
	const vasal_engine_map_point_s to) {
	return _vasal_engine_world_order(world, vasal, mask, to, true);
}

void vasal_engine_world_set_pace(vasal_engine_world_s* const world,
	const size_t vasal, const uint32_t mask,
	const vasal_engine_world_pace_e pace) {
	for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
		if (0U != (mask & ((uint32_t)1U << g))) {
			world->vasals[vasal].groups[g].pace = pace;
		}
	}
}

// spec: simulation.tick#1
void vasal_engine_world_tick(vasal_engine_world_s* const world) {
	for (size_t v = 0U; v < world->vasal_count; v++) {
		for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
			_vasal_engine_world_move_group(world,
				&world->vasals[v].groups[g]);
		}
	}
	world->tick++;
}

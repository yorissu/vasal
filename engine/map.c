
/// Creating a map, reading its tiles, and checking what it needs to be
/// played on.

#include "engine/map.h"

// The standard terrain types, in the order of
// vasal_engine_map_standard_e. Forest and tall grass are slow and hide what
// is around; roads are a little faster than grass.
static const vasal_engine_map_terrain_s
	_vasal_engine_map_standards[vasal_engine_map_standards] = {
	{ .name = "grass", .color = 0x6a9a3cffU, .speed = 100U, .sight = 100U,
		.passable = true },
	{ .name = "short grass", .color = 0x82a845ffU, .speed = 85U,
		.sight = 100U, .passable = true },
	{ .name = "tall grass", .color = 0xa8b04affU, .speed = 50U,
		.sight = 60U, .passable = true },
	{ .name = "forest", .color = 0x2f5a2affU, .speed = 35U, .sight = 40U,
		.passable = true },
	{ .name = "road", .color = 0xb59a6bffU, .speed = 120U, .sight = 100U,
		.passable = true },
	{ .name = "water", .color = 0x3d6fb0ffU, .speed = 0U, .sight = 100U,
		.passable = false }
};

// spec: map.terrain#9 editor.tools#7
void vasal_engine_map_standard_terrains(vasal_engine_map_s* const map) {
	for (size_t i = 0U; i < (size_t)vasal_engine_map_standards; i++) {
		map->terrains[i] = _vasal_engine_map_standards[i];
	}
	map->terrain_count = (uint8_t)vasal_engine_map_standards;
}

// The default terrain of a new map.
static const vasal_engine_map_terrain_s _vasal_engine_map_grass = {
	.name     = "grass",
	.color    = 0x6a9a3cffU,
	.speed    = 100U,
	.sight    = 100U,
	.passable = true
};

bool_t vasal_engine_map_clear(vasal_engine_map_s* const map,
	const uint16_t width, const uint16_t height) {
	if ((0U == width) || (0U == height)
			|| (vasal_engine_map_side_max < width)
			|| (vasal_engine_map_side_max < height)) {
		return false;
	}
	map->name[0] = '\0';
	map->width = width;
	map->height = height;
	map->terrains[0] = _vasal_engine_map_grass;
	map->terrain_count = 1U;
	const size_t tiles = (size_t)width * (size_t)height;
	for (size_t i = 0U; i < tiles; i++) {
		map->tiles[i] = 0U;
	}
	map->settlement_count = 0U;
	map->spawn_count = 0U;
	map->camp_count = 0U;
	map->decoration_count = 0U;
	return true;
}

bool_t vasal_engine_map_contains(const vasal_engine_map_s* const map,
	const uint16_t x, const uint16_t y) {
	return (x < map->width) && (y < map->height);
}

const vasal_engine_map_terrain_s* vasal_engine_map_terrain(
	const vasal_engine_map_s* const map, const uint16_t x,
	const uint16_t y) {
	const size_t index = ((size_t)y * (size_t)map->width) + (size_t)x;
	return &map->terrains[map->tiles[index]];
}

// Adds a finding if there is room, and counts it either way.
static void _vasal_engine_map_find(vasal_engine_map_finding_s* const findings,
		const size_t capacity, size_t* const count,
		const vasal_engine_map_problem_e problem, const uint16_t index) {
	if (*count < capacity) {
		findings[*count].problem = problem;
		findings[*count].index = index;
	}
	(*count)++;
}

// Whether two settlements' capture areas overlap.
static bool_t _vasal_engine_map_overlap(
		const vasal_engine_map_settlement_s* const first,
		const vasal_engine_map_settlement_s* const second) {
	const int64_t dx = (int64_t)first->position.x
		- (int64_t)second->position.x;
	const int64_t dy = (int64_t)first->position.y
		- (int64_t)second->position.y;
	const int64_t reach = (int64_t)first->radius + (int64_t)second->radius;
	return ((dx * dx) + (dy * dy)) < (reach * reach);
}

// spec: map.contents#1 editor.validation#1
size_t vasal_engine_map_check(const vasal_engine_map_s* const map,
	vasal_engine_map_finding_s* const findings, const size_t capacity) {
	size_t count = 0U;
	if (0U == map->settlement_count) {
		_vasal_engine_map_find(findings, capacity, &count,
			vasal_engine_map_problem_no_settlement, 0U);
	}
	if (0U == map->spawn_count) {
		_vasal_engine_map_find(findings, capacity, &count,
			vasal_engine_map_problem_no_spawn, 0U);
	}
	for (uint16_t i = 0U; i < map->settlement_count; i++) {
		const vasal_engine_map_settlement_s* const settlement =
			&map->settlements[i];
		if (!vasal_engine_map_terrain(map, settlement->position.x,
				settlement->position.y)->passable) {
			_vasal_engine_map_find(findings, capacity, &count,
				vasal_engine_map_problem_settlement_impassable, i);
		}
		for (uint16_t j = 0U; j < i; j++) {
			if (_vasal_engine_map_overlap(settlement,
					&map->settlements[j])) {
				_vasal_engine_map_find(findings, capacity, &count,
					vasal_engine_map_problem_capture_overlap, i);
			}
		}
	}
	for (uint16_t i = 0U; i < map->spawn_count; i++) {
		const vasal_engine_map_spawn_s* const spawn = &map->spawns[i];
		if (!vasal_engine_map_terrain(map, spawn->position.x,
				spawn->position.y)->passable) {
			_vasal_engine_map_find(findings, capacity, &count,
				vasal_engine_map_problem_spawn_impassable, i);
		}
	}
	return count;
}

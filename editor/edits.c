
/// Filling marks the area in a bitmap first, going along rows (a span fill
/// with a bounded stack of seeds), so it knows the area's size before it
/// changes anything, then paints the marked tiles.

#include "editor/edits.h"
#include "editor/document.h"

#include <string.h>

#define _vasal_editor_edits_tiles                                             \
	(vasal_engine_map_side_max * vasal_engine_map_side_max)
#define _vasal_editor_edits_seeds 262144U

static uint8_t _vasal_editor_edits_marks[_vasal_editor_edits_tiles / 8U];
static uint32_t _vasal_editor_edits_stack[_vasal_editor_edits_seeds];

uint16_t vasal_editor_edits_count(const vasal_editor_edits_kind_e kind) {
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	uint16_t count = map->decoration_count;
	if (vasal_editor_edits_settlement == kind) {
		count = map->settlement_count;
	} else if (vasal_editor_edits_spawn == kind) {
		count = map->spawn_count;
	} else if (vasal_editor_edits_camp == kind) {
		count = map->camp_count;
	} else {
		// Decorations.
	}
	return count;
}

uint16_t vasal_editor_edits_capacity(const vasal_editor_edits_kind_e kind) {
	static const uint16_t capacities[vasal_editor_edits_kinds] = {
		(uint16_t)vasal_engine_map_settlements_max,
		(uint16_t)vasal_engine_map_spawns_max,
		(uint16_t)vasal_engine_map_camps_max,
		(uint16_t)vasal_engine_map_decorations_max
	};
	return capacities[kind];
}

// The count of a kind, to change it.
static uint16_t* _vasal_editor_edits_counter(
		const vasal_editor_edits_kind_e kind) {
	vasal_engine_map_s* const map = vasal_editor_document_map();
	uint16_t* counter = &map->decoration_count;
	if (vasal_editor_edits_settlement == kind) {
		counter = &map->settlement_count;
	} else if (vasal_editor_edits_spawn == kind) {
		counter = &map->spawn_count;
	} else if (vasal_editor_edits_camp == kind) {
		counter = &map->camp_count;
	} else {
		// Decorations.
	}
	return counter;
}

vasal_engine_map_point_s* vasal_editor_edits_position(
	const vasal_editor_edits_kind_e kind, const uint16_t index) {
	vasal_engine_map_s* const map = vasal_editor_document_map();
	vasal_engine_map_point_s* position = &map->decorations[index].position;
	if (vasal_editor_edits_settlement == kind) {
		position = &map->settlements[index].position;
	} else if (vasal_editor_edits_spawn == kind) {
		position = &map->spawns[index].position;
	} else if (vasal_editor_edits_camp == kind) {
		position = &map->camps[index].position;
	} else {
		// A decoration.
	}
	return position;
}

// The squared distance from a point to the nearest object of a kind, or a
// negative number when it has none; index receives that object's.
static float _vasal_editor_edits_nearest(const vasal_editor_edits_kind_e kind,
		const float x, const float y, uint16_t* const index) {
	float best = -1.0f;
	const uint16_t count = vasal_editor_edits_count(kind);
	for (uint16_t i = 0U; i < count; i++) {
		const vasal_engine_map_point_s* const at =
			vasal_editor_edits_position(kind, i);
		const float dx = ((float)at->x + 0.5f) - x;
		const float dy = ((float)at->y + 0.5f) - y;
		const float distance = (dx * dx) + (dy * dy);
		if ((0.0f > best) || (distance < best)) {
			best = distance;
			*index = i;
		}
	}
	return best;
}

// spec: editor.tools#1
bool_t vasal_editor_edits_pick(const vasal_editor_edits_kind_e kind,
	const float x, const float y, const float reach, uint16_t* const index) {
	uint16_t found = 0U;
	const float distance = _vasal_editor_edits_nearest(kind, x, y, &found);
	if ((0.0f > distance) || (distance > (reach * reach))) {
		return false;
	}
	*index = found;
	return true;
}

// spec: editor.tools#1
bool_t vasal_editor_edits_pick_any(const float x, const float y,
	const float reach, vasal_editor_edits_kind_e* const kind,
	uint16_t* const index) {
	// The order in which kinds win a tie.
	static const vasal_editor_edits_kind_e order[] = {
		vasal_editor_edits_settlement, vasal_editor_edits_camp,
		vasal_editor_edits_spawn, vasal_editor_edits_decoration
	};
	float best = reach * reach;
	bool_t found = false;
	for (size_t k = 0U; k < (sizeof(order) / sizeof(order[0])); k++) {
		uint16_t candidate = 0U;
		const float distance = _vasal_editor_edits_nearest(order[k], x, y,
			&candidate);
		if ((0.0f <= distance) && (distance < best)) {
			best = distance;
			*kind = order[k];
			*index = candidate;
			found = true;
		}
	}
	return found;
}

// spec: editor.tools#1
bool_t vasal_editor_edits_add(const vasal_editor_edits_kind_e kind,
	const vasal_engine_map_point_s at, const vasal_engine_rules_s* const rules,
	const vasal_engine_map_settlement_type_e type, const uint16_t decoration,
	uint16_t* const index) {
	uint16_t* const counter = _vasal_editor_edits_counter(kind);
	if (*counter >= vasal_editor_edits_capacity(kind)) {
		return false;
	}
	vasal_engine_map_s* const map = vasal_editor_document_map();
	const uint16_t added = *counter;
	if (vasal_editor_edits_settlement == kind) {
		vasal_engine_map_settlement_s* const settlement =
			&map->settlements[added];
		(void)memset(settlement->name, 0, sizeof(settlement->name));
		vasal_engine_rules_settlement_defaults(rules, settlement, type);
	} else if (vasal_editor_edits_spawn == kind) {
		map->spawns[added].radius = 6U;
	} else if (vasal_editor_edits_decoration == kind) {
		map->decorations[added].kind = decoration;
	} else {
		// A camp is only its position.
	}
	(*counter)++;
	*vasal_editor_edits_position(kind, added) = at;
	vasal_editor_document_changed();
	*index = added;
	return true;
}

// spec: editor.tools#8
bool_t vasal_editor_edits_duplicate(const vasal_editor_edits_kind_e kind,
	const uint16_t original, const vasal_engine_map_point_s at,
	uint16_t* const index) {
	uint16_t* const counter = _vasal_editor_edits_counter(kind);
	if ((*counter >= vasal_editor_edits_capacity(kind))
			|| (original >= *counter)) {
		return false;
	}
	vasal_engine_map_s* const map = vasal_editor_document_map();
	const uint16_t added = *counter;
	if (vasal_editor_edits_settlement == kind) {
		map->settlements[added] = map->settlements[original];
	} else if (vasal_editor_edits_spawn == kind) {
		map->spawns[added] = map->spawns[original];
	} else if (vasal_editor_edits_camp == kind) {
		map->camps[added] = map->camps[original];
	} else {
		map->decorations[added] = map->decorations[original];
	}
	(*counter)++;
	*vasal_editor_edits_position(kind, added) = at;
	vasal_editor_document_changed();
	*index = added;
	return true;
}

// spec: editor.tools#1
void vasal_editor_edits_remove(const vasal_editor_edits_kind_e kind,
	const uint16_t index) {
	uint16_t* const counter = _vasal_editor_edits_counter(kind);
	if (index >= *counter) {
		return;
	}
	vasal_engine_map_s* const map = vasal_editor_document_map();
	const uint16_t last = (uint16_t)(*counter - 1U);
	if (vasal_editor_edits_settlement == kind) {
		map->settlements[index] = map->settlements[last];
	} else if (vasal_editor_edits_spawn == kind) {
		map->spawns[index] = map->spawns[last];
	} else if (vasal_editor_edits_camp == kind) {
		map->camps[index] = map->camps[last];
	} else {
		map->decorations[index] = map->decorations[last];
	}
	(*counter)--;
	vasal_editor_document_changed();
}

// spec: editor.tools#4
void vasal_editor_edits_stamp(const vasal_editor_edits_brush_s brush,
	const int32_t x, const int32_t y, const uint8_t terrain) {
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	const int32_t r = brush.radius;
	// A disc takes the tiles whose middles lie within the radius and a
	// half, so small discs look round.
	const int32_t reach = (r * r) + r;
	for (int32_t dy = -r; dy <= r; dy++) {
		for (int32_t dx = -r; dx <= r; dx++) {
			const int32_t px = x + dx;
			const int32_t py = y + dy;
			const bool_t inside = (px >= 0) && (py >= 0)
				&& (px < (int32_t)map->width) && (py < (int32_t)map->height);
			const bool_t covered = brush.square
				|| (((dx * dx) + (dy * dy)) <= reach);
			if (inside && covered) {
				vasal_editor_document_paint((uint16_t)px, (uint16_t)py,
					terrain);
			}
		}
	}
}

static int32_t _vasal_editor_edits_absolute(const int32_t value) {
	return (0 > value) ? -value : value;
}

// spec: editor.tools#8
void vasal_editor_edits_line(const vasal_editor_edits_brush_s brush,
	const int32_t from_x, const int32_t from_y, const int32_t to_x,
	const int32_t to_y, const uint8_t terrain) {
	// Bresenham's line, the brush stamped at every step.
	const int32_t dx = _vasal_editor_edits_absolute(to_x - from_x);
	const int32_t dy = -_vasal_editor_edits_absolute(to_y - from_y);
	int32_t step_x = -1;
	int32_t step_y = -1;
	if (from_x < to_x) {
		step_x = 1;
	}
	if (from_y < to_y) {
		step_y = 1;
	}
	int32_t error = dx + dy;
	int32_t x = from_x;
	int32_t y = from_y;
	for (;;) {
		vasal_editor_edits_stamp(brush, x, y, terrain);
		if ((x == to_x) && (y == to_y)) {
			break;
		}
		const int32_t twice = 2 * error;
		if (twice >= dy) {
			error += dy;
			x += step_x;
		}
		if (twice <= dx) {
			error += dx;
			y += step_y;
		}
	}
}

static bool_t _vasal_editor_edits_marked(const uint32_t tile) {
	return 0U != (_vasal_editor_edits_marks[tile / 8U]
		& (uint8_t)(1U << (tile % 8U)));
}

static void _vasal_editor_edits_mark(const uint32_t tile) {
	_vasal_editor_edits_marks[tile / 8U] |= (uint8_t)(1U << (tile % 8U));
}

// The area being marked: its terrain type, size and rows.
typedef struct _vasal_editor_edits_area_s {
	uint8_t  terrain;
	size_t   tiles;
	size_t   seeds;
	uint32_t top;
	uint32_t bottom;
	bool_t   overflow;
} _vasal_editor_edits_area_s;

static bool_t _vasal_editor_edits_joins(const vasal_engine_map_s* const map,
		const _vasal_editor_edits_area_s* const area, const uint32_t tile) {
	return (map->tiles[tile] == area->terrain)
		&& !_vasal_editor_edits_marked(tile);
}

static void _vasal_editor_edits_push(_vasal_editor_edits_area_s* const area,
		const uint32_t tile) {
	if (_vasal_editor_edits_seeds <= area->seeds) {
		area->overflow = true;
		return;
	}
	_vasal_editor_edits_stack[area->seeds] = tile;
	area->seeds++;
}

// Pushes a seed for every run of joining tiles of a row from left to
// right, inclusive.
static void _vasal_editor_edits_seed_row(const vasal_engine_map_s* const map,
		_vasal_editor_edits_area_s* const area, const uint32_t row,
		const uint32_t left, const uint32_t right) {
	bool_t running = false;
	for (uint32_t x = left; x <= right; x++) {
		const uint32_t tile = (row * (uint32_t)map->width) + x;
		const bool_t joins = _vasal_editor_edits_joins(map, area, tile);
		if (joins && !running) {
			_vasal_editor_edits_push(area, tile);
		}
		running = joins;
	}
}

// Marks the run of joining tiles through a seed, and seeds the rows above
// and below it.
static void _vasal_editor_edits_span(const vasal_engine_map_s* const map,
		_vasal_editor_edits_area_s* const area, const uint32_t seed) {
	if (!_vasal_editor_edits_joins(map, area, seed)) {
		return;
	}
	const uint32_t width = (uint32_t)map->width;
	const uint32_t row = seed / width;
	uint32_t left = seed % width;
	uint32_t right = left;
	while ((0U < left) && _vasal_editor_edits_joins(map, area,
			(row * width) + left - 1U)) {
		left--;
	}
	while (((right + 1U) < width) && _vasal_editor_edits_joins(map, area,
			(row * width) + right + 1U)) {
		right++;
	}
	for (uint32_t x = left; x <= right; x++) {
		_vasal_editor_edits_mark((row * width) + x);
	}
	area->tiles += ((size_t)right - (size_t)left) + 1U;
	if (row < area->top) {
		area->top = row;
	}
	if (row > area->bottom) {
		area->bottom = row;
	}
	if (0U < row) {
		_vasal_editor_edits_seed_row(map, area, row - 1U, left, right);
	}
	if ((row + 1U) < (uint32_t)map->height) {
		_vasal_editor_edits_seed_row(map, area, row + 1U, left, right);
	}
}

// spec: editor.tools#8
vasal_editor_edits_fill_e vasal_editor_edits_fill(const uint16_t x,
	const uint16_t y, const uint8_t terrain, size_t* const filled) {
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	*filled = 0U;
	if (!vasal_engine_map_contains(map, x, y)) {
		return vasal_editor_edits_fill_same;
	}
	const uint32_t start = ((uint32_t)y * (uint32_t)map->width) + x;
	if (map->tiles[start] == terrain) {
		return vasal_editor_edits_fill_same;
	}
	(void)memset(_vasal_editor_edits_marks, 0,
		sizeof(_vasal_editor_edits_marks));
	_vasal_editor_edits_area_s area = {
		map->tiles[start], 0U, 0U, y, y, false
	};
	_vasal_editor_edits_push(&area, start);
	while ((0U < area.seeds) && !area.overflow) {
		area.seeds--;
		_vasal_editor_edits_span(map, &area,
			_vasal_editor_edits_stack[area.seeds]);
	}
	if (area.overflow || (area.tiles > vasal_editor_document_room())) {
		return vasal_editor_edits_fill_large;
	}
	for (uint32_t row = area.top; row <= area.bottom; row++) {
		for (uint32_t column = 0U; column < (uint32_t)map->width; column++) {
			if (_vasal_editor_edits_marked((row * (uint32_t)map->width)
					+ column)) {
				vasal_editor_document_paint((uint16_t)column, (uint16_t)row,
					terrain);
			}
		}
	}
	*filled = area.tiles;
	return vasal_editor_edits_fill_done;
}

// spec: editor.tools#4
bool_t vasal_editor_edits_add_terrain(uint8_t* const index) {
	vasal_engine_map_s* const map = vasal_editor_document_map();
	if (vasal_engine_map_terrains_max <= map->terrain_count) {
		return false;
	}
	vasal_engine_map_terrain_s* const added =
		&map->terrains[map->terrain_count];
	static const char name[] = "new terrain";
	(void)memset(added->name, 0, sizeof(added->name));
	(void)memcpy(added->name, name, sizeof(name));
	added->color = 0x808080ffU;
	added->speed = 100U;
	added->sight = 100U;
	added->passable = true;
	*index = map->terrain_count;
	map->terrain_count++;
	vasal_editor_document_changed();
	return true;
}

// spec: editor.tools#4
bool_t vasal_editor_edits_remove_terrain(const uint8_t terrain) {
	vasal_engine_map_s* const map = vasal_editor_document_map();
	if ((0U == terrain) || (terrain >= map->terrain_count)) {
		return false;
	}
	for (uint16_t y = 0U; y < map->height; y++) {
		for (uint16_t x = 0U; x < map->width; x++) {
			const uint8_t tile = map->tiles[((size_t)y * map->width) + x];
			if (tile == terrain) {
				vasal_editor_document_paint(x, y, 0U);
			} else if (tile > terrain) {
				vasal_editor_document_paint(x, y, (uint8_t)(tile - 1U));
			} else {
				// Unchanged.
			}
		}
	}
	for (uint8_t i = terrain; (i + 1U) < map->terrain_count; i++) {
		map->terrains[i] = map->terrains[i + 1U];
	}
	map->terrain_count--;
	vasal_editor_document_changed();
	return true;
}

void vasal_editor_edits_census(size_t* const counts) {
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	for (size_t i = 0U; i < map->terrain_count; i++) {
		counts[i] = 0U;
	}
	const size_t tiles = (size_t)map->width * (size_t)map->height;
	for (size_t i = 0U; i < tiles; i++) {
		counts[map->tiles[i]]++;
	}
}


/// The history is a ring of operations. Tile changes go into one shared
/// pool, each operation owning a run of it; when the pool fills up, the
/// history is forgotten and starts again. Everything but the tiles is
/// copied whole into the operation, and undoing swaps it with the map's.

#include "editor/document.h"
#include "engine/mapfile.h"

#include <string.h>

#define _vasal_editor_document_changes_max 524288U

// Everything of a map but its tiles.
typedef struct _vasal_editor_document_objects_s {
	char                          name[vasal_engine_map_name_capacity];
	vasal_engine_map_terrain_s
		terrains[vasal_engine_map_terrains_max];
	vasal_engine_map_settlement_s
		settlements[vasal_engine_map_settlements_max];
	vasal_engine_map_spawn_s      spawns[vasal_engine_map_spawns_max];
	vasal_engine_map_camp_s       camps[vasal_engine_map_camps_max];
	vasal_engine_map_decoration_s
		decorations[vasal_engine_map_decorations_max];
	uint16_t                      settlement_count;
	uint16_t                      spawn_count;
	uint16_t                      camp_count;
	uint16_t                      decoration_count;
	uint8_t                       terrain_count;
} _vasal_editor_document_objects_s;

// A tile changed by an operation.
typedef struct _vasal_editor_document_change_s {
	uint32_t tile;
	uint8_t  before;
	uint8_t  after;
} _vasal_editor_document_change_s;

// An operation: the objects as they were on its other side, and its run
// of tile changes.
typedef struct _vasal_editor_document_operation_s {
	_vasal_editor_document_objects_s objects;
	size_t                           first;
	size_t                           count;
	uint32_t                         key;
} _vasal_editor_document_operation_s;

static vasal_engine_map_s _vasal_editor_document_current;
static vasal_engine_map_s _vasal_editor_document_loaded;
static _vasal_editor_document_operation_s
	_vasal_editor_document_operations[vasal_editor_document_history];
static _vasal_editor_document_change_s
	_vasal_editor_document_changes[_vasal_editor_document_changes_max];
static _vasal_editor_document_objects_s _vasal_editor_document_swap;
// The oldest operation, how many can be undone and redone, how much of the
// change pool is used, and whether an operation is open.
static size_t _vasal_editor_document_oldest = 0U;
static size_t _vasal_editor_document_done = 0U;
static size_t _vasal_editor_document_undone = 0U;
static size_t _vasal_editor_document_used = 0U;
static bool_t _vasal_editor_document_open = false;
static bool_t _vasal_editor_document_merging = false;
static uint32_t _vasal_editor_document_revision = 0U;
static bool_t _vasal_editor_document_dirty = false;

static void _vasal_editor_document_copy_out(
		_vasal_editor_document_objects_s* const objects) {
	const vasal_engine_map_s* const map = &_vasal_editor_document_current;
	(void)memcpy(objects->name, map->name, sizeof(objects->name));
	(void)memcpy(objects->terrains, map->terrains, sizeof(objects->terrains));
	(void)memcpy(objects->settlements, map->settlements,
		sizeof(objects->settlements));
	(void)memcpy(objects->spawns, map->spawns, sizeof(objects->spawns));
	(void)memcpy(objects->camps, map->camps, sizeof(objects->camps));
	(void)memcpy(objects->decorations, map->decorations,
		sizeof(objects->decorations));
	objects->terrain_count = map->terrain_count;
	objects->settlement_count = map->settlement_count;
	objects->spawn_count = map->spawn_count;
	objects->camp_count = map->camp_count;
	objects->decoration_count = map->decoration_count;
}

static void _vasal_editor_document_copy_in(
		const _vasal_editor_document_objects_s* const objects) {
	vasal_engine_map_s* const map = &_vasal_editor_document_current;
	(void)memcpy(map->name, objects->name, sizeof(objects->name));
	(void)memcpy(map->terrains, objects->terrains, sizeof(objects->terrains));
	(void)memcpy(map->settlements, objects->settlements,
		sizeof(objects->settlements));
	(void)memcpy(map->spawns, objects->spawns, sizeof(objects->spawns));
	(void)memcpy(map->camps, objects->camps, sizeof(objects->camps));
	(void)memcpy(map->decorations, objects->decorations,
		sizeof(objects->decorations));
	map->terrain_count = objects->terrain_count;
	map->settlement_count = objects->settlement_count;
	map->spawn_count = objects->spawn_count;
	map->camp_count = objects->camp_count;
	map->decoration_count = objects->decoration_count;
}

// Swaps the map's objects with an operation's.
static void _vasal_editor_document_exchange(
		_vasal_editor_document_operation_s* const operation) {
	_vasal_editor_document_copy_out(&_vasal_editor_document_swap);
	_vasal_editor_document_copy_in(&operation->objects);
	operation->objects = _vasal_editor_document_swap;
}

static _vasal_editor_document_operation_s* _vasal_editor_document_at(
		const size_t index) {
	return &_vasal_editor_document_operations[
		(_vasal_editor_document_oldest + index)
			% vasal_editor_document_history];
}

static void _vasal_editor_document_forget(void) {
	_vasal_editor_document_revision++;
	_vasal_editor_document_oldest = 0U;
	_vasal_editor_document_done = 0U;
	_vasal_editor_document_undone = 0U;
	_vasal_editor_document_used = 0U;
	_vasal_editor_document_open = false;
}

vasal_engine_map_s* vasal_editor_document_map(void) {
	return &_vasal_editor_document_current;
}

uint32_t vasal_editor_document_revision(void) {
	return _vasal_editor_document_revision;
}

// spec: editor.files
bool_t vasal_editor_document_create(const uint16_t width,
	const uint16_t height) {
	if (!vasal_engine_map_clear(&_vasal_editor_document_current, width,
			height)) {
		return false;
	}
	_vasal_editor_document_forget();
	return true;
}

// spec: editor.files
uint32_t vasal_editor_document_open(const uint8_t* const data,
	const size_t size) {
	uint16_t version = 0U;
	const vasal_engine_mapfile_read_e outcome = vasal_engine_mapfile_read(
		data, size, &_vasal_editor_document_loaded, &version);
	if (vasal_engine_mapfile_read_ok == outcome) {
		(void)memcpy(&_vasal_editor_document_current,
			&_vasal_editor_document_loaded,
			sizeof(_vasal_editor_document_current));
		_vasal_editor_document_forget();
	}
	return (uint32_t)outcome;
}

// spec: editor.tools#3
void vasal_editor_document_begin(const uint32_t key) {
	if (_vasal_editor_document_open) {
		return;
	}
	_vasal_editor_document_open = true;
	_vasal_editor_document_dirty = false;
	const bool_t merge = (0U != key) && (0U < _vasal_editor_document_done)
		&& (0U == _vasal_editor_document_undone)
		&& (key == _vasal_editor_document_at(
			_vasal_editor_document_done - 1U)->key);
	_vasal_editor_document_merging = merge;
	if (merge) {
		return;
	}
	// A new operation drops whatever could be redone.
	_vasal_editor_document_undone = 0U;
	if (_vasal_editor_document_used
			>= (_vasal_editor_document_changes_max / 2U)) {
		_vasal_editor_document_forget();
		_vasal_editor_document_open = true;
	}
	if (vasal_editor_document_history == _vasal_editor_document_done) {
		_vasal_editor_document_oldest = (_vasal_editor_document_oldest + 1U)
			% vasal_editor_document_history;
		_vasal_editor_document_done--;
	}
	_vasal_editor_document_operation_s* const operation =
		_vasal_editor_document_at(_vasal_editor_document_done);
	_vasal_editor_document_copy_out(&operation->objects);
	operation->first = _vasal_editor_document_used;
	operation->count = 0U;
	operation->key = key;
	_vasal_editor_document_done++;
}

void vasal_editor_document_changed(void) {
	_vasal_editor_document_dirty = true;
}

void vasal_editor_document_paint(const uint16_t x, const uint16_t y,
	const uint8_t terrain) {
	vasal_engine_map_s* const map = &_vasal_editor_document_current;
	if (!_vasal_editor_document_open || !vasal_engine_map_contains(map, x, y)
			|| (terrain >= map->terrain_count)) {
		return;
	}
	const uint32_t tile = ((uint32_t)y * (uint32_t)map->width) + (uint32_t)x;
	if ((map->tiles[tile] == terrain) || (_vasal_editor_document_changes_max
			<= _vasal_editor_document_used)) {
		return;
	}
	_vasal_editor_document_operation_s* const operation =
		_vasal_editor_document_at(_vasal_editor_document_done - 1U);
	_vasal_editor_document_change_s* const change =
		&_vasal_editor_document_changes[_vasal_editor_document_used];
	change->tile = tile;
	change->before = map->tiles[tile];
	change->after = terrain;
	_vasal_editor_document_used++;
	operation->count++;
	map->tiles[tile] = terrain;
	_vasal_editor_document_revision++;
}

void vasal_editor_document_end(void) {
	if (!_vasal_editor_document_open) {
		return;
	}
	_vasal_editor_document_open = false;
	_vasal_editor_document_revision++;
	if (_vasal_editor_document_merging) {
		return;
	}
	// An operation that changed nothing is dropped.
	const _vasal_editor_document_operation_s* const operation =
		_vasal_editor_document_at(_vasal_editor_document_done - 1U);
	if ((0U == operation->count) && !_vasal_editor_document_dirty) {
		_vasal_editor_document_done--;
	}
}

// spec: editor.tools#3
bool_t vasal_editor_document_undo(void) {
	if (_vasal_editor_document_open || (0U == _vasal_editor_document_done)) {
		return false;
	}
	_vasal_editor_document_done--;
	_vasal_editor_document_undone++;
	_vasal_editor_document_operation_s* const operation =
		_vasal_editor_document_at(_vasal_editor_document_done);
	for (size_t i = operation->count; 0U < i; i--) {
		const _vasal_editor_document_change_s* const change =
			&_vasal_editor_document_changes[operation->first + i - 1U];
		_vasal_editor_document_current.tiles[change->tile] = change->before;
	}
	_vasal_editor_document_exchange(operation);
	_vasal_editor_document_revision++;
	return true;
}

bool_t vasal_editor_document_redo(void) {
	if (_vasal_editor_document_open || (0U == _vasal_editor_document_undone)) {
		return false;
	}
	_vasal_editor_document_operation_s* const operation =
		_vasal_editor_document_at(_vasal_editor_document_done);
	_vasal_editor_document_done++;
	_vasal_editor_document_undone--;
	for (size_t i = 0U; i < operation->count; i++) {
		const _vasal_editor_document_change_s* const change =
			&_vasal_editor_document_changes[operation->first + i];
		_vasal_editor_document_current.tiles[change->tile] = change->after;
	}
	_vasal_editor_document_exchange(operation);
	_vasal_editor_document_revision++;
	return true;
}

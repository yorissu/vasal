
/// Tests of the editor's document: saving and opening, undo and redo.

#include "editor/document.h"
#include "engine/mapfile.h"
#include "engine/rules.h"

#include "cairo.h"

#include <string.h>

static uint8_t _vasal_tests_editor_document_file[vasal_engine_mapfile_capacity];
static vasal_engine_map_s _vasal_tests_editor_document_saved;

// A new 50 by 40 map with a painted tile, a terrain and a settlement.
static vasal_engine_map_s* _vasal_tests_editor_document_sample(void) {
	(void)vasal_editor_document_create(50U, 40U);
	vasal_engine_map_s* const map = vasal_editor_document_map();
	vasal_editor_document_begin(0U);
	const vasal_engine_map_terrain_s road = {
		.name = "road", .color = 0xc0a080ffU, .speed = 100U, .passable = true
	};
	map->terrains[map->terrain_count] = road;
	map->terrain_count++;
	vasal_engine_rules_s rules;
	vasal_engine_rules_standard(&rules);
	vasal_engine_rules_settlement_defaults(&rules, &map->settlements[0],
		vasal_engine_map_settlement_town);
	map->settlements[0].position.x = 20U;
	map->settlements[0].position.y = 15U;
	map->settlement_count = 1U;
	vasal_editor_document_changed();
	vasal_editor_document_end();
	vasal_editor_document_begin(0U);
	vasal_editor_document_paint(3U, 4U, (uint8_t)(map->terrain_count - 1U));
	vasal_editor_document_end();
	return map;
}

// spec: editor.files#1
cairo_test_new(editor_document, saved_and_opened_is_unchanged) {
	const vasal_engine_map_s* const map = _vasal_tests_editor_document_sample();
	const size_t size = vasal_engine_mapfile_write(map,
		_vasal_tests_editor_document_file,
		sizeof(_vasal_tests_editor_document_file));
	cairo_assert_gt(size, 0U);
	_vasal_tests_editor_document_saved = *map;
	cairo_assert_true(vasal_editor_document_create(10U, 10U));
	cairo_assert_eq(vasal_editor_document_open(
		_vasal_tests_editor_document_file, size),
		vasal_engine_mapfile_read_ok);
	const vasal_engine_map_s* const a = &_vasal_tests_editor_document_saved;
	const vasal_engine_map_s* const b = vasal_editor_document_map();
	cairo_assert_eq(b->width, 50U);
	cairo_assert_eq(b->height, 40U);
	cairo_assert_eq(b->terrain_count, a->terrain_count);
	cairo_assert_true(0 == strcmp(b->terrains[b->terrain_count - 1U].name,
		"road"));
	cairo_assert_eq(0, memcmp(a->tiles, b->tiles, 50U * 40U));
	cairo_assert_eq(b->settlement_count, 1U);
	cairo_assert_eq(b->settlements[0].position.x, 20U);
	cairo_assert_eq(b->settlements[0].radius, a->settlements[0].radius);
	cairo_assert_eq(b->settlements[0].flags, a->settlements[0].flags);
}

// spec: editor.files#1
cairo_test_new(editor_document, refused_file_leaves_map) {
	(void)_vasal_tests_editor_document_sample();
	const uint8_t junk[16] = { 1U, 2U, 3U };
	cairo_assert_neq(vasal_editor_document_open(junk, sizeof(junk)),
		vasal_engine_mapfile_read_ok);
	cairo_assert_eq(vasal_editor_document_map()->width, 50U);
	cairo_assert_eq(vasal_editor_document_map()->settlement_count, 1U);
}

// spec: editor.tools#5
cairo_test_new(editor_document, undo_and_redo) {
	const vasal_engine_map_s* const map = _vasal_tests_editor_document_sample();
	const uint8_t painted = map->tiles[(4U * 50U) + 3U];
	cairo_assert_neq(painted, 0U);
	cairo_assert_true(vasal_editor_document_undo());
	cairo_assert_eq(map->tiles[(4U * 50U) + 3U], 0U);
	cairo_assert_eq(map->settlement_count, 1U);
	cairo_assert_true(vasal_editor_document_undo());
	cairo_assert_eq(map->settlement_count, 0U);
	cairo_assert_false(vasal_editor_document_undo());
	cairo_assert_true(vasal_editor_document_redo());
	cairo_assert_true(vasal_editor_document_redo());
	cairo_assert_eq(map->tiles[(4U * 50U) + 3U], painted);
	cairo_assert_eq(map->settlement_count, 1U);
	cairo_assert_false(vasal_editor_document_redo());
}

// spec: editor.tools#5
cairo_test_new(editor_document, empty_operation_is_dropped) {
	(void)_vasal_tests_editor_document_sample();
	vasal_editor_document_begin(0U);
	vasal_editor_document_end();
	cairo_assert_true(vasal_editor_document_undo());
	cairo_assert_true(vasal_editor_document_undo());
	cairo_assert_false(vasal_editor_document_undo());
}

// spec: editor.tools#7 map.terrain#9
cairo_test_new(editor_document, new_maps_have_the_standard_terrains) {
	(void)vasal_editor_document_create(30U, 20U);
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	cairo_assert_eq(map->terrain_count, (uint8_t)vasal_engine_map_standards);
	const vasal_engine_map_terrain_s* const forest =
		&map->terrains[vasal_engine_map_standard_forest];
	cairo_assert_true(0 == strcmp(forest->name, "forest"));
	cairo_assert_true(forest->passable);
	cairo_assert_lt(forest->speed, 100U);
	cairo_assert_lt(forest->sight, 100U);
	cairo_assert_false(
		map->terrains[vasal_engine_map_standard_water].passable);
	// The map itself is grass.
	cairo_assert_eq(map->tiles[0], (uint8_t)vasal_engine_map_standard_grass);
}

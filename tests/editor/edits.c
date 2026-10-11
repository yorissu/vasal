
/// Tests of the editor's map edits: objects, brushes, lines, filling and
/// terrain types.

#include "editor/edits.h"
#include "editor/document.h"

#include "cairo.h"

#include <string.h>

static vasal_engine_rules_s _vasal_tests_editor_edits_rules;

// A new map of grass, the standard terrain types ready.
static vasal_engine_map_s* _vasal_tests_editor_edits_map(const uint16_t width,
		const uint16_t height) {
	(void)vasal_editor_document_create(width, height);
	vasal_engine_rules_standard(&_vasal_tests_editor_edits_rules);
	return vasal_editor_document_map();
}

static uint8_t _vasal_tests_editor_edits_tile(
		const vasal_engine_map_s* const map, const uint16_t x,
		const uint16_t y) {
	return map->tiles[((size_t)y * map->width) + x];
}

// spec: editor.tools#1 editor.tools#8
cairo_test_new(editor_edits, adds_picks_copies_and_removes_objects) {
	(void)_vasal_tests_editor_edits_map(64U, 64U);
	uint16_t index = 99U;
	vasal_editor_document_begin(0U);
	const vasal_engine_map_point_s at = { 10U, 12U };
	cairo_assert_true(vasal_editor_edits_add(vasal_editor_edits_settlement,
		at, &_vasal_tests_editor_edits_rules,
		vasal_engine_map_settlement_castle, 0U, &index));
	cairo_assert_eq(index, 0U);
	const vasal_engine_map_point_s camp = { 30U, 30U };
	cairo_assert_true(vasal_editor_edits_add(vasal_editor_edits_camp, camp,
		&_vasal_tests_editor_edits_rules, vasal_engine_map_settlement_village,
		0U, &index));
	const vasal_engine_map_point_s copy_at = { 40U, 12U };
	cairo_assert_true(vasal_editor_edits_duplicate(
		vasal_editor_edits_settlement, 0U, copy_at, &index));
	vasal_editor_document_end();
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	cairo_assert_eq(map->settlement_count, 2U);
	cairo_assert_eq(index, 1U);
	cairo_assert_eq((uint32_t)map->settlements[1].type,
		(uint32_t)vasal_engine_map_settlement_castle);
	cairo_assert_eq(map->settlements[1].position.x, 40U);

	// Picking: the nearest within reach, of one kind or any.
	cairo_assert_true(vasal_editor_edits_pick(vasal_editor_edits_settlement,
		39.8f, 12.2f, 1.5f, &index));
	cairo_assert_eq(index, 1U);
	cairo_assert_false(vasal_editor_edits_pick(vasal_editor_edits_settlement,
		25.0f, 12.0f, 1.5f, &index));
	vasal_editor_edits_kind_e kind = vasal_editor_edits_spawn;
	cairo_assert_true(vasal_editor_edits_pick_any(30.2f, 30.7f, 1.5f, &kind,
		&index));
	cairo_assert_eq((uint32_t)kind, (uint32_t)vasal_editor_edits_camp);

	// Removing moves the last into the place; undo brings it back.
	vasal_editor_document_begin(0U);
	vasal_editor_edits_remove(vasal_editor_edits_settlement, 0U);
	vasal_editor_document_end();
	cairo_assert_eq(map->settlement_count, 1U);
	cairo_assert_eq(map->settlements[0].position.x, 40U);
	cairo_assert_true(vasal_editor_document_undo());
	cairo_assert_eq(map->settlement_count, 2U);
	cairo_assert_eq(map->settlements[0].position.x, 10U);
}

// spec: editor.tools#4 editor.tools#8
cairo_test_new(editor_edits, brushes_and_lines_paint) {
	const vasal_engine_map_s* const map = _vasal_tests_editor_edits_map(40U,
		40U);
	const uint8_t forest = (uint8_t)vasal_engine_map_standard_forest;
	vasal_editor_document_begin(0U);
	const vasal_editor_edits_brush_s disc = { 2, false };
	vasal_editor_edits_stamp(disc, 10, 10, forest);
	vasal_editor_document_end();
	cairo_assert_eq(_vasal_tests_editor_edits_tile(map, 12U, 10U), forest);
	cairo_assert_eq(_vasal_tests_editor_edits_tile(map, 11U, 11U), forest);
	// A disc leaves its corners.
	cairo_assert_eq(_vasal_tests_editor_edits_tile(map, 12U, 12U), 0U);

	vasal_editor_document_begin(0U);
	const vasal_editor_edits_brush_s square = { 1, true };
	vasal_editor_edits_stamp(square, 0, 0, forest);
	vasal_editor_document_end();
	cairo_assert_eq(_vasal_tests_editor_edits_tile(map, 1U, 1U), forest);

	vasal_editor_document_begin(0U);
	const vasal_editor_edits_brush_s dot = { 0, false };
	const uint8_t road = (uint8_t)vasal_engine_map_standard_road;
	vasal_editor_edits_line(dot, 5, 30, 35, 20, road);
	vasal_editor_document_end();
	cairo_assert_eq(_vasal_tests_editor_edits_tile(map, 5U, 30U), road);
	cairo_assert_eq(_vasal_tests_editor_edits_tile(map, 35U, 20U), road);
	cairo_assert_eq(_vasal_tests_editor_edits_tile(map, 20U, 25U), road);
	// One undo takes the whole line away.
	cairo_assert_true(vasal_editor_document_undo());
	cairo_assert_eq(_vasal_tests_editor_edits_tile(map, 20U, 25U), 0U);
}

// spec: editor.tools#8
cairo_test_new(editor_edits, fills_the_joined_area) {
	vasal_engine_map_s* const map = _vasal_tests_editor_edits_map(30U, 20U);
	const uint8_t water = (uint8_t)vasal_engine_map_standard_water;
	const uint8_t forest = (uint8_t)vasal_engine_map_standard_forest;
	// A wall of water down column 10 splits the map, but for a gap at row
	// 19; then a closed ring of water around (20, 5).
	vasal_editor_document_begin(0U);
	for (uint16_t y = 0U; y < 19U; y++) {
		vasal_editor_document_paint(10U, y, water);
	}
	for (uint16_t i = 0U; i < 5U; i++) {
		vasal_editor_document_paint((uint16_t)(18U + i), 3U, water);
		vasal_editor_document_paint((uint16_t)(18U + i), 7U, water);
		vasal_editor_document_paint(18U, (uint16_t)(3U + i), water);
		vasal_editor_document_paint(22U, (uint16_t)(3U + i), water);
	}
	vasal_editor_document_end();

	size_t filled = 0U;
	vasal_editor_document_begin(0U);
	cairo_assert_eq((uint32_t)vasal_editor_edits_fill(20U, 5U, forest,
		&filled), (uint32_t)vasal_editor_edits_fill_done);
	vasal_editor_document_end();
	cairo_assert_eq(filled, 9U);
	cairo_assert_eq(_vasal_tests_editor_edits_tile(map, 21U, 6U), forest);
	cairo_assert_eq(_vasal_tests_editor_edits_tile(map, 23U, 5U), 0U);

	// The grass outside: everything but the water and the ring's inside,
	// through the gap.
	vasal_editor_document_begin(0U);
	cairo_assert_eq((uint32_t)vasal_editor_edits_fill(0U, 0U, forest,
		&filled), (uint32_t)vasal_editor_edits_fill_done);
	vasal_editor_document_end();
	cairo_assert_eq(filled, (size_t)(30U * 20U) - 19U - 16U - 9U);
	cairo_assert_eq(_vasal_tests_editor_edits_tile(map, 29U, 0U), forest);

	// Filling with the same type changes nothing.
	vasal_editor_document_begin(0U);
	cairo_assert_eq((uint32_t)vasal_editor_edits_fill(0U, 0U, forest,
		&filled), (uint32_t)vasal_editor_edits_fill_same);
	vasal_editor_document_end();
	cairo_assert_eq(filled, 0U);
}

// spec: editor.tools#8
cairo_test_new(editor_edits, refuses_fills_larger_than_an_operation) {
	(void)_vasal_tests_editor_edits_map(2048U, 2048U);
	size_t filled = 0U;
	vasal_editor_document_begin(0U);
	cairo_assert_eq((uint32_t)vasal_editor_edits_fill(5U, 5U, 1U, &filled),
		(uint32_t)vasal_editor_edits_fill_large);
	vasal_editor_document_end();
	cairo_assert_eq(filled, 0U);
	cairo_assert_eq(vasal_editor_document_map()->tiles[0], 0U);
}

// spec: editor.tools#4
cairo_test_new(editor_edits, adds_and_removes_terrain_types) {
	vasal_engine_map_s* const map = _vasal_tests_editor_edits_map(8U, 8U);
	const uint8_t count = map->terrain_count;
	vasal_editor_document_begin(0U);
	vasal_editor_document_paint(1U, 1U, 3U);
	vasal_editor_document_paint(2U, 2U, 4U);
	vasal_editor_document_end();
	uint8_t added = 0U;
	vasal_editor_document_begin(0U);
	cairo_assert_true(vasal_editor_edits_add_terrain(&added));
	vasal_editor_document_end();
	cairo_assert_eq(added, count);
	cairo_assert_true(0 == strcmp(map->terrains[added].name, "new terrain"));

	vasal_editor_document_begin(0U);
	cairo_assert_true(vasal_editor_edits_remove_terrain(3U));
	vasal_editor_document_end();
	cairo_assert_eq(map->terrain_count, count);
	cairo_assert_eq(_vasal_tests_editor_edits_tile(map, 1U, 1U), 0U);
	cairo_assert_eq(_vasal_tests_editor_edits_tile(map, 2U, 2U), 3U);
	vasal_editor_document_begin(0U);
	cairo_assert_false(vasal_editor_edits_remove_terrain(0U));
	vasal_editor_document_end();

	size_t counts[vasal_engine_map_terrains_max];
	vasal_editor_edits_census(counts);
	cairo_assert_eq(counts[0], 63U);
	cairo_assert_eq(counts[3], 1U);
}

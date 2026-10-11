
/// Tests of the rules the editor edits: starting, opening, undo and redo.

#include "editor/ruleset.h"

#include "cairo.h"

#include <string.h>

static uint8_t _vasal_tests_editor_ruleset_file[vasal_engine_rules_capacity];

// spec: editor.rules#1 editor.rules#3
cairo_test_new(editor_ruleset, changes_undo_and_redo_exactly) {
	vasal_editor_ruleset_create();
	vasal_engine_rules_s* const rules = vasal_editor_ruleset_rules();
	cairo_assert_true(0 == strcmp(rules->name, "standard"));
	const uint32_t speed =
		rules->units[0].values[vasal_engine_rules_unit_speed];
	vasal_editor_ruleset_begin(7U);
	rules->units[0].values[vasal_engine_rules_unit_speed] = speed + 5U;
	cairo_assert_true(vasal_editor_ruleset_end());
	// A merged second step undoes with the first.
	vasal_editor_ruleset_begin(7U);
	rules->units[0].values[vasal_engine_rules_unit_speed] = speed + 9U;
	cairo_assert_true(vasal_editor_ruleset_end());
	cairo_assert_true(vasal_editor_ruleset_undo());
	cairo_assert_eq(rules->units[0].values[vasal_engine_rules_unit_speed],
		speed);
	cairo_assert_false(vasal_editor_ruleset_undo());
	cairo_assert_true(vasal_editor_ruleset_redo());
	cairo_assert_eq(rules->units[0].values[vasal_engine_rules_unit_speed],
		speed + 9U);
}

// spec: editor.rules#3
cairo_test_new(editor_ruleset, invalid_changes_are_undone_at_once) {
	vasal_editor_ruleset_create();
	vasal_engine_rules_s* const rules = vasal_editor_ruleset_rules();
	vasal_editor_ruleset_begin(0U);
	rules->units[1].values[vasal_engine_rules_unit_sight] = 0U;
	cairo_assert_false(vasal_editor_ruleset_end());
	cairo_assert_gt(rules->units[1].values[vasal_engine_rules_unit_sight], 0U);
	cairo_assert_false(vasal_editor_ruleset_undo());
}

// spec: editor.rules#2
cairo_test_new(editor_ruleset, opens_rules_files) {
	vasal_engine_rules_s rules;
	vasal_engine_rules_standard(&rules);
	(void)strcpy(rules.name, "mine");
	rules.units[3].values[vasal_engine_rules_unit_range] = 11U;
	const size_t size = vasal_engine_rules_write(&rules,
		_vasal_tests_editor_ruleset_file,
		sizeof(_vasal_tests_editor_ruleset_file));
	vasal_editor_ruleset_create();
	cairo_assert_eq(vasal_editor_ruleset_open(_vasal_tests_editor_ruleset_file,
		size), (uint32_t)vasal_engine_rules_read_ok);
	cairo_assert_true(0 == strcmp(vasal_editor_ruleset_rules()->name, "mine"));
	cairo_assert_eq(vasal_editor_ruleset_rules()->units[3].values
		[vasal_engine_rules_unit_range], 11U);
	// A broken file changes nothing.
	cairo_assert_neq(vasal_editor_ruleset_open(_vasal_tests_editor_ruleset_file,
		size - 3U), (uint32_t)vasal_engine_rules_read_ok);
	cairo_assert_true(0 == strcmp(vasal_editor_ruleset_rules()->name, "mine"));
}

// spec: editor.rules#6
cairo_test_new(editor_ruleset, adds_and_duplicates_unit_types) {
	vasal_editor_ruleset_create();
	vasal_engine_rules_s* const rules = vasal_editor_ruleset_rules();
	const uint8_t count = rules->unit_count;
	const uint32_t revision = vasal_editor_ruleset_revision();
	const uint8_t added = vasal_editor_ruleset_add_unit();
	cairo_assert_eq(added, count);
	cairo_assert_eq(rules->unit_count, count + 1U);
	cairo_assert_true(0 == strcmp(rules->units[added].name, "unit 8"));
	cairo_assert_neq(vasal_editor_ruleset_revision(), revision);
	const uint8_t copy = vasal_editor_ruleset_duplicate_unit(2U);
	cairo_assert_eq(copy, count + 1U);
	cairo_assert_true(0 == strcmp(rules->units[copy].name, "archer copy"));
	cairo_assert_eq(rules->units[copy].values[vasal_engine_rules_unit_range],
		rules->units[2].values[vasal_engine_rules_unit_range]);
	cairo_assert_true(vasal_editor_ruleset_undo());
	cairo_assert_true(vasal_editor_ruleset_undo());
	cairo_assert_eq(rules->unit_count, count);
	// No more than the most unit types.
	while (vasal_engine_rules_units_max > rules->unit_count) {
		(void)vasal_editor_ruleset_add_unit();
	}
	cairo_assert_eq(vasal_editor_ruleset_add_unit(), vasal_engine_rules_none);
	cairo_assert_eq(vasal_editor_ruleset_duplicate_unit(0U),
		vasal_engine_rules_none);
	cairo_assert_true(0 == strcmp(rules->units[29].name, "unit 30"));
}

// spec: editor.rules#6
cairo_test_new(editor_ruleset, removing_a_unit_type_fixes_references) {
	vasal_editor_ruleset_create();
	vasal_engine_rules_s* const rules = vasal_editor_ruleset_rules();
	// The standard start: spearmen, swordsmen, archers, throwers and
	// scouts, the captain as centenar; the camp holds bandits and archers.
	const uint32_t offers = rules->settlements[0]
		[vasal_engine_rules_settlement_flags];
	cairo_assert_true(vasal_editor_ruleset_remove_unit(2U));
	cairo_assert_eq(rules->unit_count, 6U);
	cairo_assert_true(0 == strcmp(rules->units[2].name, "thrower"));
	cairo_assert_eq(rules->start[2][0].unit, vasal_engine_rules_none);
	cairo_assert_eq(rules->start[2][0].count, 0U);
	cairo_assert_eq(rules->start[3][0].unit, 2U);
	cairo_assert_eq(rules->start[0][0].unit, 0U);
	cairo_assert_eq(rules->centenar, 4U);
	cairo_assert_eq(rules->camp[0].unit, 5U);
	cairo_assert_eq(rules->camp[1].unit, vasal_engine_rules_none);
	cairo_assert_eq(rules->settlements[0][vasal_engine_rules_settlement_flags],
		vasal_editor_ruleset_flags_after(offers, 2U, vasal_engine_rules_none,
			vasal_engine_rules_none));
	cairo_assert_true(vasal_engine_rules_valid(rules));
	// The last type stays.
	while (1U < rules->unit_count) {
		cairo_assert_true(vasal_editor_ruleset_remove_unit(0U));
	}
	cairo_assert_false(vasal_editor_ruleset_remove_unit(0U));
	cairo_assert_eq(rules->centenar, vasal_engine_rules_none);
}

// spec: editor.rules#6
cairo_test_new(editor_ruleset, swaps_unit_types_with_references) {
	vasal_editor_ruleset_create();
	vasal_engine_rules_s* const rules = vasal_editor_ruleset_rules();
	cairo_assert_true(vasal_editor_ruleset_swap_units(0U));
	cairo_assert_true(0 == strcmp(rules->units[0].name, "swordsman"));
	cairo_assert_eq(rules->start[0][0].unit, 1U);
	cairo_assert_eq(rules->start[1][0].unit, 0U);
	cairo_assert_false(vasal_editor_ruleset_swap_units(
		(uint8_t)(rules->unit_count - 1U)));
	// Flags: bit 0 stays, bits 1 and 2 trade places.
	cairo_assert_eq(vasal_editor_ruleset_flags_after(0x03U,
		vasal_engine_rules_none, 0U, 1U), 0x05U);
	cairo_assert_eq(vasal_editor_ruleset_flags_after(0x0dU, 1U,
		vasal_engine_rules_none, vasal_engine_rules_none), 0x05U);
}

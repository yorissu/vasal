
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

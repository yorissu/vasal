
/// Tests of the rules: the standard rules, their file, and its checks.

#include "engine/rules.h"

#include "cairo.h"

#include <string.h>

static vasal_engine_rules_s _vasal_tests_engine_rules_rules;
static vasal_engine_rules_s _vasal_tests_engine_rules_read_back;
static uint8_t _vasal_tests_engine_rules_file[vasal_engine_rules_capacity];

static size_t _vasal_tests_engine_rules_write(void) {
	vasal_engine_rules_standard(&_vasal_tests_engine_rules_rules);
	return vasal_engine_rules_write(&_vasal_tests_engine_rules_rules,
		_vasal_tests_engine_rules_file,
		sizeof(_vasal_tests_engine_rules_file));
}

static vasal_engine_rules_read_e _vasal_tests_engine_rules_read(
		const size_t size) {
	uint16_t version = 0U;
	return vasal_engine_rules_read(_vasal_tests_engine_rules_file, size,
		&_vasal_tests_engine_rules_read_back, &version);
}

// spec: rules.content#1 rules.content#3
cairo_test_new(engine_rules, the_standard_rules_hold_the_first_build) {
	vasal_engine_rules_s* const rules = &_vasal_tests_engine_rules_rules;
	vasal_engine_rules_standard(rules);
	cairo_assert_true(vasal_engine_rules_valid(rules));
	cairo_assert_true(0 == strcmp(rules->units[0].name, "spearman"));
	cairo_assert_true(0 == strcmp(rules->units[2].name, "archer"));
	cairo_assert_eq(vasal_engine_rules_get(rules, 2U,
		vasal_engine_rules_unit_ranged), 1U);
	cairo_assert_gt(vasal_engine_rules_get(rules, 2U,
		vasal_engine_rules_unit_range), 0U);
	// The scout sees farther and moves faster than every other type.
	for (size_t u = 0U; u < rules->unit_count; u++) {
		if (4U != u) {
			cairo_assert_gt(vasal_engine_rules_get(rules, 4U,
				vasal_engine_rules_unit_sight),
				vasal_engine_rules_get(rules, u,
					vasal_engine_rules_unit_sight));
			cairo_assert_gt(vasal_engine_rules_get(rules, 4U,
				vasal_engine_rules_unit_speed),
				vasal_engine_rules_get(rules, u,
					vasal_engine_rules_unit_speed));
		}
	}
	cairo_assert_neq(rules->centenar, vasal_engine_rules_none);
}

// spec: rules.content#2
cairo_test_new(engine_rules, written_and_read_back_unchanged) {
	const size_t size = _vasal_tests_engine_rules_write();
	cairo_assert_gt(size, 0U);
	cairo_assert_eq(_vasal_tests_engine_rules_read(size),
		vasal_engine_rules_read_ok);
	const vasal_engine_rules_s* const a = &_vasal_tests_engine_rules_rules;
	const vasal_engine_rules_s* const b = &_vasal_tests_engine_rules_read_back;
	cairo_assert_eq(b->unit_count, a->unit_count);
	for (size_t u = 0U; u < a->unit_count; u++) {
		cairo_assert_true(0 == strcmp(a->units[u].name, b->units[u].name));
		cairo_assert_eq(a->units[u].color, b->units[u].color);
		cairo_assert_true(0 == memcmp(a->units[u].values, b->units[u].values,
			sizeof(a->units[u].values)));
	}
	cairo_assert_true(0 == memcmp(a->combat, b->combat, sizeof(a->combat)));
	cairo_assert_true(0 == memcmp(a->settlements, b->settlements,
		sizeof(a->settlements)));
	cairo_assert_true(0 == memcmp(a->start, b->start, sizeof(a->start)));
	cairo_assert_true(0 == memcmp(a->camp, b->camp, sizeof(a->camp)));
	cairo_assert_eq(a->centenar, b->centenar);
	cairo_assert_true(vasal_engine_rules_equal(a, b));
	_vasal_tests_engine_rules_read_back.units[1].values[3]++;
	cairo_assert_false(vasal_engine_rules_equal(a, b));
}

// spec: rules.content#2
cairo_test_new(engine_rules, refuses_broken_files) {
	const size_t size = _vasal_tests_engine_rules_write();
	cairo_assert_eq(_vasal_tests_engine_rules_read(size - 1U),
		vasal_engine_rules_read_checksum);
	_vasal_tests_engine_rules_file[0] = 'x';
	cairo_assert_eq(_vasal_tests_engine_rules_read(size),
		vasal_engine_rules_read_magic);
	_vasal_tests_engine_rules_file[0] = 'v';
	_vasal_tests_engine_rules_file[8] = 9U;
	cairo_assert_eq(_vasal_tests_engine_rules_read(size),
		vasal_engine_rules_read_version);
	_vasal_tests_engine_rules_file[8] = 1U;
	_vasal_tests_engine_rules_file[60] ^= 0xffU;
	cairo_assert_eq(_vasal_tests_engine_rules_read(size),
		vasal_engine_rules_read_checksum);
}

// spec: rules.content#2
cairo_test_new(engine_rules, checks_every_value_and_reference) {
	vasal_engine_rules_s* const rules = &_vasal_tests_engine_rules_rules;
	vasal_engine_rules_standard(rules);
	rules->units[0].values[vasal_engine_rules_unit_speed] = 0U;
	cairo_assert_false(vasal_engine_rules_valid(rules));
	cairo_assert_eq(vasal_engine_rules_write(rules,
		_vasal_tests_engine_rules_file,
		sizeof(_vasal_tests_engine_rules_file)), 0U);
	vasal_engine_rules_standard(rules);
	rules->start[4][0].unit = 29U;
	rules->start[4][0].count = 3U;
	cairo_assert_false(vasal_engine_rules_valid(rules));
	vasal_engine_rules_standard(rules);
	rules->start[0][1].unit = 1U;
	rules->start[0][1].count = 5U;
	cairo_assert_false(vasal_engine_rules_valid(rules));
	vasal_engine_rules_standard(rules);
	rules->settlements[0][vasal_engine_rules_settlement_flags] =
		(uint32_t)1U << 20U;
	cairo_assert_false(vasal_engine_rules_valid(rules));
	vasal_engine_rules_standard(rules);
	rules->centenar = 20U;
	cairo_assert_false(vasal_engine_rules_valid(rules));
	rules->centenar = vasal_engine_rules_none;
	cairo_assert_true(vasal_engine_rules_valid(rules));
}

// spec: rules.content#4 settlements.types#1
cairo_test_new(engine_rules, settlements_take_their_types_defaults) {
	vasal_engine_rules_s* const rules = &_vasal_tests_engine_rules_rules;
	vasal_engine_rules_standard(rules);
	rules->settlements[vasal_engine_map_settlement_castle]
		[vasal_engine_rules_settlement_radius] = 13U;
	vasal_engine_map_settlement_s settlement;
	vasal_engine_rules_settlement_defaults(rules, &settlement,
		vasal_engine_map_settlement_castle);
	cairo_assert_eq(settlement.type,
		(uint8_t)vasal_engine_map_settlement_castle);
	cairo_assert_eq(settlement.radius, 13U);
	cairo_assert_eq(settlement.flags, rules->settlements
		[vasal_engine_map_settlement_castle]
		[vasal_engine_rules_settlement_flags]);
}

// spec: rules.content#2
cairo_test_new(engine_rules, every_field_is_described) {
	for (size_t f = 0U; f < (size_t)vasal_engine_rules_unit_fields; f++) {
		const vasal_engine_rules_field_s* const field =
			vasal_engine_rules_unit_field((vasal_engine_rules_unit_e)f);
		cairo_assert_gt(strlen(field->label), 0U);
		cairo_assert_le(field->minimum, field->maximum);
	}
	for (size_t f = 0U; f < (size_t)vasal_engine_rules_combat_fields; f++) {
		cairo_assert_gt(strlen(vasal_engine_rules_combat_field(
			(vasal_engine_rules_combat_e)f)->label), 0U);
	}
	for (size_t f = 0U; f < (size_t)vasal_engine_rules_settlement_fields;
			f++) {
		cairo_assert_gt(strlen(vasal_engine_rules_settlement_field(
			(vasal_engine_rules_settlement_e)f)->label), 0U);
	}
}

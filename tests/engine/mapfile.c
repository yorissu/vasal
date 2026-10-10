
/// Tests of the map model and the map file: writing, reading back, and
/// refusing anything out of place.

#include "engine/crc.h"
#include "engine/map.h"
#include "engine/mapfile.h"

#include "cairo.h"

#include <string.h>

static vasal_engine_map_s _vasal_tests_engine_mapfile_map;
static vasal_engine_map_s _vasal_tests_engine_mapfile_read_back;
static uint8_t _vasal_tests_engine_mapfile_file[vasal_engine_mapfile_capacity];

// A small map with one of everything.
static vasal_engine_map_s* _vasal_tests_engine_mapfile_sample(void) {
	vasal_engine_map_s* const map = &_vasal_tests_engine_mapfile_map;
	(void)vasal_engine_map_clear(map, 40U, 30U);
	(void)strcpy(map->name, "sample");
	const vasal_engine_map_terrain_s water = {
		.name = "water", .color = 0x3060c0ffU, .speed = 0U, .passable = false
	};
	const vasal_engine_map_terrain_s tall_grass = {
		.name = "tall grass", .color = 0x60a040ffU, .speed = 60U,
		.passable = true
	};
	map->terrains[1] = water;
	map->terrains[2] = tall_grass;
	map->terrain_count = 3U;
	map->tiles[(5U * 40U) + 7U] = 1U;
	map->tiles[(6U * 40U) + 7U] = 2U;
	const vasal_engine_map_settlement_s village = {
		.name = "Dvor", .position = { 10U, 12U }, .radius = 3U,
		.requirement = 10U, .capture_time = 200U, .bonus = 100U,
		.income = 50U, .flags = 5U, .type = 0U
	};
	map->settlements[0] = village;
	map->settlement_count = 1U;
	map->spawns[0].position.x = 30U;
	map->spawns[0].position.y = 20U;
	map->spawns[0].radius = 4U;
	map->spawn_count = 1U;
	map->camps[0].position.x = 2U;
	map->camps[0].position.y = 3U;
	map->camp_count = 1U;
	map->decorations[0].position.x = 1U;
	map->decorations[0].position.y = 1U;
	map->decorations[0].kind = 7U;
	map->decoration_count = 1U;
	return map;
}

static size_t _vasal_tests_engine_mapfile_write(void) {
	return vasal_engine_mapfile_write(_vasal_tests_engine_mapfile_sample(),
		_vasal_tests_engine_mapfile_file,
		sizeof(_vasal_tests_engine_mapfile_file));
}

static vasal_engine_mapfile_read_e _vasal_tests_engine_mapfile_read(
		const size_t size) {
	uint16_t version = 0U;
	return vasal_engine_mapfile_read(_vasal_tests_engine_mapfile_file, size,
		&_vasal_tests_engine_mapfile_read_back, &version);
}

// Rewrites the checksum after a test changed the file, so the change
// itself is what is checked.
static void _vasal_tests_engine_mapfile_reseal(const size_t size) {
	const uint32_t crc = vasal_engine_crc_compute(
		_vasal_tests_engine_mapfile_file, size - 4U);
	for (size_t i = 0U; i < 4U; i++) {
		_vasal_tests_engine_mapfile_file[size - 4U + i] =
			(uint8_t)((crc >> (8U * i)) & 0xffU);
	}
}

// spec: mapfile.format#1 map.terrain#5
cairo_test_new(engine_mapfile, writes_and_reads_back_unchanged) {
	const size_t size = _vasal_tests_engine_mapfile_write();
	cairo_assert_gt(size, 0U);
	cairo_assert_eq(_vasal_tests_engine_mapfile_read(size),
		vasal_engine_mapfile_read_ok);
	const vasal_engine_map_s* const a = &_vasal_tests_engine_mapfile_map;
	const vasal_engine_map_s* const b = &_vasal_tests_engine_mapfile_read_back;
	cairo_assert_eq(b->width, 40U);
	cairo_assert_eq(b->height, 30U);
	cairo_assert_true(0 == strcmp(a->name, b->name));
	cairo_assert_eq(b->terrain_count, 3U);
	cairo_assert_true(0 == strcmp(b->terrains[1].name, "water"));
	cairo_assert_false(b->terrains[1].passable);
	cairo_assert_eq(b->terrains[2].speed, 60U);
	cairo_assert_true(b->terrains[2].passable);
	cairo_assert_eq(0, memcmp(a->tiles, b->tiles, 40U * 30U));
	cairo_assert_eq(b->settlement_count, 1U);
	cairo_assert_true(0 == strcmp(b->settlements[0].name, "Dvor"));
	cairo_assert_eq(b->settlements[0].capture_time, 200U);
	cairo_assert_eq(b->settlements[0].flags, 5U);
	cairo_assert_eq(b->spawns[0].radius, 4U);
	cairo_assert_eq(b->camps[0].position.y, 3U);
	cairo_assert_eq(b->decorations[0].kind, 7U);
}

// spec: mapfile.format#2
cairo_test_new(engine_mapfile, refuses_another_version) {
	const size_t size = _vasal_tests_engine_mapfile_write();
	_vasal_tests_engine_mapfile_file[8] = 2U;
	_vasal_tests_engine_mapfile_reseal(size);
	uint16_t version = 0U;
	cairo_assert_eq(vasal_engine_mapfile_read(_vasal_tests_engine_mapfile_file,
		size, &_vasal_tests_engine_mapfile_read_back, &version),
		vasal_engine_mapfile_read_version);
	cairo_assert_eq(version, 2U);
}

// spec: mapfile.validation#1
cairo_test_new(engine_mapfile, refuses_a_wrong_magic_word) {
	const size_t size = _vasal_tests_engine_mapfile_write();
	_vasal_tests_engine_mapfile_file[0] = 'V';
	cairo_assert_eq(_vasal_tests_engine_mapfile_read(size),
		vasal_engine_mapfile_read_magic);
}

// spec: mapfile.validation#1
cairo_test_new(engine_mapfile, refuses_a_corrupted_file) {
	const size_t size = _vasal_tests_engine_mapfile_write();
	_vasal_tests_engine_mapfile_file[100] ^= 0x01U;
	cairo_assert_eq(_vasal_tests_engine_mapfile_read(size),
		vasal_engine_mapfile_read_checksum);
}

// spec: mapfile.validation#1
cairo_test_new(engine_mapfile, refuses_a_cut_file) {
	const size_t size = _vasal_tests_engine_mapfile_write();
	for (size_t cut = 1U; cut < size; cut += 97U) {
		_vasal_tests_engine_mapfile_reseal(size - cut);
		cairo_assert_neq(_vasal_tests_engine_mapfile_read(size - cut),
			vasal_engine_mapfile_read_ok);
	}
}

// spec: mapfile.validation#1
cairo_test_new(engine_mapfile, refuses_tiles_of_unknown_terrain) {
	vasal_engine_map_s* const map = _vasal_tests_engine_mapfile_sample();
	map->tiles[0] = 9U;
	const size_t size = vasal_engine_mapfile_write(map,
		_vasal_tests_engine_mapfile_file,
		sizeof(_vasal_tests_engine_mapfile_file));
	cairo_assert_eq(_vasal_tests_engine_mapfile_read(size),
		vasal_engine_mapfile_read_value);
}

// spec: mapfile.validation#1
cairo_test_new(engine_mapfile, refuses_positions_outside_the_map) {
	vasal_engine_map_s* const map = _vasal_tests_engine_mapfile_sample();
	map->settlements[0].position.x = 40U;
	const size_t size = vasal_engine_mapfile_write(map,
		_vasal_tests_engine_mapfile_file,
		sizeof(_vasal_tests_engine_mapfile_file));
	cairo_assert_eq(_vasal_tests_engine_mapfile_read(size),
		vasal_engine_mapfile_read_value);
}

// spec: mapfile.validation#1
cairo_test_new(engine_mapfile, refuses_counts_above_the_limits) {
	const size_t size = _vasal_tests_engine_mapfile_write();
	// The sample's camp count: after the header (46 bytes), 3 terrain
	// types (1 + 3 * 39), 40 * 30 tiles, 1 settlement (2 + 57) and 1 spawn
	// area (2 + 6).
	const size_t camps = 46U + 118U + 1200U + 59U + 8U;
	cairo_assert_eq(_vasal_tests_engine_mapfile_file[camps], 1U);
	_vasal_tests_engine_mapfile_file[camps] = 0xffU;
	_vasal_tests_engine_mapfile_file[camps + 1U] = 0xffU;
	_vasal_tests_engine_mapfile_reseal(size);
	cairo_assert_eq(_vasal_tests_engine_mapfile_read(size),
		vasal_engine_mapfile_read_value);
}

// spec: map.space#4
cairo_test_new(engine_mapfile, refuses_sides_above_the_limit) {
	vasal_engine_map_s* const map = _vasal_tests_engine_mapfile_sample();
	cairo_assert_false(vasal_engine_map_clear(map, 2049U, 10U));
	cairo_assert_false(vasal_engine_map_clear(map, 10U, 0U));
	cairo_assert_true(vasal_engine_map_clear(map, 2048U, 1U));
}

// spec: map.contents#1
cairo_test_new(engine_map, needs_a_settlement_and_a_spawn_area) {
	vasal_engine_map_s* const map = &_vasal_tests_engine_mapfile_map;
	(void)vasal_engine_map_clear(map, 20U, 20U);
	vasal_engine_map_finding_s findings[4];
	cairo_assert_eq(vasal_engine_map_check(map, findings, 4U), 2U);
	cairo_assert_eq(findings[0].problem,
		vasal_engine_map_problem_no_settlement);
	cairo_assert_eq(findings[1].problem, vasal_engine_map_problem_no_spawn);
	cairo_assert_eq(vasal_engine_map_check(_vasal_tests_engine_mapfile_sample(),
		findings, 4U), 0U);
}

// spec: editor.validation#1
cairo_test_new(engine_map, reports_impassable_and_overlapping_places) {
	vasal_engine_map_s* const map = _vasal_tests_engine_mapfile_sample();
	map->tiles[(12U * 40U) + 10U] = 1U;
	map->settlements[1] = map->settlements[0];
	map->settlements[1].position.x = 13U;
	map->settlement_count = 2U;
	vasal_engine_map_finding_s findings[4];
	cairo_assert_eq(vasal_engine_map_check(map, findings, 4U), 2U);
	cairo_assert_eq(findings[0].problem,
		vasal_engine_map_problem_settlement_impassable);
	cairo_assert_eq(findings[1].problem,
		vasal_engine_map_problem_capture_overlap);
}

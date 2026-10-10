
/// Tests of the minimap's geometry and colors.

#include "client/minimap.h"

#include "cairo.h"

static vasal_engine_map_s _vasal_tests_client_minimap_map;

// A 200 by 100 map of grass with a lake tile at (10, 20).
static vasal_engine_map_s* _vasal_tests_client_minimap_start(void) {
	vasal_engine_map_s* const map = &_vasal_tests_client_minimap_map;
	(void)vasal_engine_map_clear(map, 200U, 100U);
	map->terrains[0].color = 0x6a9a3cffU;
	const vasal_engine_map_terrain_s water = {
		.name = "water", .color = 0x3d6fb0ffU, .speed = 0U,
		.passable = false
	};
	map->terrains[1] = water;
	map->terrain_count = 2U;
	map->tiles[(20U * 200U) + 10U] = 1U;
	return map;
}

// spec: map.minimap#1
cairo_test_new(client_minimap, the_whole_map_fits_as_a_diamond) {
	const vasal_engine_map_s* const map = _vasal_tests_client_minimap_start();
	const float width = (float)vasal_client_minimap_width;
	const float height = (float)vasal_client_minimap_height;
	// Squashed as the main view is.
	cairo_assert_nr(height, width * vasal_client_camera_tilt, 0.001f);
	float px = 0.0f;
	float py = 0.0f;
	// The map's corners are the diamond's: top, right, bottom, left.
	vasal_client_minimap_to_pixel(map, 0.0f, 0.0f, &px, &py);
	cairo_assert_nr(py, 0.0f, 0.001f);
	vasal_client_minimap_to_pixel(map, 200.0f, 0.0f, &px, &py);
	cairo_assert_nr(px, width, 0.001f);
	vasal_client_minimap_to_pixel(map, 200.0f, 100.0f, &px, &py);
	cairo_assert_nr(py, height, 0.001f);
	vasal_client_minimap_to_pixel(map, 0.0f, 100.0f, &px, &py);
	cairo_assert_nr(px, 0.0f, 0.001f);
	// Its x axis runs down to the right, as in the main view.
	float later_x = 0.0f;
	float later_y = 0.0f;
	vasal_client_minimap_to_pixel(map, 50.0f, 50.0f, &px, &py);
	vasal_client_minimap_to_pixel(map, 60.0f, 50.0f, &later_x, &later_y);
	cairo_assert_gt(later_x, px);
	cairo_assert_gt(later_y, py);
}

// spec: map.minimap#1
cairo_test_new(client_minimap, pixels_lead_back_to_their_tiles) {
	const vasal_engine_map_s* const map = _vasal_tests_client_minimap_start();
	float px = 0.0f;
	float py = 0.0f;
	vasal_client_minimap_to_pixel(map, 123.5f, 45.25f, &px, &py);
	float x = 0.0f;
	float y = 0.0f;
	cairo_assert_true(vasal_client_minimap_to_map(map, px, py, &x, &y));
	cairo_assert_nr(x, 123.5f, 0.01f);
	cairo_assert_nr(y, 45.25f, 0.01f);
	// The top left corner of the picture is beside the diamond.
	cairo_assert_false(vasal_client_minimap_to_map(map, 1.0f, 1.0f, &x, &y));
}

// spec: map.minimap#1
cairo_test_new(client_minimap, terrain_is_gray_and_each_type_its_shade) {
	const vasal_engine_map_s* const map = _vasal_tests_client_minimap_start();
	const uint32_t grass = vasal_client_minimap_color(map, 0U, 0U);
	const uint32_t water = vasal_client_minimap_color(map, 10U, 20U);
	cairo_assert_eq((grass >> 24U) & 0xffU, (grass >> 16U) & 0xffU);
	cairo_assert_eq((grass >> 16U) & 0xffU, (grass >> 8U) & 0xffU);
	cairo_assert_eq(grass & 0xffU, 0xffU);
	cairo_assert_neq(grass, water);
}

// spec: map.minimap#1
cairo_test_new(client_minimap, sits_in_the_bottom_right_corner) {
	const float width = 1280.0f;
	const float height = 720.0f;
	float left = 0.0f;
	float top = 0.0f;
	vasal_client_minimap_panel(width, height, &left, &top);
	cairo_assert_true(vasal_client_minimap_contains(width, height,
		left + 1.0f, top + 1.0f));
	cairo_assert_true(vasal_client_minimap_contains(width, height,
		width - 13.0f, height - 13.0f));
	cairo_assert_false(vasal_client_minimap_contains(width, height,
		width - 5.0f, height - 5.0f));
	cairo_assert_false(vasal_client_minimap_contains(width, height,
		640.0f, 360.0f));
}

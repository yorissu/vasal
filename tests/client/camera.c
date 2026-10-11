
/// Tests of the camera: framing, following, gliding and resizing. The map
/// is 200 by 200 tiles with the centaine in the middle; the monitor is
/// 1600 by 900 pixels and the window half of it, unless a test says
/// otherwise.

#include "client/camera.h"

#include "cairo.h"

#include <math.h>

static vasal_engine_map_s _vasal_tests_client_camera_map;
static vasal_client_game_s _vasal_tests_client_camera_game;

static vasal_client_game_s* _vasal_tests_client_camera_start(void) {
	vasal_engine_map_s* const map = &_vasal_tests_client_camera_map;
	(void)vasal_engine_map_clear(map, 200U, 200U);
	const vasal_engine_map_spawn_s spawn = { { 100U, 100U }, 5U };
	map->spawns[0] = spawn;
	map->spawn_count = 1U;
	vasal_client_game_start(&_vasal_tests_client_camera_game, map, NULL);
	return &_vasal_tests_client_camera_game;
}

// Moves a group's anchor to a tile, as if it had been there for a while.
static void _vasal_tests_client_camera_place(vasal_client_game_s* const game,
		const size_t group, const float x, const float y) {
	const float precision = (float)vasal_engine_map_precision;
	const vasal_engine_world_position_s at = {
		(int32_t)(x * precision), (int32_t)(y * precision)
	};
	game->world->vasals[game->vasal].groups[group].anchor = at;
	game->previous_anchors[game->vasal][group] = at;
}

// The camera after its first update, which jumps.
static vasal_client_camera_s _vasal_tests_client_camera_first(
		const vasal_client_game_s* const game, const float width,
		const float height, const float monitor_width,
		const float monitor_height) {
	vasal_client_camera_s camera = { 0.0f, 0.0f, 1.0f, false };
	vasal_client_camera_update(&camera, game, 0.0f, width, height,
		monitor_width, monitor_height);
	return camera;
}

// How many tiles the monitor frames: across, or down in the slanted view,
// whichever is fewer.
static float _vasal_tests_client_camera_frame(
		const vasal_client_camera_s* const camera, const float monitor_width,
		const float monitor_height) {
	return fminf(monitor_width / camera->scale,
		monitor_height / (camera->scale * vasal_client_camera_tilt));
}

static float _vasal_tests_client_camera_expected(const float extent) {
	return 2.0f * extent * (1.0f + vasal_client_camera_margin);
}

// spec: map.camera#1
cairo_test_new(client_camera, sight_fits_the_monitor_with_margin) {
	vasal_client_game_s* const game = _vasal_tests_client_camera_start();
	const vasal_client_camera_s camera = _vasal_tests_client_camera_first(
		game, 800.0f, 450.0f, 1600.0f, 900.0f);
	cairo_assert_nr(_vasal_tests_client_camera_frame(&camera, 1600.0f,
		900.0f), _vasal_tests_client_camera_expected(vasal_client_game_sight),
		0.01f);
	const vasal_client_camera_s large = _vasal_tests_client_camera_first(
		game, 800.0f, 450.0f, 3200.0f, 1800.0f);
	cairo_assert_nr(_vasal_tests_client_camera_frame(&large, 3200.0f,
		1800.0f), _vasal_tests_client_camera_frame(&camera, 1600.0f, 900.0f),
		0.01f);
}

// spec: map.camera#2
cairo_test_new(client_camera, frames_the_focused_alone) {
	vasal_client_game_s* const game = _vasal_tests_client_camera_start();
	_vasal_tests_client_camera_place(game, 0U, 100.0f, 100.0f);
	_vasal_tests_client_camera_place(game, 1U, 110.0f, 100.0f);
	(void)vasal_client_game_select(game, 1U, false);
	(void)vasal_client_game_select(game, 0U, true);
	const vasal_client_camera_s camera = _vasal_tests_client_camera_first(
		game, 800.0f, 450.0f, 1600.0f, 900.0f);
	cairo_assert_nr(_vasal_tests_client_camera_frame(&camera, 1600.0f,
		900.0f), _vasal_tests_client_camera_expected(vasal_client_game_sight),
		0.01f);
	cairo_assert_nr(camera.x, 100.0f, 0.01f);
	cairo_assert_nr(camera.y, 100.0f, 0.01f);
}

// spec: map.camera#3
cairo_test_new(client_camera, moves_to_the_last_selected) {
	vasal_client_game_s* const game = _vasal_tests_client_camera_start();
	_vasal_tests_client_camera_place(game, 0U, 100.0f, 100.0f);
	_vasal_tests_client_camera_place(game, 1U, 110.0f, 100.0f);
	(void)vasal_client_game_select(game, 0U, false);
	vasal_client_camera_s camera = _vasal_tests_client_camera_first(game,
		800.0f, 450.0f, 1600.0f, 900.0f);
	const float scale = camera.scale;
	(void)vasal_client_game_select(game, 1U, true);
	for (size_t frame = 0U; frame < 300U; frame++) {
		vasal_client_camera_update(&camera, game, 1.0f / 60.0f, 800.0f,
			450.0f, 1600.0f, 900.0f);
	}
	cairo_assert_nr(camera.x, 110.0f, 0.01f);
	cairo_assert_nr(camera.scale, scale, 0.0001f);
}

// spec: map.camera#4
cairo_test_new(client_camera, never_frames_more_than_max) {
	vasal_client_game_s* const game = _vasal_tests_client_camera_start();
	vasal_client_game_select_all(game, 0U);
	const vasal_client_camera_s camera = _vasal_tests_client_camera_first(
		game, 800.0f, 450.0f, 1600.0f, 900.0f);
	cairo_assert_le(_vasal_tests_client_camera_frame(&camera, 1600.0f,
		900.0f), vasal_client_camera_max_frame + 0.01f);
}

// spec: map.camera#5
cairo_test_new(client_camera, follows_focus_to_the_edges) {
	vasal_client_game_s* const game = _vasal_tests_client_camera_start();
	_vasal_tests_client_camera_place(game, 0U, 100.0f, 100.0f);
	vasal_client_camera_s camera = _vasal_tests_client_camera_first(game,
		800.0f, 450.0f, 1600.0f, 900.0f);
	cairo_assert_nr(camera.x, 100.0f, 0.01f);
	cairo_assert_nr(camera.y, 100.0f, 0.01f);
	_vasal_tests_client_camera_place(game, 0U, 2.0f, 198.0f);
	for (size_t frame = 0U; frame < 600U; frame++) {
		vasal_client_camera_update(&camera, game, 1.0f / 60.0f, 800.0f,
			450.0f, 1600.0f, 900.0f);
	}
	cairo_assert_nr(camera.x, 2.0f, 0.01f);
	cairo_assert_nr(camera.y, 198.0f, 0.01f);
	float sx = 0.0f;
	float sy = 0.0f;
	vasal_client_camera_to_screen(&camera, 800.0f, 450.0f, 2.0f, 198.0f, &sx,
		&sy);
	cairo_assert_nr(sx, 400.0f, 0.01f);
	cairo_assert_nr(sy, 225.0f, 0.01f);
}

// spec: client.render#2
cairo_test_new(client_camera, isometric_projection) {
	const vasal_client_camera_s camera = { 50.0f, 50.0f, 40.0f, true };
	float sx = 0.0f;
	float sy = 0.0f;
	// Along x the map runs down to the right, along y down to the left,
	// and a tile is twice as wide as tall.
	vasal_client_camera_to_screen(&camera, 800.0f, 600.0f, 51.0f, 50.0f, &sx,
		&sy);
	cairo_assert_gt(sx, 400.0f);
	cairo_assert_gt(sy, 300.0f);
	cairo_assert_nr(sx - 400.0f, 2.0f * (sy - 300.0f), 0.01f);
	vasal_client_camera_to_screen(&camera, 800.0f, 600.0f, 50.0f, 51.0f, &sx,
		&sy);
	cairo_assert_lt(sx, 400.0f);
	cairo_assert_gt(sy, 300.0f);
	float x = 0.0f;
	float y = 0.0f;
	vasal_client_camera_to_screen(&camera, 800.0f, 600.0f, 57.25f, 31.5f, &sx,
		&sy);
	vasal_client_camera_to_map(&camera, 800.0f, 600.0f, sx, sy, &x, &y);
	cairo_assert_nr(x, 57.25f, 0.001f);
	cairo_assert_nr(y, 31.5f, 0.001f);
}

// spec: map.camera#6
cairo_test_new(client_camera, glides_to_new_focus) {
	vasal_client_game_s* const game = _vasal_tests_client_camera_start();
	_vasal_tests_client_camera_place(game, 0U, 100.0f, 100.0f);
	_vasal_tests_client_camera_place(game, 1U, 120.0f, 100.0f);
	vasal_client_camera_s camera = _vasal_tests_client_camera_first(game,
		800.0f, 450.0f, 1600.0f, 900.0f);
	(void)vasal_client_game_select(game, 1U, false);
	vasal_client_camera_update(&camera, game, 1.0f / 60.0f, 800.0f, 450.0f,
		1600.0f, 900.0f);
	cairo_assert_gt(camera.x, 100.0f);
	cairo_assert_lt(camera.x, 110.0f);
	for (size_t frame = 0U; frame < 300U; frame++) {
		vasal_client_camera_update(&camera, game, 1.0f / 60.0f, 800.0f,
			450.0f, 1600.0f, 900.0f);
	}
	cairo_assert_nr(camera.x, 120.0f, 0.01f);
}

// spec: map.camera#8
cairo_test_new(client_camera, resizing_the_window_keeps_scale) {
	vasal_client_game_s* const game = _vasal_tests_client_camera_start();
	const vasal_client_camera_s small = _vasal_tests_client_camera_first(
		game, 800.0f, 450.0f, 1600.0f, 900.0f);
	const vasal_client_camera_s large = _vasal_tests_client_camera_first(
		game, 1400.0f, 800.0f, 1600.0f, 900.0f);
	cairo_assert_nr(small.scale, large.scale, 0.0001f);
	cairo_assert_nr(small.x, large.x, 0.0001f);
	cairo_assert_nr(small.y, large.y, 0.0001f);
}

// spec: map.camera#9
cairo_test_new(client_camera, another_monitor_rescales) {
	vasal_client_game_s* const game = _vasal_tests_client_camera_start();
	const vasal_client_camera_s first = _vasal_tests_client_camera_first(
		game, 800.0f, 450.0f, 1600.0f, 900.0f);
	const vasal_client_camera_s second = _vasal_tests_client_camera_first(
		game, 800.0f, 450.0f, 2560.0f, 1440.0f);
	cairo_assert_nr(second.scale / first.scale, 1.6f, 0.0001f);
}

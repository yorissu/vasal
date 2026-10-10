
/// The game client, running in the browser. It asks the server for the
/// match's map file and falls back to the built-in demonstration map when
/// there is none, then runs the game: input, simulation, camera, drawing.
/// Until the server runs the match (milestone 3), the client simulates the
/// player's centaine itself.

#include "client/demo.h"
#include "client/game.h"
#include "client/hud.h"
#include "client/view.h"
#include "engine/mapfile.h"

#include <emscripten/emscripten.h>
#include <raylib.h>

static vasal_engine_map_s _vasal_client_main_map;
static vasal_client_game_s _vasal_client_main_game;
static vasal_client_camera_s _vasal_client_main_camera;
static vasal_client_input_s _vasal_client_main_input;
static bool_t _vasal_client_main_ready = false;

static void _vasal_client_main_play(void) {
	vasal_client_game_start(&_vasal_client_main_game,
		&_vasal_client_main_map);
	_vasal_client_main_ready = true;
}

// The server sent the map file: play on it if it reads well, else on the
// demonstration map.
static void _vasal_client_main_loaded(void* const argument, void* const data,
		const int size) {
	(void)argument;
	uint16_t version = 0U;
	const vasal_engine_mapfile_read_e outcome = vasal_engine_mapfile_read(
		(const uint8_t*)data, (size_t)size, &_vasal_client_main_map, &version);
	if (vasal_engine_mapfile_read_ok != outcome) {
		vasal_client_demo_make(&_vasal_client_main_map);
	}
	_vasal_client_main_play();
}

// The server has no map file: play on the demonstration map.
static void _vasal_client_main_failed(void* const argument) {
	(void)argument;
	vasal_client_demo_make(&_vasal_client_main_map);
	_vasal_client_main_play();
}

// Runs and draws one frame. The browser calls it once per display refresh.
static void _vasal_client_main_update(void) {
	const float seconds = GetFrameTime();
	const float width = (float)GetScreenWidth();
	const float height = (float)GetScreenHeight();
	BeginDrawing();
	ClearBackground((Color){ 32, 32, 32, 255 });
	if (!_vasal_client_main_ready) {
		DrawText("loading", 20, 20, 20, RAYWHITE);
	} else {
		vasal_client_input_update(&_vasal_client_main_input,
			&_vasal_client_main_game, &_vasal_client_main_camera, seconds,
			width, height);
		vasal_client_game_update(&_vasal_client_main_game, seconds);
		int monitor_width = 0;
		int monitor_height = 0;
		emscripten_get_screen_size(&monitor_width, &monitor_height);
		vasal_client_camera_update(&_vasal_client_main_camera,
			&_vasal_client_main_game, seconds, width, height,
			(float)monitor_width, (float)monitor_height);
		vasal_client_view_draw(&_vasal_client_main_game,
			&_vasal_client_main_camera, width, height);
		vasal_client_hud_draw(&_vasal_client_main_game,
			&_vasal_client_main_input, width, height);
	}
	EndDrawing();
}

// spec: client.platform
int main(void) {
	// A resizable window follows the browser window: raylib sizes the
	// canvas to it at start and whenever it changes.
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(1280, 720, "vasal");
	vasal_client_keys_start(&_vasal_client_main_input.keys);
	emscripten_async_wget_data("map", NULL, _vasal_client_main_loaded,
		_vasal_client_main_failed);
	emscripten_set_main_loop(_vasal_client_main_update, 0, true);
	return 0;
}

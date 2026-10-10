
/// Turns keys and the right button into selection, focus and orders.

#include "client/input.h"
#include "client/minimap.h"

#include <emscripten/emscripten.h>
#include <raylib.h>

#include <math.h>

// The keys of the groups, in order: the vintaines, then the centenar.
static const int _vasal_client_input_group_keys[vasal_engine_world_groups] = {
	KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE, KEY_SIX
};

// Whether shift is held, as the browser's latest key or mouse event says
// (client/shell.html), not as raylib's keys say: raylib keeps a key held
// until its keyup comes, and the browser sometimes keeps it.
// spec: client.input#2
static bool_t _vasal_client_input_shift(void) {
	return 0 != emscripten_run_script_int("Module.shift ? 1 : 0");
}

static void _vasal_client_input_keys(vasal_client_input_s* const input,
		vasal_client_game_s* const game, const float seconds,
		const bool_t shift) {
	vasal_client_keys_wait(&input->keys, seconds);
	for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
		if (IsKeyPressed(_vasal_client_input_group_keys[g])) {
			vasal_client_keys_group(&input->keys, game, g, shift);
		}
	}
	if (IsKeyPressed(KEY_TAB)) {
		vasal_client_game_cycle_focus(game);
	}
	if (IsKeyPressed(KEY_Z)) {
		vasal_client_game_toggle_pace(game);
	}
}

// spec: client.input#1
void vasal_client_input_update(vasal_client_input_s* const input,
	vasal_client_game_s* const game,
	const vasal_client_camera_s* const camera, const float seconds,
	const float width, const float height) {
	const bool_t shift = _vasal_client_input_shift();
	_vasal_client_input_keys(input, game, seconds, shift);
	const Vector2 mouse = GetMousePosition();
	vasal_client_game_action_e chosen = game->action;
	// A press on the minimap opens no wheel and so gives no order.
	// spec: map.minimap#1
	const bool_t pressed = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)
		&& !vasal_client_minimap_contains(width, height, mouse.x, mouse.y);
	const vasal_client_wheel_event_e event = vasal_client_wheel_update(
		&input->wheel, pressed,
		IsMouseButtonReleased(MOUSE_BUTTON_RIGHT), mouse.x, mouse.y, seconds,
		&chosen);
	if ((vasal_client_wheel_event_quick != event)
			&& (vasal_client_wheel_event_chosen != event)) {
		return;
	}
	float x = 0.0f;
	float y = 0.0f;
	vasal_client_camera_to_map(camera, width, height, input->wheel.press_x,
		input->wheel.press_y, &x, &y);
	game->action = chosen;
	vasal_client_game_order(game, (int32_t)floorf(x), (int32_t)floorf(y),
		chosen, shift);
}

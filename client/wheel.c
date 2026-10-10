
/// The wheel has three sectors around where the button went down: move
/// above, the two attacks to the lower left and right. Close to the middle
/// or far outside, the pointer is over none.

#include "client/wheel.h"

#include <math.h>

// spec: vintaines.actions#2 vintaines.actions#3 vintaines.actions#4
vasal_client_game_action_e vasal_client_wheel_action(
	const vasal_client_wheel_s* const wheel, const float x, const float y) {
	const float dx = x - wheel->press_x;
	const float dy = y - wheel->press_y;
	const float distance = sqrtf((dx * dx) + (dy * dy));
	if ((distance < (0.25f * vasal_client_wheel_radius))
			|| (distance > (1.6f * vasal_client_wheel_radius))) {
		return vasal_client_game_action_count;
	}
	if (dy < 0.0f) {
		return vasal_client_game_action_move;
	}
	return (dx < 0.0f) ? vasal_client_game_action_attack_to_point
		: vasal_client_game_action_attack_beyond;
}

// spec: vintaines.actions#1 vintaines.actions#2 vintaines.actions#3
// spec: vintaines.actions#4
vasal_client_wheel_event_e vasal_client_wheel_update(
	vasal_client_wheel_s* const wheel, const bool_t pressed,
	const bool_t released, const float x, const float y, const float seconds,
	vasal_client_game_action_e* const chosen) {
	if (pressed) {
		wheel->pressing = true;
		wheel->open = false;
		wheel->held = 0.0f;
		wheel->press_x = x;
		wheel->press_y = y;
		return vasal_client_wheel_event_none;
	}
	if (!wheel->pressing) {
		return vasal_client_wheel_event_none;
	}
	wheel->held += seconds;
	wheel->open = wheel->held >= vasal_client_wheel_hold;
	if (!released) {
		return vasal_client_wheel_event_none;
	}
	wheel->pressing = false;
	const bool_t was_open = wheel->open;
	wheel->open = false;
	if (!was_open) {
		return vasal_client_wheel_event_quick;
	}
	const vasal_client_game_action_e action =
		vasal_client_wheel_action(wheel, x, y);
	if (vasal_client_game_action_count == action) {
		return vasal_client_wheel_event_cancel;
	}
	*chosen = action;
	return vasal_client_wheel_event_chosen;
}

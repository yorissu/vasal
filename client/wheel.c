
/// The ring around the wheel's middle is split into one part per action,
/// the first centered straight above, the others following clockwise.
/// Close to the middle or far outside, the pointer is over none.

#include "client/wheel.h"

#include <math.h>

#define _vasal_client_wheel_turn 6.28318531f

// How wide each action's part of the ring is, in radians.
static float _vasal_client_wheel_part(void) {
	return _vasal_client_wheel_turn / (float)vasal_client_game_action_count;
}

void vasal_client_wheel_close(vasal_client_wheel_s* const wheel) {
	wheel->open = false;
}

// spec: vintaines.actions#1 vintaines.actions#2 vintaines.actions#3
// spec: vintaines.actions#4 vintaines.actions#7
bool_t vasal_client_wheel_buttons(vasal_client_wheel_s* const wheel,
	const bool_t left, const bool_t right, const float x, const float y,
	const bool_t on_interface, vasal_client_game_action_e* const current) {
	if (on_interface) {
		return false;
	}
	if (right) {
		wheel->open = false;
		return true;
	}
	if (!left) {
		return false;
	}
	if (!wheel->open) {
		wheel->open = true;
		wheel->x = x;
		wheel->y = y;
		return false;
	}
	const vasal_client_game_action_e action =
		vasal_client_wheel_action(wheel, x, y);
	if (vasal_client_game_action_count != action) {
		*current = action;
	}
	wheel->open = false;
	return false;
}

// spec: vintaines.actions#3 vintaines.actions#4
vasal_client_game_action_e vasal_client_wheel_action(
	const vasal_client_wheel_s* const wheel, const float x, const float y) {
	const float dx = x - wheel->x;
	const float dy = y - wheel->y;
	const float distance = sqrtf((dx * dx) + (dy * dy));
	if ((distance < (0.25f * vasal_client_wheel_radius))
			|| (distance > (1.6f * vasal_client_wheel_radius))) {
		return vasal_client_game_action_count;
	}
	// The angle from straight above, clockwise, from 0 to a whole turn,
	// shifted by half a part so the first part is centered above.
	const float part = _vasal_client_wheel_part();
	float angle = atan2f(dx, -dy) + (0.5f * part);
	if (angle < 0.0f) {
		angle += _vasal_client_wheel_turn;
	}
	const float parts = angle / part;
	size_t index = (size_t)parts;
	if (index >= (size_t)vasal_client_game_action_count) {
		index = 0U;
	}
	return (vasal_client_game_action_e)index;
}

// spec: vintaines.actions#2
void vasal_client_wheel_place(const vasal_client_game_action_e action,
	float* const dx, float* const dy) {
	const float angle = (float)action * _vasal_client_wheel_part();
	*dx = sinf(angle) * 0.6f * vasal_client_wheel_radius;
	*dy = -cosf(angle) * 0.6f * vasal_client_wheel_radius;
}

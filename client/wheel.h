
/// The mouse buttons and the action wheel, as a state machine free of any
/// input library, so it can be tested. The right button gives orders and
/// the left one works the interface: a right-click orders the current
/// action toward the pointer; a left click on the map opens the wheel
/// around the pointer, and with the wheel open a left click on an action
/// makes it the current one, and a left click away from every action
/// closes it. The wheel itself gives no orders. Clicks on the interface's
/// panels do neither.

#ifndef vasal__client__wheel_h
#define vasal__client__wheel_h

#include "client/game.h"

/// The wheel's radius, in pixels.
#define vasal_client_wheel_radius 80.0f

/// The wheel's state between frames.
typedef struct vasal_client_wheel_s {
	/// Its middle, where it was opened, in window pixels.
	float  x;
	float  y;
	bool_t open;
} vasal_client_wheel_s;

/// Closes the wheel.
void vasal_client_wheel_close(vasal_client_wheel_s* const wheel);

/// Handles a frame's mouse buttons: whether each went down in it, where
/// the pointer is, in window pixels, and whether it is over a panel of the
/// interface. An action chosen on the wheel is written to current. Returns
/// whether to order the current action toward the pointer.
bool_t vasal_client_wheel_buttons(vasal_client_wheel_s* const wheel,
	const bool_t left, const bool_t right, const float x, const float y,
	const bool_t on_interface, vasal_client_game_action_e* const current);

/// The action of the open wheel under a window pixel, or
/// vasal_client_game_action_count when the pointer is over none.
vasal_client_game_action_e vasal_client_wheel_action(
	const vasal_client_wheel_s* const wheel, const float x, const float y);

/// Where an action's label stands, in pixels from the wheel's middle: the
/// middle of its part of the ring.
void vasal_client_wheel_place(const vasal_client_game_action_e action,
	float* const dx, float* const dy);

#endif

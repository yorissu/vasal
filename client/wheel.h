
/// The right mouse button and the action wheel, as a state machine free of
/// any input library, so it can be tested: a quick click orders the current
/// action; holding the button opens the wheel around the pointer, and
/// releasing it over an action orders that action and makes it the current
/// one, or cancels away from every action.

#ifndef vasal__client__wheel_h
#define vasal__client__wheel_h

#include "client/game.h"

/// How long the button must be held to open the wheel, in seconds
/// (placeholder until tuning).
#define vasal_client_wheel_hold 0.25f

/// The wheel's radius, in pixels.
#define vasal_client_wheel_radius 80.0f

/// The wheel's state between frames.
typedef struct vasal_client_wheel_s {
	/// Where the button went down, in window pixels.
	float  press_x;
	float  press_y;
	/// How long the button has been down, in seconds.
	float  held;
	bool_t pressing;
	bool_t open;
} vasal_client_wheel_s;

/// What a frame of the button gives.
typedef enum vasal_client_wheel_event_e {
	/// Nothing to do.
	vasal_client_wheel_event_none,
	/// Order the current action toward where the button went down.
	vasal_client_wheel_event_quick,
	/// Order the chosen action (it becomes the current one) toward where
	/// the button went down.
	vasal_client_wheel_event_chosen,
	/// The wheel closed away from every action: no order.
	vasal_client_wheel_event_cancel
} vasal_client_wheel_event_e;

/// Advances the wheel by a frame: whether the button went down or up in
/// it, where the pointer is, and how long the frame was. A chosen action
/// is written to chosen.
vasal_client_wheel_event_e vasal_client_wheel_update(
	vasal_client_wheel_s* const wheel, const bool_t pressed,
	const bool_t released, const float x, const float y, const float seconds,
	vasal_client_game_action_e* const chosen);

/// The action of the open wheel under a window pixel, or
/// vasal_client_game_action_count when the pointer is over none.
vasal_client_game_action_e vasal_client_wheel_action(
	const vasal_client_wheel_s* const wheel, const float x, const float y);

#endif


/// Mouse and keyboard. Keys 1 to 5 select a vintaine and 6 the centenar;
/// with shift they add or remove it from the selection. Tab moves the
/// focus. Pressing a group's key twice quickly selects every group. The
/// mouse never selects groups (client/keys.h). The right button orders,
/// through the action wheel (client/wheel.h).

#ifndef vasal__client__input_h
#define vasal__client__input_h

#include "client/camera.h"
#include "client/keys.h"
#include "client/wheel.h"

/// The state of the input between frames.
typedef struct vasal_client_input_s {
	vasal_client_keys_s  keys;
	vasal_client_wheel_s wheel;
} vasal_client_input_s;

/// Reads this frame's input and acts on it.
void vasal_client_input_update(vasal_client_input_s* const input,
	vasal_client_game_s* const game,
	const vasal_client_camera_s* const camera, const float seconds,
	const float width, const float height);

#endif

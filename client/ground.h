
/// The ground: the map drawn as one continuous plane, not as tiles. The
/// simulation keeps a terrain type per tile, but on the screen terrain
/// types blend softly into each other along wavering borders, the map's
/// edge is a straight line, and the edge of the player's sight is a smooth
/// curve around their troops, beyond which the ground is grayed. A shader
/// draws it in one pass over the window.

#ifndef vasal__client__ground_h
#define vasal__client__ground_h

#include "client/camera.h"

/// Draws the ground for a window of the given size in pixels. Returns
/// false, drawing nothing, when the graphics can't run its shader; the
/// caller then draws the ground itself.
bool_t vasal_client_ground_draw(const vasal_client_game_s* const game,
	const vasal_client_camera_s* const camera, const float width,
	const float height);

#endif

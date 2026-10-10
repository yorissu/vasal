
/// Drawing the map and the troops: terrain in color within the player's
/// sight and grayed outside it, fading softly at the edge; the player's
/// troops in their alliance color, bright and accented, and other vasals'
/// troops in sight, dulled; rings under the selected groups and lines to
/// where moving groups are going.

#ifndef vasal__client__view_h
#define vasal__client__view_h

#include "client/camera.h"

/// Draws the map and troops for a window of the given size in pixels.
/// Returns how many troops it drew.
size_t vasal_client_view_draw(const vasal_client_game_s* const game,
	const vasal_client_camera_s* const camera, const float width,
	const float height);

#endif

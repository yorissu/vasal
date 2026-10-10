
/// The interface drawn over the map, kept at the window's edges so it is
/// never cropped with the map: the groups along the bottom, the minimap in
/// the bottom right corner (client/minimap.h), the current action, notices,
/// and the action wheel while it is open.

#ifndef vasal__client__hud_h
#define vasal__client__hud_h

#include "client/input.h"

/// Draws the interface for a window of the given size in pixels.
void vasal_client_hud_draw(const vasal_client_game_s* const game,
	const vasal_client_input_s* const input, const float width,
	const float height);

#endif

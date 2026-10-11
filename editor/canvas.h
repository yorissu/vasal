
/// The map view of the editor: the map drawn from above with its objects,
/// panning and zooming (unlike the game, the editor zooms), a grid, the
/// ground colored by terrain, speed or sight, a minimap, and the tools
/// working on the map: selecting and moving objects, painting terrain with
/// a brush or along a line, filling areas, picking a terrain type from the
/// map, and placing settlements, spawn areas, bandit camps and
/// decorations, previewed under the pointer before they are placed.

#ifndef vasal__editor__canvas_h
#define vasal__editor__canvas_h

#include "editor/state.h"

#include <raylib.h>

/// Handles the pointer over the map view, which covers the area given in
/// window pixels. Without input (a dialog is open, or a list), it only
/// follows the pointer.
void vasal_editor_canvas_update(vasal_editor_state_s* const state,
	const Rectangle area, const float seconds, const bool_t input);

/// Draws the map view into the area.
void vasal_editor_canvas_draw(const vasal_editor_state_s* const state,
	const Rectangle area);

/// Centers the view on the map and picks a zoom that shows all of it.
void vasal_editor_canvas_fit(vasal_editor_state_s* const state,
	const Rectangle area);

/// Centers the view on a tile, zoomed in at least far enough to see a
/// settlement well.
void vasal_editor_canvas_focus(vasal_editor_state_s* const state,
	const uint16_t x, const uint16_t y);

/// Zooms by a factor around the middle of the view.
void vasal_editor_canvas_zoom(vasal_editor_state_s* const state,
	const float factor);

/// Deletes the selected object, as one operation.
void vasal_editor_canvas_delete(vasal_editor_state_s* const state);

/// Copies the selected object a little beside it and selects the copy, as
/// one operation.
void vasal_editor_canvas_duplicate(vasal_editor_state_s* const state);

/// Moves the selected object by some tiles, keeping it on the map.
/// Consecutive moves of the same object are one operation.
void vasal_editor_canvas_nudge(vasal_editor_state_s* const state,
	const int32_t dx, const int32_t dy);

#endif

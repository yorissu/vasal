
/// The map view of the editor: drawing the map and its objects from above,
/// panning and zooming (unlike the game, the editor zooms), and the tools
/// working on the map: painting terrain, and placing, moving and deleting
/// settlements, spawn areas, bandit camps and decorations.

#ifndef vasal__editor__canvas_h
#define vasal__editor__canvas_h

#include "editor/state.h"

#include <raylib.h>

/// Handles the input over the map view, which covers the area given in
/// window pixels. Input over the panels is not for the map.
void vasal_editor_canvas_update(vasal_editor_state_s* const state,
	const Rectangle area, const float seconds);

/// Draws the map view into the area.
void vasal_editor_canvas_draw(const vasal_editor_state_s* const state,
	const Rectangle area);

/// Deletes the selected object, as one operation.
void vasal_editor_canvas_delete(vasal_editor_state_s* const state);

/// Centers the view on the map and picks a zoom that shows all of it.
void vasal_editor_canvas_fit(vasal_editor_state_s* const state,
	const Rectangle area);

#endif


/// The editor's panels, drawn with raygui. On the left: the map's name and
/// size, new, open and save, undo and redo, the tools, the terrain types
/// with their values, the brush, and the problems the map check finds. On
/// the right, while an object is selected: its values and flags.

#ifndef vasal__editor__panel_h
#define vasal__editor__panel_h

#include "editor/state.h"

#include <raylib.h>

/// The width of a panel, in pixels.
#define vasal_editor_panel_width 280.0f

/// Draws the panels and acts on them, for a window of the given size.
/// Returns the area left for the map view.
Rectangle vasal_editor_panel_update(vasal_editor_state_s* const state,
	const float width, const float height);

#endif

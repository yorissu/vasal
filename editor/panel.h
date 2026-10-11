
/// The side panels of the map page. On the left: the map's name and size,
/// the terrain types as swatches with their speed and sight, the selected
/// type's values, the brush, and pictures of what the placing tools place.
/// On the right: the selected object's values (the inspector), every
/// object of the map in a list, and the problems the map check finds, each
/// of which jumps to what it is about.

#ifndef vasal__editor__panel_h
#define vasal__editor__panel_h

#include "editor/state.h"

#include <raylib.h>

/// The width of a side panel, in pixels.
#define vasal_editor_panel_width 300.0f

/// Draws the left panel into its area and acts on it.
void vasal_editor_panel_left(vasal_editor_state_s* const state,
	const Rectangle area);

/// Draws the right panel into its area and acts on it.
void vasal_editor_panel_right(vasal_editor_state_s* const state,
	const Rectangle area);

#endif

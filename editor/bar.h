
/// The editor's bars: the top bar with the pages (map, rules, server), the
/// file and history buttons, the map tools and the map view's switches, all
/// with their keys in their tooltips; the status bar with the tile under
/// the pointer, its terrain's speed and sight, the zoom, the brush and
/// what is unsaved; and the messages, shown for a few seconds over the
/// bottom of the window.

#ifndef vasal__editor__bar_h
#define vasal__editor__bar_h

#include "editor/state.h"

#include <raylib.h>

/// The heights of the bars, in pixels.
#define vasal_editor_bar_top 46.0f
#define vasal_editor_bar_bottom 26.0f

/// Draws the top bar across the window and acts on it.
void vasal_editor_bar_top_update(vasal_editor_state_s* const state,
	const float width);

/// Draws the status bar along the bottom of the window.
void vasal_editor_bar_status(const vasal_editor_state_s* const state,
	const float width, const float height);

/// Draws the message, if one shows, over the bottom middle of an area, and
/// counts down its time.
void vasal_editor_bar_message(vasal_editor_state_s* const state,
	const Rectangle area, const float seconds);

#endif

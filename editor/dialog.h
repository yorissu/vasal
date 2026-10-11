
/// The editor's dialogs, drawn over a dimmed window: a new map's name and
/// size, with common sizes to pick; every key and mouse button; and
/// whether to throw away unsaved changes.

#ifndef vasal__editor__dialog_h
#define vasal__editor__dialog_h

#include "editor/state.h"

/// Draws the open dialog over a window of the given size and acts on it.
/// The new map dialog makes the map itself. Returns true when the discard
/// dialog was confirmed, so what waits on it goes ahead.
bool_t vasal_editor_dialog_update(vasal_editor_state_s* const state,
	const float width, const float height);

#endif

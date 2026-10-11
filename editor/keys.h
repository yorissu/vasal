
/// The editor's keys: the commands everywhere (saving, opening, undo,
/// redo, help) and, on the map page, the tools, terrain types, brush,
/// view switches and the selection's keys. Keys typed into a box are
/// text, not commands.

#ifndef vasal__editor__keys_h
#define vasal__editor__keys_h

#include "editor/state.h"

/// Reads the keys pressed this frame and acts on them, or asks for their
/// commands. seconds is the frame's time, for panning.
void vasal_editor_keys_update(vasal_editor_state_s* const state,
	const float seconds);

#endif

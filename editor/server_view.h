
/// The server page of the editor: the game server's address and admin key,
/// uploading the map and the rules to it, fetching what it plays, and a
/// log of what the server answered.

#ifndef vasal__editor__server_view_h
#define vasal__editor__server_view_h

#include "editor/state.h"

#include <raylib.h>

/// Draws the page into its area and acts on it.
void vasal_editor_server_view_update(vasal_editor_state_s* const state,
	const Rectangle area);

/// Adds a line to the log, good or not.
void vasal_editor_server_view_log(const char* const text, const bool_t good);

#endif

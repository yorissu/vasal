
/// The rules page of the editor: on the left, the rules' sections and the
/// unit types, which can be added, copied, removed and reordered; in the
/// middle, the chosen section's values, each with its help text: a unit
/// type's name, color and fields, with what they amount to in play; the
/// starting army's vintaines and centenar; the bandits of a camp; the
/// settlement types' defaults and offers; and the combat values.

#ifndef vasal__editor__rules_view_h
#define vasal__editor__rules_view_h

#include "editor/state.h"

#include <raylib.h>

/// Draws the rules' sections and unit types into the left area, and the
/// chosen section into the page area, and acts on them.
void vasal_editor_rules_view_update(vasal_editor_state_s* const state,
	const Rectangle left, const Rectangle page);

#endif

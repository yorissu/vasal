
/// The state's small helpers, shared by the editor's parts.

#include "editor/state.h"
#include "editor/document.h"
#include "editor/ruleset.h"

void vasal_editor_state_say(vasal_editor_state_s* const state,
	const char* const text, const bool_t failure) {
	size_t length = 0U;
	while (('\0' != text[length])
			&& ((length + 1U) < vasal_editor_state_message_capacity)) {
		state->message[length] = text[length];
		length++;
	}
	state->message[length] = '\0';
	state->message_seconds = failure ? 6.0f : 3.0f;
	state->failure = failure;
}

void vasal_editor_state_select(vasal_editor_state_s* const state,
	const bool_t selected, const vasal_editor_edits_kind_e kind,
	const uint16_t index) {
	state->selected = selected;
	state->selection_kind = kind;
	state->selection = index;
}

// spec: editor.files#3
bool_t vasal_editor_state_map_unsaved(const vasal_editor_state_s* const state) {
	return state->map_saved != vasal_editor_document_revision();
}

// spec: editor.files#3
bool_t vasal_editor_state_rules_unsaved(
	const vasal_editor_state_s* const state) {
	return state->rules_saved != vasal_editor_ruleset_revision();
}

vasal_editor_state_tool_e vasal_editor_state_kind_tool(
	const vasal_editor_edits_kind_e kind) {
	vasal_editor_state_tool_e tool = vasal_editor_state_tool_decoration;
	if (vasal_editor_edits_settlement == kind) {
		tool = vasal_editor_state_tool_settlement;
	} else if (vasal_editor_edits_spawn == kind) {
		tool = vasal_editor_state_tool_spawn;
	} else if (vasal_editor_edits_camp == kind) {
		tool = vasal_editor_state_tool_camp;
	} else {
		// Decorations.
	}
	return tool;
}

vasal_editor_edits_kind_e vasal_editor_state_tool_kind(
	const vasal_editor_state_tool_e tool) {
	vasal_editor_edits_kind_e kind = vasal_editor_edits_settlement;
	if (vasal_editor_state_tool_spawn == tool) {
		kind = vasal_editor_edits_spawn;
	} else if (vasal_editor_state_tool_camp == tool) {
		kind = vasal_editor_edits_camp;
	} else if (vasal_editor_state_tool_decoration == tool) {
		kind = vasal_editor_edits_decoration;
	} else {
		// Settlements, and the tools that place nothing.
	}
	return kind;
}

bool_t vasal_editor_state_places(const vasal_editor_state_tool_e tool) {
	return (vasal_editor_state_tool_settlement == tool)
		|| (vasal_editor_state_tool_spawn == tool)
		|| (vasal_editor_state_tool_camp == tool)
		|| (vasal_editor_state_tool_decoration == tool);
}

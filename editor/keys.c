
/// Keys held with Control are commands; plain keys are tools and switches.
/// Arrows move the selection when there is one, else they pan.

#include "editor/keys.h"
#include "editor/canvas.h"
#include "editor/document.h"
#include "editor/form.h"

#include <raylib.h>

// The tools' keys, in the order of vasal_editor_state_tool_e.
static const int _vasal_editor_keys_tools[] = {
	KEY_V, KEY_B, KEY_F, KEY_I, KEY_S, KEY_P, KEY_C, KEY_D
};

static bool_t _vasal_editor_keys_control(void) {
	return IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)
		|| IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
}

static bool_t _vasal_editor_keys_shift(void) {
	return IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
}

// The commands held with Control, on every page.
// spec: editor.tools#3 editor.files
static void _vasal_editor_keys_commands(vasal_editor_state_s* const state) {
	if (IsKeyPressed(KEY_Z)) {
		state->command = _vasal_editor_keys_shift()
			? vasal_editor_state_command_redo
			: vasal_editor_state_command_undo;
	} else if (IsKeyPressed(KEY_Y)) {
		state->command = vasal_editor_state_command_redo;
	} else if (IsKeyPressed(KEY_S)) {
		state->command = vasal_editor_state_command_save;
	} else if (IsKeyPressed(KEY_O)) {
		state->command = vasal_editor_state_command_open;
	} else if (IsKeyPressed(KEY_D)
			&& (vasal_editor_state_tab_map == state->tab)) {
		vasal_editor_canvas_duplicate(state);
	} else {
		// No command.
	}
}

// The selection's keys: moving it, deleting it, letting it go.
// spec: editor.tools#8
static void _vasal_editor_keys_selection(vasal_editor_state_s* const state) {
	const int32_t step = _vasal_editor_keys_shift() ? 8 : 1;
	if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
		vasal_editor_canvas_nudge(state, -step, 0);
	}
	if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
		vasal_editor_canvas_nudge(state, step, 0);
	}
	if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) {
		vasal_editor_canvas_nudge(state, 0, -step);
	}
	if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
		vasal_editor_canvas_nudge(state, 0, step);
	}
	if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE)) {
		vasal_editor_canvas_delete(state);
	}
}

// Panning with the arrows, when nothing is selected.
static void _vasal_editor_keys_pan(vasal_editor_state_s* const state,
		const float seconds) {
	const float step = (700.0f * seconds) / state->zoom;
	if (IsKeyDown(KEY_LEFT)) {
		state->view_x -= step;
	}
	if (IsKeyDown(KEY_RIGHT)) {
		state->view_x += step;
	}
	if (IsKeyDown(KEY_UP)) {
		state->view_y -= step;
	}
	if (IsKeyDown(KEY_DOWN)) {
		state->view_y += step;
	}
}

// The terrain types' number keys, and the brush's size.
// spec: editor.tools#10
static void _vasal_editor_keys_paint(vasal_editor_state_s* const state) {
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	static const int digits[9] = {
		KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE, KEY_SIX, KEY_SEVEN,
		KEY_EIGHT, KEY_NINE
	};
	for (uint8_t terrain = 0U; terrain < 9U; terrain++) {
		if ((terrain < map->terrain_count) && IsKeyPressed(digits[terrain])) {
			state->terrain = terrain;
			if ((vasal_editor_state_tool_paint != state->tool)
					&& (vasal_editor_state_tool_fill != state->tool)) {
				state->tool = vasal_editor_state_tool_paint;
			}
		}
	}
	if ((IsKeyPressed(KEY_LEFT_BRACKET)
			|| IsKeyPressedRepeat(KEY_LEFT_BRACKET))
			&& (0 < state->brush.radius)) {
		state->brush.radius--;
	}
	if ((IsKeyPressed(KEY_RIGHT_BRACKET)
			|| IsKeyPressedRepeat(KEY_RIGHT_BRACKET))
			&& (vasal_editor_edits_brush_max > state->brush.radius)) {
		state->brush.radius++;
	}
}

// The map page's plain keys.
// spec: editor.tools editor.tools#9
static void _vasal_editor_keys_map(vasal_editor_state_s* const state,
		const float seconds) {
	for (size_t t = 0U; t < (size_t)vasal_editor_state_tools; t++) {
		if (IsKeyPressed(_vasal_editor_keys_tools[t])) {
			state->tool = (vasal_editor_state_tool_e)t;
		}
	}
	_vasal_editor_keys_paint(state);
	if (IsKeyPressed(KEY_G)) {
		state->grid = !state->grid;
	}
	if (IsKeyPressed(KEY_M)) {
		state->minimap = !state->minimap;
	}
	if (IsKeyPressed(KEY_O)) {
		uint32_t next = (uint32_t)state->overlay + 1U;
		if ((uint32_t)vasal_editor_state_overlays <= next) {
			next = 0U;
		}
		state->overlay = (vasal_editor_state_overlay_e)next;
	}
	if (IsKeyPressed(KEY_HOME)) {
		state->command = vasal_editor_state_command_fit;
	}
	if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) {
		vasal_editor_canvas_zoom(state, 1.5f);
	}
	if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
		vasal_editor_canvas_zoom(state, 1.0f / 1.5f);
	}
	if (IsKeyPressed(KEY_ESCAPE)) {
		state->selected = false;
	}
	if (state->selected) {
		_vasal_editor_keys_selection(state);
	} else {
		_vasal_editor_keys_pan(state, seconds);
	}
}

void vasal_editor_keys_update(vasal_editor_state_s* const state,
	const float seconds) {
	if (vasal_editor_form_typing() || vasal_editor_form_dropped()
			|| (vasal_editor_state_dialog_none != state->dialog)) {
		return;
	}
	if (_vasal_editor_keys_control()) {
		_vasal_editor_keys_commands(state);
		return;
	}
	if (IsKeyPressed(KEY_F1)) {
		state->dialog = vasal_editor_state_dialog_help;
	}
	if (IsKeyPressed(KEY_N)) {
		state->command = (vasal_editor_state_tab_rules == state->tab)
			? vasal_editor_state_command_new_rules
			: vasal_editor_state_command_new_map;
	}
	if (vasal_editor_state_tab_map == state->tab) {
		_vasal_editor_keys_map(state, seconds);
	}
}

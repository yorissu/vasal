
/// Escape closes a dialog, Enter confirms it.

#include "editor/dialog.h"
#include "editor/document.h"
#include "editor/form.h"
#include "editor/theme.h"

#include <raygui.h>

#include <math.h>
#include <string.h>

static char _vasal_editor_dialog_name[vasal_engine_map_name_capacity];
static bool_t _vasal_editor_dialog_named = false;
static int _vasal_editor_dialog_width = 512;
static int _vasal_editor_dialog_height = 512;
static float _vasal_editor_dialog_scroll = 0.0f;

// The keys and what they do, in two columns.
static const char* const _vasal_editor_dialog_keys[][2] = {
	{ "V", "Select and move objects" },
	{ "B", "Paint terrain" },
	{ "F", "Fill an area" },
	{ "I", "Pick a terrain from the map" },
	{ "S P C D", "Place settlements, spawns, camps, decorations" },
	{ "1 to 9", "Choose a terrain type" },
	{ "[ ]", "Smaller or larger brush" },
	{ "Shift+wheel", "Brush size" },
	{ "Shift+click", "Paint a line from the last point" },
	{ "Alt+click", "Pick the terrain under the pointer" },
	{ "Wheel", "Zoom around the pointer" },
	{ "Right or middle drag", "Pan; also Space and drag" },
	{ "Arrows", "Pan; move the selection by 1 (Shift: 8)" },
	{ "Home", "Show the whole map" },
	{ "G M O", "Grid, minimap, ground colors" },
	{ "Ctrl+D", "Copy the selection" },
	{ "Delete", "Delete the selection" },
	{ "Escape", "Select nothing" },
	{ "Ctrl+Z, Ctrl+Y", "Undo, redo" },
	{ "Ctrl+S, Ctrl+O", "Save, open (or drop a file)" },
	{ "N", "New map" },
	{ "F1", "This help" }
};

// A dialog's box in the middle of the window, its title drawn. Returns its
// inside.
static Rectangle _vasal_editor_dialog_box(const float width,
		const float height, const float box_width, const float box_height,
		const char* const title) {
	DrawRectangle(0, 0, (int)width, (int)height, (Color){ 0, 0, 0, 150 });
	const Rectangle box = {
		(width - box_width) / 2.0f, (height - box_height) / 2.0f, box_width,
		box_height
	};
	DrawRectangleRounded((Rectangle){ box.x, box.y + 6.0f, box.width,
		box.height }, 0.04f, 8, (Color){ 0, 0, 0, 90 });
	DrawRectangleRounded(box, 0.04f, 8, vasal_editor_theme_panel);
	DrawRectangleRoundedLinesEx(box, 0.04f, 8, 1.0f,
		vasal_editor_theme_border);
	vasal_editor_theme_text(title, box.x + 20.0f, box.y + 16.0f,
		vasal_editor_theme_large, true, vasal_editor_theme_foreground);
	const Rectangle inside = {
		box.x + 20.0f, box.y + 52.0f, box.width - 40.0f, box.height - 68.0f
	};
	return inside;
}

// A new map's name and size.
// spec: editor.files
static void _vasal_editor_dialog_new_map(vasal_editor_state_s* const state,
		const float width, const float height) {
	if (!_vasal_editor_dialog_named) {
		static const char name[] = "new map";
		(void)memcpy(_vasal_editor_dialog_name, name, sizeof(name));
		_vasal_editor_dialog_named = true;
	}
	const Rectangle inside = _vasal_editor_dialog_box(width, height, 440.0f,
		300.0f, "New map");
	vasal_editor_form_s form;
	float scroll = 0.0f;
	vasal_editor_form_begin(&form, inside, &scroll, 0.0f);
	(void)vasal_editor_form_text(&form, "Name", _vasal_editor_dialog_name,
		sizeof(_vasal_editor_dialog_name), 0x400001U);
	(void)vasal_editor_form_number(&form, "Width", NULL,
		&_vasal_editor_dialog_width, 16, (int)vasal_engine_map_side_max,
		0x400002U);
	(void)vasal_editor_form_number(&form, "Height", NULL,
		&_vasal_editor_dialog_height, 16, (int)vasal_engine_map_side_max,
		0x400003U);
	static const int sizes[] = { 256, 512, 1024, 2048 };
	const Rectangle presets = vasal_editor_form_take(&form,
		vasal_editor_form_row);
	for (size_t i = 0U; i < 4U; i++) {
		if (0 != GuiButton(vasal_editor_form_part(presets, 4U, i),
				TextFormat("%d", sizes[i]))) {
			_vasal_editor_dialog_width = sizes[i];
			_vasal_editor_dialog_height = sizes[i];
		}
	}
	vasal_editor_form_paragraph(&form, "It starts as grass, with the "
		"standard terrain types ready to paint. A tile is a few troops wide.");
	vasal_editor_form_space(&form, 8.0f);
	const Rectangle row = vasal_editor_form_take(&form, 32.0f);
	const bool_t cancel = (0 != GuiButton(vasal_editor_form_part(row, 2U, 0U),
		"Cancel")) || IsKeyPressed(KEY_ESCAPE);
	const bool_t create = (0 != GuiButton(vasal_editor_form_part(row, 2U, 1U),
		GuiIconText(ICON_OK_TICK, "Create"))) || (IsKeyPressed(KEY_ENTER)
		&& !vasal_editor_form_typing());
	vasal_editor_form_end(&form);
	if (create && vasal_editor_document_create(
			(uint16_t)_vasal_editor_dialog_width,
			(uint16_t)_vasal_editor_dialog_height)) {
		vasal_engine_map_s* const map = vasal_editor_document_map();
		(void)memcpy(map->name, _vasal_editor_dialog_name, sizeof(map->name));
		state->map_saved = vasal_editor_document_revision();
		state->selected = false;
		state->terrain = 0U;
		state->painted = false;
		state->command = vasal_editor_state_command_fit;
		state->tab = vasal_editor_state_tab_map;
		vasal_editor_state_say(state, "New map made.", false);
	}
	if (create || cancel) {
		state->dialog = vasal_editor_state_dialog_none;
		vasal_editor_form_release();
	}
}

// Every key and mouse button.
static void _vasal_editor_dialog_help(vasal_editor_state_s* const state,
		const float width, const float height) {
	const float box_height = fminf(height - 80.0f, 620.0f);
	const Rectangle inside = _vasal_editor_dialog_box(width, height, 620.0f,
		box_height, "Keys and mouse");
	vasal_editor_form_s form;
	vasal_editor_form_begin(&form, (Rectangle){ inside.x, inside.y,
		inside.width, inside.height - 40.0f }, &_vasal_editor_dialog_scroll,
		0.0f);
	const size_t count = sizeof(_vasal_editor_dialog_keys)
		/ sizeof(_vasal_editor_dialog_keys[0]);
	for (size_t i = 0U; i < count; i++) {
		const Rectangle row = vasal_editor_form_take(&form, 20.0f);
		vasal_editor_theme_text(_vasal_editor_dialog_keys[i][0], row.x,
			row.y, vasal_editor_theme_normal, true, vasal_editor_theme_accent);
		vasal_editor_theme_text(_vasal_editor_dialog_keys[i][1],
			row.x + 190.0f, row.y, vasal_editor_theme_normal, false,
			vasal_editor_theme_foreground);
	}
	vasal_editor_form_end(&form);
	const bool_t close = (0 != GuiButton((Rectangle){
		(inside.x + inside.width) - 120.0f, (inside.y + inside.height)
		- 32.0f, 120.0f, 32.0f }, "Close")) || IsKeyPressed(KEY_ESCAPE)
		|| IsKeyPressed(KEY_F1) || IsKeyPressed(KEY_ENTER);
	if (close) {
		state->dialog = vasal_editor_state_dialog_none;
	}
}

// Whether to throw away unsaved changes. Returns whether that was
// confirmed.
static bool_t _vasal_editor_dialog_discard(vasal_editor_state_s* const state,
		const float width, const float height) {
	const bool_t rules = vasal_editor_state_pending_new_rules
		== state->pending;
	const Rectangle inside = _vasal_editor_dialog_box(width, height, 460.0f,
		190.0f, "Unsaved changes");
	const char* text = "The map has changes that are not saved. Throw "
		"them away?";
	if (rules) {
		text = "The rules have changes that are not saved. Throw them away?";
	} else if (vasal_editor_state_pending_open == state->pending) {
		text = "There are changes that are not saved. A file opened takes "
			"the place of the map or the rules. Throw the changes away?";
	} else {
		// A new map.
	}
	(void)vasal_editor_theme_wrapped(text, inside.x, inside.y, inside.width,
		vasal_editor_theme_normal, vasal_editor_theme_foreground);
	const Rectangle row = {
		inside.x, (inside.y + inside.height) - 32.0f, inside.width, 32.0f
	};
	const bool_t cancel = (0 != GuiButton(vasal_editor_form_part(row, 2U, 0U),
		"Keep them")) || IsKeyPressed(KEY_ESCAPE);
	const bool_t discard = (0 != GuiButton(vasal_editor_form_part(row, 2U,
		1U), GuiIconText(ICON_BIN, "Throw away")))
		|| IsKeyPressed(KEY_ENTER);
	if (cancel || discard) {
		state->dialog = vasal_editor_state_dialog_none;
	}
	if (cancel) {
		state->pending = vasal_editor_state_pending_none;
	}
	return discard && !cancel;
}

bool_t vasal_editor_dialog_update(vasal_editor_state_s* const state,
	const float width, const float height) {
	bool_t confirmed = false;
	if (vasal_editor_state_dialog_new_map == state->dialog) {
		_vasal_editor_dialog_new_map(state, width, height);
	} else if (vasal_editor_state_dialog_help == state->dialog) {
		_vasal_editor_dialog_help(state, width, height);
	} else if (vasal_editor_state_dialog_discard == state->dialog) {
		confirmed = _vasal_editor_dialog_discard(state, width, height);
	} else {
		// No dialog.
	}
	return confirmed;
}

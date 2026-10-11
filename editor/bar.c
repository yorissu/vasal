
/// The bars' buttons only ask for commands (editor/state.h); the editor
/// carries them out at the end of the frame, the same way as for keys.

#include "editor/bar.h"
#include "editor/document.h"
#include "editor/ruleset.h"
#include "editor/theme.h"

#include <raygui.h>

#include <math.h>

#define _vasal_editor_bar_button 32.0f

// The tools: their icons and tooltips, in the order of
// vasal_editor_state_tool_e.
static const int _vasal_editor_bar_tool_icons[] = {
	ICON_CURSOR_POINTER, ICON_BRUSH_CLASSIC, ICON_COLOR_BUCKET,
	ICON_COLOR_PICKER, ICON_HOUSE, ICON_PLAYER, ICON_DEMON,
	ICON_FILETYPE_IMAGE
};
static const char* const _vasal_editor_bar_tool_tips[] = {
	"Select and move objects (V)", "Paint terrain (B)",
	"Fill an area with terrain (F)", "Pick a terrain from the map (I)",
	"Place settlements (S)", "Place spawn areas (P)",
	"Place bandit camps (C)", "Place decorations (D)"
};

// An icon button with a tooltip; active ones are drawn pressed. Returns
// whether it was clicked.
static bool_t _vasal_editor_bar_icon(const float x, const float y,
		const int icon, const char* const tip, const bool_t active) {
	const Rectangle bounds = {
		x, y, _vasal_editor_bar_button, _vasal_editor_bar_button
	};
	GuiSetTooltip(tip);
	bool pressed = active;
	const bool_t clicked = 0 != GuiToggle(bounds, GuiIconText(icon, NULL),
		&pressed);
	GuiSetTooltip(NULL);
	return clicked;
}

// A thin line between groups of buttons.
static float _vasal_editor_bar_separator(const float x) {
	DrawRectangleRec((Rectangle){ x + 6.0f, 10.0f, 1.0f,
		vasal_editor_bar_top - 20.0f }, vasal_editor_theme_border);
	return x + 13.0f;
}

// The pages, as tabs. Returns where the next control goes.
static float _vasal_editor_bar_tabs(vasal_editor_state_s* const state,
		const float start) {
	static const char* const names[] = { "Map", "Rules", "Server" };
	float x = start;
	for (size_t t = 0U; t < (size_t)vasal_editor_state_tabs; t++) {
		const float width = vasal_editor_theme_measure(names[t],
			vasal_editor_theme_normal, true) + 28.0f;
		const Rectangle bounds = { x, 0.0f, width, vasal_editor_bar_top };
		const bool_t active = (size_t)state->tab == t;
		const bool_t hovered = !GuiIsLocked()
			&& CheckCollisionPointRec(GetMousePosition(), bounds);
		if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			state->tab = (vasal_editor_state_tab_e)t;
		}
		const Color color = (active || hovered) ? vasal_editor_theme_foreground
			: vasal_editor_theme_dim;
		vasal_editor_theme_text(names[t], x + 14.0f, 14.0f,
			vasal_editor_theme_normal, active, color);
		if (active) {
			DrawRectangleRec((Rectangle){ x + 8.0f, vasal_editor_bar_top
				- 3.0f, width - 16.0f, 3.0f }, vasal_editor_theme_accent);
		}
		x += width;
	}
	return x;
}

// New, open, save, undo and redo. Returns where the next control goes.
// spec: editor.files editor.tools#3
static float _vasal_editor_bar_files(vasal_editor_state_s* const state,
		const float start) {
	const float y = (vasal_editor_bar_top - _vasal_editor_bar_button) / 2.0f;
	float x = start;
	const bool_t rules = vasal_editor_state_tab_rules == state->tab;
	if (_vasal_editor_bar_icon(x, y, ICON_FILE_NEW, rules
			? "New standard rules" : "New map (N)", false)) {
		state->command = rules ? vasal_editor_state_command_new_rules
			: vasal_editor_state_command_new_map;
	}
	x += _vasal_editor_bar_button + 4.0f;
	if (_vasal_editor_bar_icon(x, y, ICON_FILE_OPEN,
			"Open a map or rules file (Ctrl+O), or drop one here", false)) {
		state->command = vasal_editor_state_command_open;
	}
	x += _vasal_editor_bar_button + 4.0f;
	const bool_t unsaved = rules ? vasal_editor_state_rules_unsaved(state)
		: vasal_editor_state_map_unsaved(state);
	if (_vasal_editor_bar_icon(x, y, ICON_FILE_SAVE, rules
			? "Save the rules (Ctrl+S)" : "Save the map (Ctrl+S)", false)) {
		state->command = vasal_editor_state_command_save;
	}
	if (unsaved) {
		DrawCircleV((Vector2){ x + _vasal_editor_bar_button - 5.0f, y + 5.0f },
			4.0f, vasal_editor_theme_warning);
	}
	x = _vasal_editor_bar_separator(x + _vasal_editor_bar_button);
	if (_vasal_editor_bar_icon(x, y, ICON_UNDO, "Undo (Ctrl+Z)", false)) {
		state->command = vasal_editor_state_command_undo;
	}
	x += _vasal_editor_bar_button + 4.0f;
	if (_vasal_editor_bar_icon(x, y, ICON_REDO, "Redo (Ctrl+Y)", false)) {
		state->command = vasal_editor_state_command_redo;
	}
	return x + _vasal_editor_bar_button;
}

// The map tools. Returns where the next control goes.
// spec: editor.tools
static float _vasal_editor_bar_tools(vasal_editor_state_s* const state,
		const float start) {
	const float y = (vasal_editor_bar_top - _vasal_editor_bar_button) / 2.0f;
	float x = _vasal_editor_bar_separator(start);
	for (size_t t = 0U; t < (size_t)vasal_editor_state_tools; t++) {
		const vasal_editor_state_tool_e tool = (vasal_editor_state_tool_e)t;
		if (_vasal_editor_bar_icon(x, y, _vasal_editor_bar_tool_icons[t],
				_vasal_editor_bar_tool_tips[t], tool == state->tool)) {
			state->tool = tool;
		}
		x += _vasal_editor_bar_button + 4.0f;
		if ((vasal_editor_state_tool_pick == tool)
				|| (vasal_editor_state_tool_select == tool)) {
			x = _vasal_editor_bar_separator(x - 4.0f);
		}
	}
	return x;
}

// The map view's switches, from the right edge leftward.
// spec: editor.tools#9
static void _vasal_editor_bar_view(vasal_editor_state_s* const state,
		const float width) {
	const float y = (vasal_editor_bar_top - _vasal_editor_bar_button) / 2.0f;
	float x = width - 12.0f - _vasal_editor_bar_button;
	if (_vasal_editor_bar_icon(x, y, ICON_HELP, "Keys and mouse (F1)",
			vasal_editor_state_dialog_help == state->dialog)) {
		state->dialog = vasal_editor_state_dialog_help;
	}
	if (vasal_editor_state_tab_map != state->tab) {
		return;
	}
	x -= _vasal_editor_bar_button + 13.0f;
	if (_vasal_editor_bar_icon(x, y, ICON_ZOOM_ALL, "Show the whole map "
			"(Home)", false)) {
		state->command = vasal_editor_state_command_fit;
	}
	x -= _vasal_editor_bar_button + 4.0f;
	if (_vasal_editor_bar_icon(x, y, ICON_MAPS, "Minimap (M)",
			state->minimap)) {
		state->minimap = !state->minimap;
	}
	x -= _vasal_editor_bar_button + 4.0f;
	if (_vasal_editor_bar_icon(x, y, ICON_GRID, "Grid (G)", state->grid)) {
		state->grid = !state->grid;
	}
	x -= 222.0f;
	int overlay = (int)state->overlay;
	GuiSetTooltip("Color the ground by terrain, speed or sight (O)");
	(void)GuiToggleGroup((Rectangle){ x, y, 70.0f, _vasal_editor_bar_button },
		"Terrain;Speed;Sight", &overlay);
	GuiSetTooltip(NULL);
	state->overlay = (vasal_editor_state_overlay_e)overlay;
}

// spec: editor.tools
void vasal_editor_bar_top_update(vasal_editor_state_s* const state,
	const float width) {
	DrawRectangleRec((Rectangle){ 0.0f, 0.0f, width, vasal_editor_bar_top },
		vasal_editor_theme_panel);
	DrawRectangleRec((Rectangle){ 0.0f, vasal_editor_bar_top - 1.0f, width,
		1.0f }, vasal_editor_theme_border);
	vasal_editor_theme_text("VASAL", 16.0f, 12.0f, vasal_editor_theme_large,
		true, vasal_editor_theme_accent);
	float x = _vasal_editor_bar_separator(92.0f);
	x = _vasal_editor_bar_tabs(state, x);
	x = _vasal_editor_bar_separator(x);
	x = _vasal_editor_bar_files(state, x);
	if (vasal_editor_state_tab_map == state->tab) {
		(void)_vasal_editor_bar_tools(state, x);
	}
	_vasal_editor_bar_view(state, width);
}

// spec: editor.tools#9
void vasal_editor_bar_status(const vasal_editor_state_s* const state,
	const float width, const float height) {
	const float y = height - vasal_editor_bar_bottom;
	DrawRectangleRec((Rectangle){ 0.0f, y, width, vasal_editor_bar_bottom },
		vasal_editor_theme_panel);
	DrawRectangleRec((Rectangle){ 0.0f, y, width, 1.0f },
		vasal_editor_theme_border);
	const float text_y = y + 6.0f;
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	if ((vasal_editor_state_tab_map == state->tab) && state->hovering) {
		const vasal_engine_map_terrain_s* const terrain =
			vasal_engine_map_terrain(map, (uint16_t)state->hover_x,
				(uint16_t)state->hover_y);
		const char* const values = terrain->passable
			? TextFormat("speed %u%%, sight %u%%",
				(unsigned int)terrain->speed, (unsigned int)terrain->sight)
			: TextFormat("impassable, sight %u%%",
				(unsigned int)terrain->sight);
		vasal_editor_theme_text(TextFormat("%d, %d   %s (%s)",
			(int)state->hover_x, (int)state->hover_y, terrain->name, values),
			12.0f, text_y, vasal_editor_theme_small, false,
			vasal_editor_theme_foreground);
	}
	if (vasal_editor_state_tab_map == state->tab) {
		const char* const view = TextFormat("zoom %d%%   brush %d",
			(int)lroundf(state->zoom * 100.0f), (int)state->brush.radius);
		vasal_editor_theme_text(view, width * 0.45f, text_y,
			vasal_editor_theme_small, false, vasal_editor_theme_dim);
	}
	const bool_t map_unsaved = vasal_editor_state_map_unsaved(state);
	const bool_t rules_unsaved = vasal_editor_state_rules_unsaved(state);
	const char* const saved = TextFormat("map %s   rules %s",
		map_unsaved ? "unsaved" : "saved",
		rules_unsaved ? "unsaved" : "saved");
	vasal_editor_theme_text(saved, width - 12.0f
		- vasal_editor_theme_measure(saved, vasal_editor_theme_small, false),
		text_y, vasal_editor_theme_small, false, (map_unsaved || rules_unsaved)
		? vasal_editor_theme_warning : vasal_editor_theme_dim);
}

void vasal_editor_bar_message(vasal_editor_state_s* const state,
	const Rectangle area, const float seconds) {
	if (('\0' == state->message[0]) || (0.0f >= state->message_seconds)) {
		return;
	}
	const float alpha = fminf(1.0f, state->message_seconds * 2.0f);
	const float width = fminf(vasal_editor_theme_measure(state->message,
		vasal_editor_theme_normal, false) + 52.0f, area.width - 24.0f);
	const Rectangle toast = {
		area.x + ((area.width - width) / 2.0f),
		(area.y + area.height) - 56.0f, width, 38.0f
	};
	DrawRectangleRounded((Rectangle){ toast.x, toast.y + 3.0f, toast.width,
		toast.height }, 0.4f, 8, ColorAlpha(BLACK, 0.3f * alpha));
	DrawRectangleRounded(toast, 0.4f, 8,
		ColorAlpha(vasal_editor_theme_raised, alpha));
	const Color mark = state->failure ? vasal_editor_theme_bad
		: vasal_editor_theme_good;
	GuiDrawIcon(state->failure ? ICON_WARNING : ICON_OK_TICK,
		(int)toast.x + 12, (int)toast.y + 11, 1, ColorAlpha(mark, alpha));
	vasal_editor_theme_text(state->message, toast.x + 38.0f, toast.y + 10.0f,
		vasal_editor_theme_normal, false,
		ColorAlpha(vasal_editor_theme_foreground, alpha));
	state->message_seconds -= seconds;
}

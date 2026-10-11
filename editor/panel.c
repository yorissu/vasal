
/// Every control edits a copy of its value; only when the copy differs is
/// the change made, inside a document operation, so the history records
/// real changes only. Repeated changes to the same field share a key and
/// merge into one operation. Text and number boxes are edited one at a
/// time: the panel remembers which.

#include "editor/panel.h"
#include "editor/canvas.h"
#include "editor/document.h"
#include "editor/files.h"
#include "editor/ruleset.h"

#include <raygui.h>

#include <string.h>

#define _vasal_editor_panel_row 28.0f
#define _vasal_editor_panel_gap 6.0f
#define _vasal_editor_panel_margin 12.0f

// Fields, for merge keys and for which box is being edited.
enum {
	_vasal_editor_panel_none,
	_vasal_editor_panel_map_name,
	_vasal_editor_panel_width_field,
	_vasal_editor_panel_height_field,
	_vasal_editor_panel_terrain_name,
	_vasal_editor_panel_terrain_speed,
	_vasal_editor_panel_terrain_color,
	_vasal_editor_panel_terrain_passable,
	_vasal_editor_panel_terrain_sight,
	_vasal_editor_panel_name,
	_vasal_editor_panel_type,
	_vasal_editor_panel_radius,
	_vasal_editor_panel_requirement,
	_vasal_editor_panel_capture,
	_vasal_editor_panel_bonus,
	_vasal_editor_panel_income,
	_vasal_editor_panel_flags,
	_vasal_editor_panel_kind
};

// Where the next control goes.
typedef struct _vasal_editor_panel_cursor_s {
	float x;
	float y;
	float width;
} _vasal_editor_panel_cursor_s;

static int _vasal_editor_panel_editing = _vasal_editor_panel_none;
static int _vasal_editor_panel_new_width = 512;
static int _vasal_editor_panel_new_height = 512;
static int _vasal_editor_panel_terrain_scroll = 0;

static const char* const _vasal_editor_panel_problems[] = {
	"The map has no settlement.",
	"The map has no spawn area.",
	"A settlement stands on impassable ground.",
	"A spawn area is on impassable ground.",
	"Two capture areas overlap."
};

static const char* const _vasal_editor_panel_flag_names[] = {
	"Centenars", "Spearmen", "Swordsmen", "Archers", "Throwers", "Scouts"
};

static Rectangle _vasal_editor_panel_take(
		_vasal_editor_panel_cursor_s* const cursor, const float height) {
	const Rectangle bounds = { cursor->x, cursor->y, cursor->width, height };
	cursor->y += height + _vasal_editor_panel_gap;
	return bounds;
}

// A merge key for a field of an object.
static uint32_t _vasal_editor_panel_key(const int field,
		const uint32_t index) {
	return ((uint32_t)field << 16U) | (index + 1U);
}

static void _vasal_editor_panel_toggle_editing(const int field) {
	_vasal_editor_panel_editing = (_vasal_editor_panel_editing == field)
		? _vasal_editor_panel_none : field;
}

// A number box: returns whether the value was changed.
static bool_t _vasal_editor_panel_number(
		_vasal_editor_panel_cursor_s* const cursor, const char* const label,
		int* const value, const int minimum, const int maximum,
		const int field) {
	(void)GuiLabel(_vasal_editor_panel_take(cursor, 18.0f), label);
	const int before = *value;
	if (0 != GuiValueBox(_vasal_editor_panel_take(cursor,
			_vasal_editor_panel_row), NULL, value, minimum, maximum,
			_vasal_editor_panel_editing == field)) {
		_vasal_editor_panel_toggle_editing(field);
	}
	return before != *value;
}

// A text box over a name: returns whether the name was changed.
static bool_t _vasal_editor_panel_text(
		_vasal_editor_panel_cursor_s* const cursor, const char* const label,
		char* const name, const int field) {
	(void)GuiLabel(_vasal_editor_panel_take(cursor, 18.0f), label);
	char edited[vasal_engine_map_name_capacity];
	(void)memcpy(edited, name, sizeof(edited));
	if (0 != GuiTextBox(_vasal_editor_panel_take(cursor,
			_vasal_editor_panel_row), edited, (int)sizeof(edited),
			_vasal_editor_panel_editing == field)) {
		_vasal_editor_panel_toggle_editing(field);
	}
	// Names hold printable ASCII only, like the map file.
	for (size_t i = 0U; '\0' != edited[i]; i++) {
		if ((edited[i] < ' ') || (edited[i] > '~')) {
			edited[i] = '_';
		}
	}
	if (0 == memcmp(edited, name, sizeof(edited))) {
		return false;
	}
	(void)memcpy(name, edited, sizeof(edited));
	return true;
}

static void _vasal_editor_panel_message(vasal_editor_state_s* const state,
		const char* const text) {
	state->message = text;
	state->message_seconds = 3.0f;
}

// Map name, size, new, open, save, undo and redo.
// spec: editor.files#1 editor.tools#3
static void _vasal_editor_panel_file(vasal_editor_state_s* const state,
		_vasal_editor_panel_cursor_s* const cursor, const Rectangle area) {
	vasal_engine_map_s* const map = vasal_editor_document_map();
	(void)GuiLabel(_vasal_editor_panel_take(cursor, 22.0f), "Vasal map editor");
	char name[vasal_engine_map_name_capacity];
	(void)memcpy(name, map->name, sizeof(name));
	if (_vasal_editor_panel_text(cursor, "Map name", name,
			_vasal_editor_panel_map_name)) {
		vasal_editor_document_begin(_vasal_editor_panel_key(
			_vasal_editor_panel_map_name, 0U));
		(void)memcpy(map->name, name, sizeof(name));
		vasal_editor_document_changed();
		vasal_editor_document_end();
	}
	(void)GuiLabel(_vasal_editor_panel_take(cursor, 18.0f),
		TextFormat("%u x %u tiles", (unsigned int)map->width,
			(unsigned int)map->height));

	(void)_vasal_editor_panel_number(cursor, "New map width",
		&_vasal_editor_panel_new_width, 16, (int)vasal_engine_map_side_max,
		_vasal_editor_panel_width_field);
	(void)_vasal_editor_panel_number(cursor, "New map height",
		&_vasal_editor_panel_new_height, 16, (int)vasal_engine_map_side_max,
		_vasal_editor_panel_height_field);
	const Rectangle row = _vasal_editor_panel_take(cursor,
		_vasal_editor_panel_row);
	const float third = (row.width - (2.0f * _vasal_editor_panel_gap)) / 3.0f;
	const float step = third + _vasal_editor_panel_gap;
	if (0 != GuiButton((Rectangle){ row.x, row.y, third, row.height }, "New")) {
		(void)vasal_editor_document_create(
			(uint16_t)_vasal_editor_panel_new_width,
			(uint16_t)_vasal_editor_panel_new_height);
		state->selected = false;
		vasal_editor_canvas_fit(state, area);
	}
	if (0 != GuiButton((Rectangle){ row.x + step, row.y, third, row.height },
			"Open")) {
		vasal_editor_files_choose();
	}
	if (0 != GuiButton((Rectangle){ row.x + (2.0f * step), row.y, third,
			row.height }, "Save")) {
		if (!vasal_editor_files_save(map)) {
			_vasal_editor_panel_message(state, "The map could not be saved.");
		}
	}
	const Rectangle history = _vasal_editor_panel_take(cursor,
		_vasal_editor_panel_row);
	const float half = (history.width - _vasal_editor_panel_gap) / 2.0f;
	if (0 != GuiButton((Rectangle){ history.x, history.y, half,
			history.height }, "Undo")) {
		(void)vasal_editor_document_undo();
	}
	if (0 != GuiButton((Rectangle){ history.x + half + _vasal_editor_panel_gap,
			history.y, half, history.height }, "Redo")) {
		(void)vasal_editor_document_redo();
	}
}

// Replaces the tiles of a removed terrain type with the first type, and
// moves the numbers of the types after it down by one.
static void _vasal_editor_panel_remove_terrain(
		vasal_editor_state_s* const state) {
	vasal_engine_map_s* const map = vasal_editor_document_map();
	const uint8_t removed = state->terrain;
	if ((1U >= map->terrain_count) || (0U == removed)) {
		_vasal_editor_panel_message(state,
			"The first terrain type can't be removed.");
		return;
	}
	vasal_editor_document_begin(0U);
	for (uint16_t y = 0U; y < map->height; y++) {
		for (uint16_t x = 0U; x < map->width; x++) {
			const uint8_t tile = map->tiles[((size_t)y * map->width) + x];
			if (tile == removed) {
				vasal_editor_document_paint(x, y, 0U);
			} else if (tile > removed) {
				vasal_editor_document_paint(x, y, tile - 1U);
			} else {
				// Unchanged.
			}
		}
	}
	for (uint8_t i = removed; (i + 1U) < map->terrain_count; i++) {
		map->terrains[i] = map->terrains[i + 1U];
	}
	map->terrain_count--;
	vasal_editor_document_changed();
	vasal_editor_document_end();
	state->terrain = 0U;
}

// The terrain types, the one painted, its values, and the brush.
// spec: editor.tools#4 map.terrain#9
static void _vasal_editor_panel_terrain(vasal_editor_state_s* const state,
		_vasal_editor_panel_cursor_s* const cursor) {
	vasal_engine_map_s* const map = vasal_editor_document_map();
	static char names[vasal_engine_map_terrains_max
		* vasal_engine_map_name_capacity];
	size_t at = 0U;
	for (size_t i = 0U; i < map->terrain_count; i++) {
		const size_t length = strlen(map->terrains[i].name);
		(void)memcpy(&names[at], map->terrains[i].name, length);
		at += length;
		names[at] = ((i + 1U) < map->terrain_count) ? ';' : '\0';
		at++;
	}
	int active = (int)state->terrain;
	(void)GuiListView(_vasal_editor_panel_take(cursor, 140.0f), names,
		&_vasal_editor_panel_terrain_scroll, &active);
	if ((active >= 0) && (active < (int)map->terrain_count)) {
		state->terrain = (uint8_t)active;
	}

	vasal_engine_map_terrain_s* const terrain = &map->terrains[state->terrain];
	const uint32_t index = state->terrain;
	char name[vasal_engine_map_name_capacity];
	(void)memcpy(name, terrain->name, sizeof(name));
	if (_vasal_editor_panel_text(cursor, "Terrain name", name,
			_vasal_editor_panel_terrain_name)) {
		vasal_editor_document_begin(_vasal_editor_panel_key(
			_vasal_editor_panel_terrain_name, index));
		(void)memcpy(terrain->name, name, sizeof(name));
		vasal_editor_document_changed();
		vasal_editor_document_end();
	}
	bool passable = terrain->passable;
	(void)GuiCheckBox(_vasal_editor_panel_take(cursor, 20.0f), "Passable",
		&passable);
	int speed = (int)terrain->speed;
	const bool_t sped = terrain->passable && _vasal_editor_panel_number(cursor,
		"Speed (percent)", &speed, 1, (int)vasal_engine_map_speed_max,
		_vasal_editor_panel_terrain_speed);
	// raygui writes the value through its pointer, which cppcheck can't
	// see.
	// cppcheck-suppress knownConditionTrueFalse
	if ((passable != terrain->passable) || sped) {
		vasal_editor_document_begin(_vasal_editor_panel_key(
			_vasal_editor_panel_terrain_speed, index));
		terrain->passable = passable;
		terrain->speed = 0U;
		if (passable) {
			terrain->speed = (0 < speed) ? (uint16_t)speed : 100U;
		}
		vasal_editor_document_changed();
		vasal_editor_document_end();
	}
	int sight = (int)terrain->sight;
	// raygui writes the value through its pointer, which cppcheck can't
	// see.
	// cppcheck-suppress knownConditionTrueFalse
	if (_vasal_editor_panel_number(cursor, "Sight (percent)", &sight, 0,
			(int)vasal_engine_map_sight_max,
			_vasal_editor_panel_terrain_sight)) {
		vasal_editor_document_begin(_vasal_editor_panel_key(
			_vasal_editor_panel_terrain_sight, index));
		terrain->sight = (uint16_t)sight;
		vasal_editor_document_changed();
		vasal_editor_document_end();
	}
	Color color = {
		(unsigned char)(terrain->color >> 24U),
		(unsigned char)(terrain->color >> 16U),
		(unsigned char)(terrain->color >> 8U), 255U
	};
	const Rectangle picker = _vasal_editor_panel_take(cursor, 90.0f);
	(void)GuiColorPicker((Rectangle){ picker.x, picker.y,
		picker.width - 34.0f, picker.height }, NULL, &color);
	const uint32_t rgba = ((uint32_t)color.r << 24U)
		| ((uint32_t)color.g << 16U) | ((uint32_t)color.b << 8U) | 0xffU;
	if (rgba != terrain->color) {
		vasal_editor_document_begin(_vasal_editor_panel_key(
			_vasal_editor_panel_terrain_color, index));
		terrain->color = rgba;
		vasal_editor_document_changed();
		vasal_editor_document_end();
	}

	const Rectangle row = _vasal_editor_panel_take(cursor,
		_vasal_editor_panel_row);
	const float half = (row.width - _vasal_editor_panel_gap) / 2.0f;
	if (0 != GuiButton((Rectangle){ row.x, row.y, half, row.height },
			"Add type")) {
		if (vasal_engine_map_terrains_max > map->terrain_count) {
			vasal_editor_document_begin(0U);
			vasal_engine_map_terrain_s* const added =
				&map->terrains[map->terrain_count];
			(void)memcpy(added->name, "new terrain", 12U);
			added->color = 0x808080ffU;
			added->speed = 100U;
			added->sight = 100U;
			added->passable = true;
			state->terrain = map->terrain_count;
			map->terrain_count++;
			vasal_editor_document_changed();
		vasal_editor_document_end();
		}
	}
	if (0 != GuiButton((Rectangle){ row.x + half + _vasal_editor_panel_gap,
			row.y, half, row.height }, "Remove type")) {
		_vasal_editor_panel_remove_terrain(state);
	}
	float brush = (float)state->brush;
	(void)GuiLabel(_vasal_editor_panel_take(cursor, 18.0f),
		TextFormat("Brush radius: %d", state->brush));
	(void)GuiSlider(_vasal_editor_panel_take(cursor, 20.0f), NULL, NULL,
		&brush, 0.0f, 12.0f);
	state->brush = (int)(brush + 0.5f);
}

// The tool, and what new objects of the tool's kind start as.
static void _vasal_editor_panel_tools(vasal_editor_state_s* const state,
		_vasal_editor_panel_cursor_s* const cursor) {
	(void)GuiLabel(_vasal_editor_panel_take(cursor, 18.0f), "Tool");
	int tool = (int)state->tool;
	const Rectangle tools = _vasal_editor_panel_take(cursor,
		(5.0f * 24.0f) + (4.0f * 2.0f));
	(void)GuiToggleGroup((Rectangle){ tools.x, tools.y, tools.width, 24.0f },
		"Paint terrain\nSettlements\nSpawn areas\nBandit camps\nDecorations",
		&tool);
	if (tool != (int)state->tool) {
		state->tool = (vasal_editor_state_tool_e)tool;
		state->selected = false;
	}
	if (vasal_editor_state_tool_paint == state->tool) {
		_vasal_editor_panel_terrain(state, cursor);
	} else if (vasal_editor_state_tool_settlement == state->tool) {
		(void)GuiLabel(_vasal_editor_panel_take(cursor, 18.0f),
			"New settlements");
		int type = (int)state->settlement_type;
		const Rectangle types = _vasal_editor_panel_take(cursor, 24.0f);
		(void)GuiToggleGroup((Rectangle){ types.x, types.y,
			(types.width / 4.0f) - 2.0f, types.height },
			"Village;Town;Castle;City", &type);
		state->settlement_type = (vasal_engine_map_settlement_type_e)type;
	} else if (vasal_editor_state_tool_decoration == state->tool) {
		int kind = (int)state->decoration_kind;
		(void)_vasal_editor_panel_number(cursor, "New decorations' kind",
			&kind, 0, 65535, _vasal_editor_panel_kind);
		state->decoration_kind = (uint16_t)kind;
	} else {
		// Spawn areas and camps have nothing to choose.
	}
}

// What the map check finds.
// spec: editor.validation
static void _vasal_editor_panel_problems_list(
		_vasal_editor_panel_cursor_s* const cursor) {
	vasal_engine_map_finding_s findings[8];
	const size_t count = vasal_engine_map_check(vasal_editor_document_map(),
		findings, sizeof(findings) / sizeof(findings[0]));
	(void)GuiLabel(_vasal_editor_panel_take(cursor, 18.0f), (0U == count)
		? "No problems found." : TextFormat("Problems: %u",
			(unsigned int)count));
	const size_t shown = (count < 8U) ? count : 8U;
	for (size_t i = 0U; i < shown; i++) {
		(void)GuiLabel(_vasal_editor_panel_take(cursor, 18.0f),
			_vasal_editor_panel_problems[findings[i].problem]);
	}
}

// Applies one number field of a settlement, inside a merged operation.
static void _vasal_editor_panel_settlement_field(
		_vasal_editor_panel_cursor_s* const cursor, const char* const label,
		uint32_t* const value, const int minimum, const int maximum,
		const int field, const uint32_t index) {
	int edited = (int)*value;
	// raygui writes the value through its pointer, which cppcheck can't
	// see.
	// cppcheck-suppress knownConditionTrueFalse
	if (_vasal_editor_panel_number(cursor, label, &edited, minimum, maximum,
			field)) {
		vasal_editor_document_begin(_vasal_editor_panel_key(field, index));
		*value = (uint32_t)edited;
		vasal_editor_document_changed();
		vasal_editor_document_end();
	}
}

// Applies one 16-bit number field, inside a merged operation.
static void _vasal_editor_panel_short_field(
		_vasal_editor_panel_cursor_s* const cursor, const char* const label,
		uint16_t* const value, const int minimum, const int maximum,
		const int field, const uint32_t index) {
	int edited = (int)*value;
	// raygui writes the value through its pointer, which cppcheck can't
	// see.
	// cppcheck-suppress knownConditionTrueFalse
	if (_vasal_editor_panel_number(cursor, label, &edited, minimum, maximum,
			field)) {
		vasal_editor_document_begin(_vasal_editor_panel_key(field, index));
		*value = (uint16_t)edited;
		vasal_editor_document_changed();
		vasal_editor_document_end();
	}
}

// The selected settlement's values and flags.
// spec: editor.tools#2 settlements.properties
static void _vasal_editor_panel_settlement(vasal_editor_state_s* const state,
		_vasal_editor_panel_cursor_s* const cursor) {
	vasal_engine_map_s* const map = vasal_editor_document_map();
	vasal_engine_map_settlement_s* const settlement =
		&map->settlements[state->selection];
	const uint32_t index = state->selection;
	char name[vasal_engine_map_name_capacity];
	(void)memcpy(name, settlement->name, sizeof(name));
	if (_vasal_editor_panel_text(cursor, "Name", name,
			_vasal_editor_panel_name)) {
		vasal_editor_document_begin(_vasal_editor_panel_key(
			_vasal_editor_panel_name, index));
		(void)memcpy(settlement->name, name, sizeof(name));
		vasal_editor_document_changed();
		vasal_editor_document_end();
	}
	(void)GuiLabel(_vasal_editor_panel_take(cursor, 18.0f), "Type");
	int type = (int)settlement->type;
	const Rectangle types = _vasal_editor_panel_take(cursor, 24.0f);
	(void)GuiToggleGroup((Rectangle){ types.x, types.y,
		(types.width / 4.0f) - 2.0f, types.height },
		"Village;Town;Castle;City", &type);
	if (type != (int)settlement->type) {
		vasal_editor_document_begin(0U);
		vasal_engine_rules_settlement_defaults(vasal_editor_ruleset_rules(),
			settlement, (vasal_engine_map_settlement_type_e)type);
		vasal_editor_document_changed();
		vasal_editor_document_end();
	}

	_vasal_editor_panel_short_field(cursor, "Capture radius (tiles)",
		&settlement->radius, 1, 64, _vasal_editor_panel_radius, index);
	_vasal_editor_panel_short_field(cursor, "Troops needed to capture",
		&settlement->requirement, 1, 1000, _vasal_editor_panel_requirement,
		index);
	_vasal_editor_panel_settlement_field(cursor, "Capture time (ticks)",
		&settlement->capture_time, 1, 100000, _vasal_editor_panel_capture,
		index);
	_vasal_editor_panel_settlement_field(cursor, "Capture bonus (gold)",
		&settlement->bonus, 0, 1000000, _vasal_editor_panel_bonus, index);
	_vasal_editor_panel_settlement_field(cursor, "Income (gold)",
		&settlement->income, 0, 1000000, _vasal_editor_panel_income, index);

	for (size_t f = 0U; f < 6U; f++) {
		const uint32_t bit = (uint32_t)1U << (uint32_t)f;
		bool set = 0U != (settlement->flags & bit);
		(void)GuiCheckBox(_vasal_editor_panel_take(cursor, 20.0f),
			_vasal_editor_panel_flag_names[f], &set);
		const bool_t was = 0U != (settlement->flags & bit);
		if (set != was) {
			vasal_editor_document_begin(0U);
			settlement->flags ^= bit;
			vasal_editor_document_changed();
		vasal_editor_document_end();
		}
	}
}

// The selected object's values, and deleting it.
static void _vasal_editor_panel_selection(vasal_editor_state_s* const state,
		_vasal_editor_panel_cursor_s* const cursor) {
	vasal_engine_map_s* const map = vasal_editor_document_map();
	const uint32_t index = state->selection;
	if (vasal_editor_state_tool_settlement == state->tool) {
		_vasal_editor_panel_settlement(state, cursor);
	} else if (vasal_editor_state_tool_spawn == state->tool) {
		_vasal_editor_panel_short_field(cursor, "Radius (tiles)",
			&map->spawns[index].radius, 1, 64, _vasal_editor_panel_radius,
			index);
	} else if (vasal_editor_state_tool_decoration == state->tool) {
		_vasal_editor_panel_short_field(cursor, "Kind",
			&map->decorations[index].kind, 0, 65535, _vasal_editor_panel_kind,
			index);
	} else {
		(void)GuiLabel(_vasal_editor_panel_take(cursor, 18.0f),
			"A bandit camp.");
	}
	if (0 != GuiButton(_vasal_editor_panel_take(cursor,
			_vasal_editor_panel_row), "Delete")) {
		vasal_editor_canvas_delete(state);
	}
}

// spec: editor.tools
Rectangle vasal_editor_panel_update(vasal_editor_state_s* const state,
	const float width, const float height) {
	const float inner = vasal_editor_panel_width
		- (2.0f * _vasal_editor_panel_margin);
	(void)GuiPanel((Rectangle){ 0.0f, 0.0f, vasal_editor_panel_width,
		height }, NULL);
	_vasal_editor_panel_cursor_s left = {
		_vasal_editor_panel_margin, _vasal_editor_panel_margin, inner
	};
	Rectangle area = {
		vasal_editor_panel_width, 0.0f, width - vasal_editor_panel_width,
		height
	};
	if (state->selected) {
		area.width -= vasal_editor_panel_width;
	}
	_vasal_editor_panel_file(state, &left, area);
	_vasal_editor_panel_tools(state, &left);
	_vasal_editor_panel_problems_list(&left);

	if (state->selected) {
		const float x = width - vasal_editor_panel_width;
		(void)GuiPanel((Rectangle){ x, 0.0f, vasal_editor_panel_width,
			height }, NULL);
		_vasal_editor_panel_cursor_s right = {
			x + _vasal_editor_panel_margin, _vasal_editor_panel_margin, inner
		};
		(void)GuiLabel(_vasal_editor_panel_take(&right, 22.0f), "Selected");
		_vasal_editor_panel_selection(state, &right);
	}
	return area;
}

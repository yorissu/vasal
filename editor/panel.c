
/// Every control edits a copy of its value; only when the copy differs is
/// the change made, inside a document operation, so the history records
/// real changes only. Repeated changes to the same field share a key and
/// merge into one operation.

#include "editor/panel.h"
#include "editor/art.h"
#include "editor/canvas.h"
#include "editor/document.h"
#include "editor/form.h"
#include "editor/ruleset.h"
#include "editor/theme.h"
#include "engine/sprites.h"

#include <raygui.h>

#include <string.h>

// Fields, for merge keys and for which box is being edited.
enum {
	_vasal_editor_panel_none,
	_vasal_editor_panel_map_name,
	_vasal_editor_panel_terrain_name,
	_vasal_editor_panel_terrain_speed,
	_vasal_editor_panel_terrain_sight,
	_vasal_editor_panel_terrain_color,
	_vasal_editor_panel_name,
	_vasal_editor_panel_radius,
	_vasal_editor_panel_requirement,
	_vasal_editor_panel_capture,
	_vasal_editor_panel_bonus,
	_vasal_editor_panel_income,
	_vasal_editor_panel_x,
	_vasal_editor_panel_y,
	_vasal_editor_panel_kind,
	_vasal_editor_panel_fields
};

// The kinds' names, one and many, in the order of
// vasal_editor_edits_kind_e.
static const char* const _vasal_editor_panel_kind_names[] = {
	"Settlement", "Spawn area", "Bandit camp", "Decoration"
};
static const char* const _vasal_editor_panel_type_names[] = {
	"Village", "Town", "Castle", "City"
};
static const char* const _vasal_editor_panel_structure_names[] = {
	"Watchtower", "Watchtower", "Watchtower", "Watchtower", "Chapel",
	"Chapel", "Chapel", "Windmill", "Bridge", "Ruin"
};
static const char* const _vasal_editor_panel_problems[] = {
	"The map has no settlement.",
	"The map has no spawn area.",
	"A settlement stands on impassable ground.",
	"A spawn area is on impassable ground.",
	"Two capture areas overlap."
};

static float _vasal_editor_panel_left_scroll = 0.0f;
static float _vasal_editor_panel_right_scroll = 0.0f;
static int _vasal_editor_panel_list_kind = 0;

// The id of a box of the panel, apart from other modules'.
static uint32_t _vasal_editor_panel_id(const int field) {
	return 0x10000U + (uint32_t)field;
}

// A merge key for a field of an object.
static uint32_t _vasal_editor_panel_key(const int field,
		const uint32_t index) {
	return ((uint32_t)field << 20U) | (index + 1U);
}

// A row that can be clicked, drawn hovered or selected. Returns whether
// it was clicked.
static bool_t _vasal_editor_panel_pressable(const Rectangle bounds,
		const bool_t selected) {
	const bool_t hovered = !GuiIsLocked()
		&& CheckCollisionPointRec(GetMousePosition(), bounds);
	if (selected) {
		DrawRectangleRounded(bounds, 0.15f, 6,
			vasal_editor_theme_accent_dark);
		DrawRectangleRoundedLinesEx(bounds, 0.15f, 6, 1.0f,
			vasal_editor_theme_accent);
	} else if (hovered) {
		DrawRectangleRounded(bounds, 0.15f, 6, vasal_editor_theme_hover);
	} else {
		// Plain.
	}
	return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// Whether a row is in the form's view, worth drawing.
static bool_t _vasal_editor_panel_shown(const vasal_editor_form_s* const form,
		const Rectangle bounds) {
	return ((bounds.y + bounds.height) >= form->view.y)
		&& (bounds.y <= (form->view.y + form->view.height));
}

// Changes a number of the map inside a merged operation.
static void _vasal_editor_panel_set_short(uint16_t* const value,
		const int edited, const int field, const uint32_t index) {
	vasal_editor_document_begin(_vasal_editor_panel_key(field, index));
	*value = (uint16_t)edited;
	vasal_editor_document_changed();
	vasal_editor_document_end();
}

static void _vasal_editor_panel_set_long(uint32_t* const value,
		const int edited, const int field, const uint32_t index) {
	vasal_editor_document_begin(_vasal_editor_panel_key(field, index));
	*value = (uint32_t)edited;
	vasal_editor_document_changed();
	vasal_editor_document_end();
}

// Changes a name of the map inside a merged operation.
static void _vasal_editor_panel_name_field(vasal_editor_form_s* const form,
		const char* const label, char* const name, const int field,
		const uint32_t index) {
	char edited[vasal_engine_map_name_capacity];
	(void)memcpy(edited, name, sizeof(edited));
	if (vasal_editor_form_text(form, label, edited, sizeof(edited),
			_vasal_editor_panel_id(field))) {
		vasal_editor_document_begin(_vasal_editor_panel_key(field, index));
		(void)memcpy(name, edited, sizeof(edited));
		vasal_editor_document_changed();
		vasal_editor_document_end();
	}
}

// The map's name, size and contents.
// spec: editor.files
static void _vasal_editor_panel_map(vasal_editor_form_s* const form) {
	vasal_engine_map_s* const map = vasal_editor_document_map();
	vasal_editor_form_heading(form, "Map");
	_vasal_editor_panel_name_field(form, "Name", map->name,
		_vasal_editor_panel_map_name, 0U);
	vasal_editor_form_label(form, TextFormat("%u x %u tiles",
		(unsigned int)map->width, (unsigned int)map->height), true);
	vasal_editor_form_label(form, TextFormat("%u settlements, %u spawns, "
		"%u camps", (unsigned int)map->settlement_count,
		(unsigned int)map->spawn_count, (unsigned int)map->camp_count), true);
}

// A terrain type's swatch: its color, name, speed and sight, and its key.
// spec: editor.tools#4 map.terrain#9
static void _vasal_editor_panel_swatch(vasal_editor_state_s* const state,
		vasal_editor_form_s* const form, const uint8_t index) {
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	const vasal_engine_map_terrain_s* const terrain = &map->terrains[index];
	const Rectangle row = vasal_editor_form_take(form, 36.0f);
	if (_vasal_editor_panel_pressable(row, index == state->terrain)) {
		state->terrain = index;
		const bool_t brush = (vasal_editor_state_tool_paint == state->tool)
			|| (vasal_editor_state_tool_fill == state->tool);
		if (!brush) {
			state->tool = vasal_editor_state_tool_paint;
		}
	}
	const Rectangle chip = { row.x + 6.0f, row.y + 6.0f, 24.0f, 24.0f };
	DrawRectangleRounded(chip, 0.25f, 6,
		vasal_editor_theme_color(terrain->color | 0xffU));
	DrawRectangleRoundedLinesEx(chip, 0.25f, 6, 1.0f, (Color){ 0, 0, 0,
		120 });
	vasal_editor_theme_text(terrain->name, row.x + 40.0f, row.y + 3.0f,
		vasal_editor_theme_normal, false, vasal_editor_theme_foreground);
	const char* const values = terrain->passable
		? TextFormat("speed %u%%   sight %u%%", (unsigned int)terrain->speed,
			(unsigned int)terrain->sight)
		: TextFormat("impassable   sight %u%%", (unsigned int)terrain->sight);
	vasal_editor_theme_text(values, row.x + 40.0f, row.y + 20.0f,
		vasal_editor_theme_small, false, vasal_editor_theme_dim);
	if (9U > index) {
		vasal_editor_theme_text(TextFormat("%u", (unsigned int)index + 1U),
			(row.x + row.width) - 18.0f, row.y + 10.0f,
			vasal_editor_theme_small, true, vasal_editor_theme_dim);
	}
}

// The selected terrain type's passability and speed.
static void _vasal_editor_panel_speed(vasal_editor_form_s* const form,
		vasal_engine_map_terrain_s* const terrain, const uint32_t index) {
	bool_t passable = terrain->passable;
	if (vasal_editor_form_check(form, "Passable", NULL, &passable)) {
		vasal_editor_document_begin(0U);
		terrain->passable = passable;
		terrain->speed = 0U;
		if (passable) {
			terrain->speed = 100U;
		}
		vasal_editor_document_changed();
		vasal_editor_document_end();
	}
	if (!terrain->passable) {
		return;
	}
	int speed = (int)terrain->speed;
	if (vasal_editor_form_slider(form, "Speed", "Of troops marching on it; "
			"100% is full speed.", &speed, 1,
			(int)vasal_engine_map_speed_max, "%")) {
		_vasal_editor_panel_set_short(&terrain->speed, speed,
			_vasal_editor_panel_terrain_speed, index);
	}
}

// The selected terrain type's values, how much of the map it covers, and
// adding and removing types.
// spec: editor.tools#4 map.terrain#9
static void _vasal_editor_panel_terrain_values(
		vasal_editor_state_s* const state, vasal_editor_form_s* const form) {
	vasal_engine_map_s* const map = vasal_editor_document_map();
	vasal_engine_map_terrain_s* const terrain = &map->terrains[state->terrain];
	const uint32_t index = state->terrain;
	vasal_editor_form_space(form, 4.0f);
	_vasal_editor_panel_name_field(form, "Name", terrain->name,
		_vasal_editor_panel_terrain_name, index);
	_vasal_editor_panel_speed(form, terrain, index);
	int sight = (int)terrain->sight;
	if (vasal_editor_form_slider(form, "Sight", "How far troops standing on "
			"it see; 100% is their full sight.", &sight, 0,
			(int)vasal_engine_map_sight_max, "%")) {
		_vasal_editor_panel_set_short(&terrain->sight, sight,
			_vasal_editor_panel_terrain_sight, index);
	}
	Color color = vasal_editor_theme_color(terrain->color | 0xffU);
	const Rectangle picker = vasal_editor_form_take(form, 96.0f);
	(void)GuiColorPicker((Rectangle){ picker.x, picker.y,
		picker.width - 30.0f, picker.height }, NULL, &color);
	const uint32_t rgba = vasal_editor_theme_rgba(color) | 0xffU;
	if (rgba != terrain->color) {
		vasal_editor_document_begin(_vasal_editor_panel_key(
			_vasal_editor_panel_terrain_color, index));
		terrain->color = rgba;
		vasal_editor_document_changed();
		vasal_editor_document_end();
	}
	size_t counts[vasal_engine_map_terrains_max];
	vasal_editor_edits_census(counts);
	const float tiles = (float)map->width * (float)map->height;
	vasal_editor_form_label(form, TextFormat("Covers %.1f%% of the map",
		(double)((100.0f * (float)counts[state->terrain]) / tiles)), true);
	const Rectangle row = vasal_editor_form_take(form, vasal_editor_form_row);
	if (0 != GuiButton(vasal_editor_form_part(row, 2U, 0U),
			GuiIconText(ICON_FILE_ADD, "Add type"))) {
		uint8_t added = 0U;
		vasal_editor_document_begin(0U);
		if (vasal_editor_edits_add_terrain(&added)) {
			state->terrain = added;
		}
		vasal_editor_document_end();
	}
	if (0 != GuiButton(vasal_editor_form_part(row, 2U, 1U),
			GuiIconText(ICON_BIN, "Remove"))) {
		vasal_editor_document_begin(0U);
		const bool_t removed = vasal_editor_edits_remove_terrain(
			state->terrain);
		vasal_editor_document_end();
		if (removed) {
			state->terrain = 0U;
		} else {
			vasal_editor_state_say(state, "The first terrain type stays: "
				"removed types' tiles become it.", true);
		}
	}
}

// The brush: its radius and shape.
// spec: editor.tools#10
static void _vasal_editor_panel_brush(vasal_editor_state_s* const state,
		vasal_editor_form_s* const form) {
	vasal_editor_form_heading(form, "Brush");
	int radius = (int)state->brush.radius;
	if (vasal_editor_form_slider(form, "Radius", "[ and ] or Shift and the "
			"wheel change it; Shift-click paints a line.", &radius, 0,
			vasal_editor_edits_brush_max, "")) {
		state->brush.radius = (int32_t)radius;
	}
	int shape = state->brush.square ? 1 : 0;
	const Rectangle row = vasal_editor_form_take(form, vasal_editor_form_row);
	(void)GuiToggleGroup((Rectangle){ row.x, row.y,
		(row.width / 2.0f) - 2.0f, row.height }, "Round;Square", &shape);
	state->brush.square = 1 == shape;
}

// A picture to choose, with its name under it. Returns whether it was
// clicked.
static bool_t _vasal_editor_panel_picture(const Rectangle bounds,
		const size_t sprite, const char* const name, const bool_t selected) {
	const bool_t clicked = _vasal_editor_panel_pressable(bounds, selected);
	vasal_editor_art_picture(sprite, (Rectangle){ bounds.x + 4.0f,
		bounds.y + 4.0f, bounds.width - 8.0f, bounds.height - 24.0f });
	const float width = vasal_editor_theme_measure(name,
		vasal_editor_theme_small, false);
	vasal_editor_theme_text(name, bounds.x + ((bounds.width - width) / 2.0f),
		(bounds.y + bounds.height) - 18.0f, vasal_editor_theme_small, false,
		vasal_editor_theme_foreground);
	return clicked;
}

// The settlement types to place, as pictures.
// spec: editor.tools#6
static void _vasal_editor_panel_types(vasal_editor_state_s* const state,
		vasal_editor_form_s* const form) {
	const Rectangle row = vasal_editor_form_take(form, 84.0f);
	for (size_t t = 0U; t < (size_t)vasal_engine_map_settlement_types; t++) {
		const vasal_engine_map_settlement_type_e type =
			(vasal_engine_map_settlement_type_e)t;
		if (_vasal_editor_panel_picture(vasal_editor_form_part(row, 4U, t),
				vasal_engine_sprites_type(type),
				_vasal_editor_panel_type_names[t],
				type == state->settlement_type)) {
			state->settlement_type = type;
		}
	}
}

// The lone structures to place as decorations, as pictures.
// spec: editor.tools#6
static void _vasal_editor_panel_structures(uint16_t* const kind,
		vasal_editor_form_s* const form) {
	for (size_t r = 0U; r < 2U; r++) {
		const Rectangle row = vasal_editor_form_take(form, 76.0f);
		for (size_t c = 0U; c < 5U; c++) {
			const uint16_t structure = (uint16_t)((r * 5U) + c);
			const vasal_engine_map_decoration_s decoration = {
				{ 0U, 0U }, structure
			};
			if (_vasal_editor_panel_picture(vasal_editor_form_part(row, 5U,
					c), vasal_engine_sprites_decoration(&decoration),
					_vasal_editor_panel_structure_names[structure],
					structure == *kind)) {
				*kind = structure;
			}
		}
	}
}

// What the placing tools place.
static void _vasal_editor_panel_place(vasal_editor_state_s* const state,
		vasal_editor_form_s* const form) {
	if (vasal_editor_state_tool_settlement == state->tool) {
		vasal_editor_form_heading(form, "New settlements");
		_vasal_editor_panel_types(state, form);
		vasal_editor_form_paragraph(form, "A new settlement takes its "
			"type's values from the rules. Click to place, drag to move.");
	} else if (vasal_editor_state_tool_decoration == state->tool) {
		vasal_editor_form_heading(form, "New decorations");
		_vasal_editor_panel_structures(&state->decoration_kind, form);
	} else if (vasal_editor_state_tool_spawn == state->tool) {
		vasal_editor_form_heading(form, "Spawn areas");
		vasal_editor_form_paragraph(form, "New vasals start in a spawn area. "
			"Click to place one, drag to move it.");
	} else if (vasal_editor_state_tool_camp == state->tool) {
		vasal_editor_form_heading(form, "Bandit camps");
		vasal_editor_form_paragraph(form, "Each camp holds the bandits the "
			"rules give a camp (Rules, Bandit camp).");
	} else {
		// The other tools place nothing.
	}
}

// spec: editor.tools
void vasal_editor_panel_left(vasal_editor_state_s* const state,
	const Rectangle area) {
	DrawRectangleRec(area, vasal_editor_theme_panel);
	vasal_editor_form_s form;
	vasal_editor_form_begin(&form, area, &_vasal_editor_panel_left_scroll,
		12.0f);
	_vasal_editor_panel_map(&form);
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	if (state->terrain >= map->terrain_count) {
		state->terrain = 0U;
	}
	vasal_editor_form_heading(&form, "Terrain");
	for (uint8_t i = 0U; i < map->terrain_count; i++) {
		_vasal_editor_panel_swatch(state, &form, i);
	}
	_vasal_editor_panel_terrain_values(state, &form);
	_vasal_editor_panel_brush(state, &form);
	_vasal_editor_panel_place(state, &form);
	vasal_editor_form_end(&form);
	DrawRectangleRec((Rectangle){ (area.x + area.width) - 1.0f, area.y, 1.0f,
		area.height }, vasal_editor_theme_border);
}

// Whether a settlement offers what bit stands for.
static void _vasal_editor_panel_offer(vasal_editor_form_s* const form,
		vasal_engine_map_settlement_s* const settlement, const uint32_t bit,
		const char* const name) {
	bool_t offered = 0U != (settlement->flags & bit);
	if (vasal_editor_form_check(form, name, NULL, &offered)) {
		vasal_editor_document_begin(0U);
		settlement->flags ^= bit;
		vasal_editor_document_changed();
		vasal_editor_document_end();
	}
}

// A settlement's number field.
static void _vasal_editor_panel_settlement_number(
		vasal_editor_form_s* const form, const char* const label,
		uint32_t* const value, const vasal_engine_rules_settlement_e field,
		const int key, const uint32_t index) {
	const vasal_engine_rules_field_s* const description =
		vasal_engine_rules_settlement_field(field);
	int edited = (int)*value;
	if (vasal_editor_form_number(form, label, description->help, &edited,
			(int)description->minimum, (int)description->maximum,
			_vasal_editor_panel_id(key))) {
		_vasal_editor_panel_set_long(value, edited, key, index);
	}
}

// The selected settlement's type, values and offers.
// spec: editor.tools#2 settlements.properties
static void _vasal_editor_panel_settlement(vasal_editor_state_s* const state,
		vasal_editor_form_s* const form) {
	vasal_engine_map_s* const map = vasal_editor_document_map();
	vasal_engine_map_settlement_s* const settlement =
		&map->settlements[state->selection];
	const uint32_t index = state->selection;
	const vasal_engine_rules_s* const rules = vasal_editor_ruleset_rules();
	_vasal_editor_panel_name_field(form, "Name", settlement->name,
		_vasal_editor_panel_name, index);
	const Rectangle row = vasal_editor_form_take(form, 84.0f);
	for (size_t t = 0U; t < (size_t)vasal_engine_map_settlement_types; t++) {
		const vasal_engine_map_settlement_type_e type =
			(vasal_engine_map_settlement_type_e)t;
		if (_vasal_editor_panel_picture(vasal_editor_form_part(row, 4U, t),
				vasal_engine_sprites_type(type),
				_vasal_editor_panel_type_names[t],
				(uint32_t)type == (uint32_t)settlement->type)
				&& ((uint32_t)type != (uint32_t)settlement->type)) {
			vasal_editor_document_begin(0U);
			vasal_engine_rules_settlement_defaults(rules, settlement, type);
			vasal_editor_document_changed();
			vasal_editor_document_end();
		}
	}
	int radius = (int)settlement->radius;
	if (vasal_editor_form_number(form, "Capture radius", "tiles", &radius,
			1, 64, _vasal_editor_panel_id(_vasal_editor_panel_radius))) {
		_vasal_editor_panel_set_short(&settlement->radius, radius,
			_vasal_editor_panel_radius, index);
	}
	int requirement = (int)settlement->requirement;
	if (vasal_editor_form_number(form, "Troops to capture",
			"troops needed in the area", &requirement, 1, 1000,
			_vasal_editor_panel_id(_vasal_editor_panel_requirement))) {
		_vasal_editor_panel_set_short(&settlement->requirement, requirement,
			_vasal_editor_panel_requirement, index);
	}
	_vasal_editor_panel_settlement_number(form, "Capture time",
		&settlement->capture_time, vasal_engine_rules_settlement_capture,
		_vasal_editor_panel_capture, index);
	_vasal_editor_panel_settlement_number(form, "Capture bonus",
		&settlement->bonus, vasal_engine_rules_settlement_bonus,
		_vasal_editor_panel_bonus, index);
	_vasal_editor_panel_settlement_number(form, "Income", &settlement->income,
		vasal_engine_rules_settlement_income, _vasal_editor_panel_income,
		index);
	vasal_editor_form_label(form, "Offers", false);
	_vasal_editor_panel_offer(form, settlement,
		vasal_engine_map_flag_centenars, "Centenars");
	for (uint32_t u = 0U; u < rules->unit_count; u++) {
		_vasal_editor_panel_offer(form, settlement, (uint32_t)1U << (u + 1U),
			rules->units[u].name);
	}
	if (vasal_editor_form_button(form, "Reset to the type's values")) {
		vasal_editor_document_begin(0U);
		vasal_engine_rules_settlement_defaults(rules, settlement,
			settlement->type);
		vasal_editor_document_changed();
		vasal_editor_document_end();
	}
}

// The selected object's position.
static void _vasal_editor_panel_position(vasal_editor_state_s* const state,
		vasal_editor_form_s* const form) {
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	vasal_engine_map_point_s* const at = vasal_editor_edits_position(
		state->selection_kind, state->selection);
	const uint32_t index = ((uint32_t)state->selection_kind << 16U)
		| state->selection;
	int x = (int)at->x;
	int y = (int)at->y;
	if (vasal_editor_form_number(form, "Column", "tiles from the left", &x,
			0, (int)map->width - 1,
			_vasal_editor_panel_id(_vasal_editor_panel_x))) {
		_vasal_editor_panel_set_short(&at->x, x, _vasal_editor_panel_x,
			index);
	}
	if (vasal_editor_form_number(form, "Row", "tiles from the top", &y, 0,
			(int)map->height - 1,
			_vasal_editor_panel_id(_vasal_editor_panel_y))) {
		_vasal_editor_panel_set_short(&at->y, y, _vasal_editor_panel_y,
			index);
	}
}

// The values of the selected object of the other kinds.
static void _vasal_editor_panel_other(const vasal_editor_state_s* const state,
		vasal_editor_form_s* const form) {
	vasal_engine_map_s* const map = vasal_editor_document_map();
	const uint32_t index = state->selection;
	if (vasal_editor_edits_spawn == state->selection_kind) {
		int radius = (int)map->spawns[index].radius;
		if (vasal_editor_form_number(form, "Radius", "tiles", &radius, 1, 64,
				_vasal_editor_panel_id(_vasal_editor_panel_radius))) {
			_vasal_editor_panel_set_short(&map->spawns[index].radius, radius,
				_vasal_editor_panel_radius, index);
		}
	} else if (vasal_editor_edits_decoration == state->selection_kind) {
		uint16_t kind = map->decorations[index].kind;
		_vasal_editor_panel_structures(&kind, form);
		if (kind != map->decorations[index].kind) {
			_vasal_editor_panel_set_short(&map->decorations[index].kind,
				(int)kind, _vasal_editor_panel_kind, index);
		}
	} else {
		vasal_editor_form_paragraph(form, "Its bandits are set in the rules "
			"(Rules, Bandit camp), the same for every camp.");
	}
}

// The selected object's values, copying and deleting it.
// spec: editor.tools#1 editor.tools#2
static void _vasal_editor_panel_inspector(vasal_editor_state_s* const state,
		vasal_editor_form_s* const form) {
	if (state->selected && (state->selection
			>= vasal_editor_edits_count(state->selection_kind))) {
		state->selected = false;
	}
	if (!state->selected) {
		vasal_editor_form_heading(form, "Nothing selected");
		vasal_editor_form_paragraph(form, "Click an object with the select "
			"tool (V) or its placing tool to edit it here. Press F1 for "
			"every key.");
		return;
	}
	vasal_editor_form_heading(form, TextFormat("%s %u",
		_vasal_editor_panel_kind_names[state->selection_kind],
		(unsigned int)state->selection + 1U));
	if (vasal_editor_edits_settlement == state->selection_kind) {
		_vasal_editor_panel_settlement(state, form);
	} else {
		_vasal_editor_panel_other(state, form);
	}
	_vasal_editor_panel_position(state, form);
	const Rectangle row = vasal_editor_form_take(form, vasal_editor_form_row);
	if (0 != GuiButton(vasal_editor_form_part(row, 3U, 0U),
			GuiIconText(ICON_TARGET, "Show"))) {
		const vasal_engine_map_point_s* const at = vasal_editor_edits_position(
			state->selection_kind, state->selection);
		vasal_editor_canvas_focus(state, at->x, at->y);
	}
	if (0 != GuiButton(vasal_editor_form_part(row, 3U, 1U),
			GuiIconText(ICON_FILE_COPY, "Copy"))) {
		vasal_editor_canvas_duplicate(state);
	}
	if (0 != GuiButton(vasal_editor_form_part(row, 3U, 2U),
			GuiIconText(ICON_BIN, "Delete"))) {
		vasal_editor_canvas_delete(state);
	}
}

// One object in the list: its name or number, and where it is.
static void _vasal_editor_panel_entry(vasal_editor_state_s* const state,
		vasal_editor_form_s* const form, const vasal_editor_edits_kind_e kind,
		const uint16_t index) {
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	const Rectangle row = vasal_editor_form_take(form, 24.0f);
	if (!_vasal_editor_panel_shown(form, row)) {
		return;
	}
	const bool_t selected = state->selected && (state->selection_kind == kind)
		&& (state->selection == index);
	const vasal_engine_map_point_s* const at =
		vasal_editor_edits_position(kind, index);
	if (_vasal_editor_panel_pressable(row, selected)) {
		vasal_editor_state_select(state, true, kind, index);
		vasal_editor_canvas_focus(state, at->x, at->y);
	}
	const char* name = TextFormat("%s %u", _vasal_editor_panel_kind_names[kind],
		(unsigned int)index + 1U);
	if (vasal_editor_edits_settlement == kind) {
		const vasal_engine_map_settlement_s* const settlement =
			&map->settlements[index];
		name = TextFormat("%s (%s)", ('\0' == settlement->name[0])
			? "unnamed" : settlement->name,
			_vasal_editor_panel_type_names[settlement->type]);
	}
	vasal_editor_theme_text(name, row.x + 6.0f, row.y + 4.0f,
		vasal_editor_theme_normal, false, vasal_editor_theme_foreground);
	const char* const where = TextFormat("%u, %u", (unsigned int)at->x,
		(unsigned int)at->y);
	vasal_editor_theme_text(where, (row.x + row.width) - 6.0f
		- vasal_editor_theme_measure(where, vasal_editor_theme_small, false),
		row.y + 6.0f, vasal_editor_theme_small, false, vasal_editor_theme_dim);
}

// Every object of one kind, chosen above the list.
static void _vasal_editor_panel_objects(vasal_editor_state_s* const state,
		vasal_editor_form_s* const form) {
	vasal_editor_form_heading(form, "Objects");
	const Rectangle row = vasal_editor_form_take(form, 24.0f);
	(void)GuiToggleGroup((Rectangle){ row.x, row.y,
		(row.width / 4.0f) - 2.0f, row.height },
		TextFormat("#%d#%u;#%d#%u;#%d#%u;#%d#%u", ICON_HOUSE,
			(unsigned int)vasal_editor_edits_count(
				vasal_editor_edits_settlement), ICON_PLAYER,
			(unsigned int)vasal_editor_edits_count(vasal_editor_edits_spawn),
			ICON_DEMON,
			(unsigned int)vasal_editor_edits_count(vasal_editor_edits_camp),
			ICON_FILETYPE_IMAGE,
			(unsigned int)vasal_editor_edits_count(
				vasal_editor_edits_decoration)),
		&_vasal_editor_panel_list_kind);
	const vasal_editor_edits_kind_e kind =
		(vasal_editor_edits_kind_e)_vasal_editor_panel_list_kind;
	vasal_editor_form_label(form, TextFormat("%ss",
		_vasal_editor_panel_kind_names[kind]), true);
	const uint16_t count = vasal_editor_edits_count(kind);
	for (uint16_t i = 0U; i < count; i++) {
		_vasal_editor_panel_entry(state, form, kind, i);
	}
	if (0U == count) {
		vasal_editor_form_paragraph(form, "None yet.");
	}
}

// What the map check finds; clicking one shows what it is about.
// spec: editor.validation editor.validation#2
static void _vasal_editor_panel_problems_list(
		vasal_editor_state_s* const state, vasal_editor_form_s* const form) {
	vasal_engine_map_finding_s findings[16];
	const size_t count = vasal_engine_map_check(vasal_editor_document_map(),
		findings, sizeof(findings) / sizeof(findings[0]));
	vasal_editor_form_heading(form, (0U == count) ? "No problems"
		: TextFormat("Problems (%u)", (unsigned int)count));
	if (0U == count) {
		vasal_editor_form_paragraph(form, "The server will take this map.");
	}
	const size_t shown = (count < 16U) ? count : 16U;
	for (size_t i = 0U; i < shown; i++) {
		const vasal_engine_map_problem_e problem = findings[i].problem;
		const char* const text = _vasal_editor_panel_problems[problem];
		const float height = vasal_editor_theme_wrapped_height(text,
			form->width - 30.0f, vasal_editor_theme_small) + 8.0f;
		const Rectangle row = vasal_editor_form_take(form, height);
		const bool_t about_settlement =
			(vasal_engine_map_problem_settlement_impassable == problem)
			|| (vasal_engine_map_problem_capture_overlap == problem);
		const bool_t about_spawn =
			vasal_engine_map_problem_spawn_impassable == problem;
		if (_vasal_editor_panel_pressable(row, false)
				&& (about_settlement || about_spawn)) {
			const vasal_editor_edits_kind_e kind = about_spawn
				? vasal_editor_edits_spawn : vasal_editor_edits_settlement;
			vasal_editor_state_select(state, true, kind, findings[i].index);
			const vasal_engine_map_point_s* const at =
				vasal_editor_edits_position(kind, findings[i].index);
			vasal_editor_canvas_focus(state, at->x, at->y);
		}
		GuiDrawIcon(ICON_WARNING, (int)row.x + 4, (int)row.y + 2, 1,
			vasal_editor_theme_warning);
		(void)vasal_editor_theme_wrapped(text, row.x + 26.0f, row.y + 4.0f,
			row.width - 30.0f, vasal_editor_theme_small,
			vasal_editor_theme_foreground);
	}
}

// spec: editor.tools
void vasal_editor_panel_right(vasal_editor_state_s* const state,
	const Rectangle area) {
	DrawRectangleRec(area, vasal_editor_theme_panel);
	DrawRectangleRec((Rectangle){ area.x, area.y, 1.0f, area.height },
		vasal_editor_theme_border);
	vasal_editor_form_s form;
	vasal_editor_form_begin(&form, area, &_vasal_editor_panel_right_scroll,
		12.0f);
	_vasal_editor_panel_inspector(state, &form);
	_vasal_editor_panel_problems_list(state, &form);
	_vasal_editor_panel_objects(state, &form);
	vasal_editor_form_end(&form);
}

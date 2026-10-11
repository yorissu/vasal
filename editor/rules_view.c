
/// Every change is one rules operation (editor/ruleset.h): a change that
/// would leave the rules invalid is undone at once, and the page says so.
/// The fields come from the rules' tables, so a new field shows here
/// without this file changing.

#include "editor/rules_view.h"
#include "editor/art.h"
#include "editor/document.h"
#include "editor/form.h"
#include "editor/ruleset.h"
#include "editor/theme.h"
#include "engine/sprites.h"

#include <raygui.h>

#include <math.h>
#include <string.h>

// The widest the page's content gets.
#define _vasal_editor_rules_view_width 720.0f

// The ticks of a second, to show times in seconds.
#define _vasal_editor_rules_view_ticks 20.0f

static float _vasal_editor_rules_view_nav_scroll = 0.0f;
static float _vasal_editor_rules_view_page_scrolls[vasal_editor_state_pages];
static char _vasal_editor_rules_view_items[1024];

static const char* const _vasal_editor_rules_view_type_names[] = {
	"Village", "Town", "Castle", "City"
};

// The id of a box of the page, apart from other modules'.
static uint32_t _vasal_editor_rules_view_id(const uint32_t page,
		const uint32_t field, const uint32_t index) {
	return 0x200000U | (page << 16U) | (field << 8U) | index;
}

// Starts a change to the rules; consecutive changes with the same key
// merge into one operation.
static vasal_engine_rules_s* _vasal_editor_rules_view_begin(
		const uint32_t key) {
	vasal_editor_ruleset_begin(key);
	return vasal_editor_ruleset_rules();
}

// Ends a change, saying so when it was undone for leaving the rules
// invalid.
static void _vasal_editor_rules_view_end(vasal_editor_state_s* const state,
		const char* const why) {
	if (!vasal_editor_ruleset_end()) {
		vasal_editor_state_say(state, why, true);
	}
}

// A clickable row of the left side, drawn hovered or chosen. Returns
// whether it was clicked.
static bool_t _vasal_editor_rules_view_entry(vasal_editor_form_s* const form,
		const char* const text, const bool_t chosen, const int icon) {
	const Rectangle row = vasal_editor_form_take(form, 28.0f);
	const bool_t hovered = !GuiIsLocked()
		&& CheckCollisionPointRec(GetMousePosition(), row);
	if (chosen) {
		DrawRectangleRounded(row, 0.2f, 6, vasal_editor_theme_accent_dark);
	} else if (hovered) {
		DrawRectangleRounded(row, 0.2f, 6, vasal_editor_theme_hover);
	} else {
		// Plain.
	}
	if (0 <= icon) {
		GuiDrawIcon(icon, (int)row.x + 6, (int)row.y + 6, 1,
			vasal_editor_theme_foreground);
	}
	vasal_editor_theme_text(text, row.x + 30.0f, row.y + 6.0f,
		vasal_editor_theme_normal, chosen, vasal_editor_theme_foreground);
	return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// A unit type in the list: its color, name and kind.
static bool_t _vasal_editor_rules_view_unit_entry(
		vasal_editor_form_s* const form,
		const vasal_engine_rules_s* const rules, const uint8_t unit,
		const bool_t chosen) {
	const Rectangle row = vasal_editor_form_take(form, 28.0f);
	const bool_t hovered = !GuiIsLocked()
		&& CheckCollisionPointRec(GetMousePosition(), row);
	if (chosen) {
		DrawRectangleRounded(row, 0.2f, 6, vasal_editor_theme_accent_dark);
	} else if (hovered) {
		DrawRectangleRounded(row, 0.2f, 6, vasal_editor_theme_hover);
	} else {
		// Plain.
	}
	const vasal_engine_rules_unit_s* const type = &rules->units[unit];
	DrawCircleV((Vector2){ row.x + 14.0f, row.y + 14.0f }, 7.0f,
		vasal_editor_theme_color(type->color | 0xffU));
	DrawCircleLinesV((Vector2){ row.x + 14.0f, row.y + 14.0f }, 7.0f,
		(Color){ 0, 0, 0, 160 });
	vasal_editor_theme_text(type->name, row.x + 30.0f, row.y + 6.0f,
		vasal_editor_theme_normal, chosen, vasal_editor_theme_foreground);
	const bool_t ranged =
		0U != type->values[vasal_engine_rules_unit_ranged];
	const char* const kind = ranged ? "ranged" : "melee";
	vasal_editor_theme_text(kind, (row.x + row.width) - 6.0f
		- vasal_editor_theme_measure(kind, vasal_editor_theme_small, false),
		row.y + 8.0f, vasal_editor_theme_small, false, vasal_editor_theme_dim);
	return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// Adding, copying, removing and moving unit types.
// spec: editor.rules#4 editor.rules#6
static void _vasal_editor_rules_view_unit_buttons(
		vasal_editor_state_s* const state, vasal_editor_form_s* const form) {
	const vasal_engine_rules_s* const rules = vasal_editor_ruleset_rules();
	const Rectangle row = vasal_editor_form_take(form, vasal_editor_form_row);
	uint8_t made = vasal_engine_rules_none;
	if (0 != GuiButton(vasal_editor_form_part(row, 3U, 0U),
			"Add")) {
		made = vasal_editor_ruleset_add_unit();
	}
	if (0 != GuiButton(vasal_editor_form_part(row, 3U, 1U),
			"Copy")) {
		made = vasal_editor_ruleset_duplicate_unit(state->unit);
	}
	if (0 != GuiButton(vasal_editor_form_part(row, 3U, 2U),
			"Remove")) {
		const uint8_t removed = state->unit;
		if (vasal_editor_ruleset_remove_unit(removed)) {
			// The map's settlements stop offering it, and later types'
			// offers follow them down.
			vasal_engine_map_s* const map = vasal_editor_document_map();
			vasal_editor_document_begin(0U);
			for (uint16_t i = 0U; i < map->settlement_count; i++) {
				map->settlements[i].flags = vasal_editor_ruleset_flags_after(
					map->settlements[i].flags, removed,
					vasal_engine_rules_none, vasal_engine_rules_none);
			}
			vasal_editor_document_changed();
			vasal_editor_document_end();
			state->unit = (0U < removed) ? (uint8_t)(removed - 1U) : 0U;
		} else {
			vasal_editor_state_say(state, "The rules keep at least one unit "
				"type.", true);
		}
	}
	if (vasal_engine_rules_none != made) {
		state->unit = made;
		state->page = vasal_editor_state_page_unit;
	} else if (rules->unit_count >= vasal_engine_rules_units_max) {
		vasal_editor_form_paragraph(form, "That is the most unit types.");
	} else {
		// Nothing made.
	}
	const Rectangle order = vasal_editor_form_take(form,
		vasal_editor_form_row);
	if ((0 != GuiButton(vasal_editor_form_part(order, 2U, 0U),
			GuiIconText(ICON_ARROW_UP, "Earlier"))) && (0U < state->unit)
			&& vasal_editor_ruleset_swap_units((uint8_t)(state->unit - 1U))) {
		state->unit--;
	}
	if ((0 != GuiButton(vasal_editor_form_part(order, 2U, 1U),
			GuiIconText(ICON_ARROW_DOWN, "Later")))
			&& vasal_editor_ruleset_swap_units(state->unit)) {
		state->unit++;
	}
}

// The sections and the unit types.
static void _vasal_editor_rules_view_nav(vasal_editor_state_s* const state,
		const Rectangle area) {
	DrawRectangleRec(area, vasal_editor_theme_panel);
	vasal_editor_form_s form;
	vasal_editor_form_begin(&form, area, &_vasal_editor_rules_view_nav_scroll,
		12.0f);
	const vasal_engine_rules_s* const rules = vasal_editor_ruleset_rules();
	vasal_editor_form_heading(&form, "Rules");
	static const char* const pages[] = {
		"General", "", "Starting army", "Bandit camp", "Settlement types",
		"Combat"
	};
	static const int icons[] = {
		ICON_FILETYPE_INFO, 0, ICON_PLAYER, ICON_SHIELD, ICON_HOUSE,
		ICON_STAR
	};
	for (size_t p = 0U; p < (size_t)vasal_editor_state_pages; p++) {
		const vasal_editor_state_page_e page = (vasal_editor_state_page_e)p;
		if ((vasal_editor_state_page_unit != page)
				&& _vasal_editor_rules_view_entry(&form, pages[p],
					page == state->page, icons[p])) {
			state->page = page;
		}
	}
	vasal_editor_form_heading(&form, TextFormat("Unit types (%u of %u)",
		(unsigned int)rules->unit_count,
		(unsigned int)vasal_engine_rules_units_max));
	if (state->unit >= rules->unit_count) {
		state->unit = 0U;
	}
	for (uint8_t u = 0U; u < rules->unit_count; u++) {
		const bool_t chosen = (vasal_editor_state_page_unit == state->page)
			&& (u == state->unit);
		if (_vasal_editor_rules_view_unit_entry(&form, rules, u, chosen)) {
			state->unit = u;
			state->page = vasal_editor_state_page_unit;
		}
	}
	vasal_editor_form_space(&form, 4.0f);
	_vasal_editor_rules_view_unit_buttons(state, &form);
	vasal_editor_form_end(&form);
	DrawRectangleRec((Rectangle){ (area.x + area.width) - 1.0f, area.y, 1.0f,
		area.height }, vasal_editor_theme_border);
}

// One of a unit type's fields, as a check box for yes-or-no fields and a
// number box for the others, greyed out when only ranged types use it.
// spec: editor.rules#4
static void _vasal_editor_rules_view_unit_field(
		vasal_editor_state_s* const state, vasal_editor_form_s* const form,
		const vasal_engine_rules_unit_e field) {
	const vasal_engine_rules_field_s* const description =
		vasal_engine_rules_unit_field(field);
	const vasal_engine_rules_s* const rules = vasal_editor_ruleset_rules();
	const uint32_t value = rules->units[state->unit].values[field];
	const uint32_t key = _vasal_editor_rules_view_id(1U, (uint32_t)field,
		state->unit);
	const bool_t ranged_only = (vasal_engine_rules_unit_range == field)
		|| (vasal_engine_rules_unit_reload == field)
		|| (vasal_engine_rules_unit_sidearm == field);
	const bool_t melee = 0U == rules->units[state->unit].values
		[vasal_engine_rules_unit_ranged];
	if (ranged_only && melee) {
		GuiDisable();
	}
	uint32_t after = value;
	if ((0U == description->minimum) && (1U == description->maximum)) {
		bool_t set = 0U != value;
		if (vasal_editor_form_check(form, description->label,
				description->help, &set)) {
			after = 0U;
			if (set) {
				after = 1U;
			}
		}
	} else {
		int edited = (int)value;
		if (vasal_editor_form_number(form, description->label,
				description->help, &edited, (int)description->minimum,
				(int)description->maximum, key)) {
			after = (uint32_t)edited;
		}
	}
	GuiEnable();
	if (after != value) {
		vasal_engine_rules_s* const changed =
			_vasal_editor_rules_view_begin(key);
		changed->units[state->unit].values[field] = after;
		_vasal_editor_rules_view_end(state, "That value is out of range.");
	}
}

// What a unit type's values amount to in play.
static void _vasal_editor_rules_view_summary(vasal_editor_form_s* const form,
		const vasal_engine_rules_unit_s* const unit) {
	const float speed = ((float)unit->values[vasal_engine_rules_unit_speed]
		* _vasal_editor_rules_view_ticks) / 256.0f;
	const float interval = (float)unit->values
		[vasal_engine_rules_unit_interval] / _vasal_editor_rules_view_ticks;
	vasal_editor_form_label(form, TextFormat("Marches %.1f tiles a second "
		"and sees %u tiles.", (double)speed,
		(unsigned int)unit->values[vasal_engine_rules_unit_sight]), true);
	vasal_editor_form_label(form, TextFormat("Strikes every %.2f seconds in "
		"a duel.", (double)interval), true);
	if (0U != unit->values[vasal_engine_rules_unit_ranged]) {
		const float reload = (float)unit->values
			[vasal_engine_rules_unit_reload] / _vasal_editor_rules_view_ticks;
		vasal_editor_form_label(form, TextFormat("Shoots every %.1f seconds "
			"up to %u tiles.", (double)reload,
			(unsigned int)unit->values[vasal_engine_rules_unit_range]), true);
	}
	vasal_editor_form_label(form, TextFormat("Costs %u gold, paid %u.",
		(unsigned int)unit->values[vasal_engine_rules_unit_cost],
		(unsigned int)unit->values[vasal_engine_rules_unit_salary]), true);
}

// Where a unit type is used: the starting army, the camp, the centenar
// and the settlement types offering it.
static void _vasal_editor_rules_view_uses(vasal_editor_form_s* const form,
		const vasal_engine_rules_s* const rules, const uint8_t unit) {
	uint32_t starting = 0U;
	uint32_t camp = 0U;
	for (size_t s = 0U; s < vasal_engine_rules_slots; s++) {
		for (size_t v = 0U; v < vasal_engine_rules_vintaines; v++) {
			if (unit == rules->start[v][s].unit) {
				starting += rules->start[v][s].count;
			}
		}
		if (unit == rules->camp[s].unit) {
			camp += rules->camp[s].count;
		}
	}
	vasal_editor_form_label(form, TextFormat("%u in the starting army%s, %u "
		"in each bandit camp.", (unsigned int)starting,
		(unit == rules->centenar) ? " and its centenar" : "",
		(unsigned int)camp), true);
	char offered[128];
	offered[0] = '\0';
	size_t at = 0U;
	for (size_t t = 0U; t < (size_t)vasal_engine_map_settlement_types; t++) {
		const uint32_t flags =
			rules->settlements[t][vasal_engine_rules_settlement_flags];
		if (0U != (flags & ((uint32_t)1U << ((uint32_t)unit + 1U)))) {
			const char* const name = _vasal_editor_rules_view_type_names[t];
			const size_t length = strlen(name);
			if ((at + length + 3U) < sizeof(offered)) {
				if (0U < at) {
					offered[at] = ',';
					offered[at + 1U] = ' ';
					at += 2U;
				}
				(void)memcpy(&offered[at], name, length);
				at += length;
				offered[at] = '\0';
			}
		}
	}
	vasal_editor_form_label(form, (0U == at) ? "No settlement type offers "
		"it." : TextFormat("Offered by: %s.", offered), true);
}

// A unit type's name, color, fields and what they amount to.
// spec: editor.rules#4
static void _vasal_editor_rules_view_unit(vasal_editor_state_s* const state,
		vasal_editor_form_s* const form) {
	const vasal_engine_rules_s* const rules = vasal_editor_ruleset_rules();
	const vasal_engine_rules_unit_s* const unit = &rules->units[state->unit];
	vasal_editor_form_heading(form, TextFormat("Unit type %u",
		(unsigned int)state->unit + 1U));
	char name[vasal_engine_map_name_capacity];
	(void)memcpy(name, unit->name, sizeof(name));
	if (vasal_editor_form_text(form, "Name", name, sizeof(name),
			_vasal_editor_rules_view_id(1U, 0xffU, state->unit))) {
		vasal_engine_rules_s* const changed = _vasal_editor_rules_view_begin(
			_vasal_editor_rules_view_id(1U, 0xffU, state->unit));
		(void)memcpy(changed->units[state->unit].name, name, sizeof(name));
		_vasal_editor_rules_view_end(state, "A unit type needs a name.");
	}
	// Its look: a troop as the game draws it until it has art.
	const Rectangle look = vasal_editor_form_take(form, 110.0f);
	const Vector2 center = { look.x + 50.0f, look.y + 55.0f };
	DrawCircleV(center, 30.0f, (Color){ 200, 40, 40, 255 });
	DrawCircleV(center, 25.0f, vasal_editor_theme_color(unit->color | 0xffU));
	Color color = vasal_editor_theme_color(unit->color | 0xffU);
	(void)GuiColorPicker((Rectangle){ look.x + 110.0f, look.y,
		look.width - 140.0f, look.height }, NULL, &color);
	const uint32_t rgba = vasal_editor_theme_rgba(color) | 0xffU;
	if (rgba != unit->color) {
		vasal_engine_rules_s* const changed = _vasal_editor_rules_view_begin(
			_vasal_editor_rules_view_id(1U, 0xfeU, state->unit));
		changed->units[state->unit].color = rgba;
		_vasal_editor_rules_view_end(state, "That color can't be used.");
	}
	_vasal_editor_rules_view_summary(form, unit);
	_vasal_editor_rules_view_uses(form, rules, state->unit);
	vasal_editor_form_heading(form, "Values");
	for (size_t f = 0U; f < (size_t)vasal_engine_rules_unit_fields; f++) {
		_vasal_editor_rules_view_unit_field(state, form,
			(vasal_engine_rules_unit_e)f);
	}
}

// The unit types as dropdown items, "empty" or "none" first.
static const char* _vasal_editor_rules_view_unit_items(
		const vasal_engine_rules_s* const rules, const char* const first) {
	size_t at = 0U;
	const size_t room = sizeof(_vasal_editor_rules_view_items) - 1U;
	for (size_t i = 0U; ('\0' != first[i]) && (at < room); i++) {
		_vasal_editor_rules_view_items[at] = first[i];
		at++;
	}
	for (uint8_t u = 0U; u < rules->unit_count; u++) {
		if (at < room) {
			_vasal_editor_rules_view_items[at] = ';';
			at++;
		}
		for (size_t i = 0U; ('\0' != rules->units[u].name[i]) && (at < room)
				&& (i < vasal_engine_map_name_capacity); i++) {
			// ';' separates items.
			const char character = rules->units[u].name[i];
			_vasal_editor_rules_view_items[at] = character;
			if (';' == character) {
				_vasal_editor_rules_view_items[at] = ',';
			}
			at++;
		}
	}
	_vasal_editor_rules_view_items[at] = '\0';
	return _vasal_editor_rules_view_items;
}

// One vintaine's slots: a unit type and a count each, and its total.
// spec: editor.rules#4
static void _vasal_editor_rules_view_slots(vasal_editor_state_s* const state,
		vasal_editor_form_s* const form, const uint32_t page,
		const uint32_t vintaine, const char* const title) {
	const vasal_engine_rules_s* const rules = vasal_editor_ruleset_rules();
	const vasal_engine_rules_slot_s* const slots = (vasal_engine_rules_vintaines
		> vintaine) ? rules->start[vintaine] : rules->camp;
	uint32_t total = 0U;
	for (size_t s = 0U; s < vasal_engine_rules_slots; s++) {
		total += slots[s].count;
	}
	const Rectangle head = vasal_editor_form_take(form, 22.0f);
	vasal_editor_theme_text(title, head.x, head.y + 2.0f,
		vasal_editor_theme_normal, true, vasal_editor_theme_foreground);
	const char* const counted = TextFormat("%u of %u troops",
		(unsigned int)total, (unsigned int)vasal_engine_rules_vintaine_max);
	vasal_editor_theme_text(counted, (head.x + head.width)
		- vasal_editor_theme_measure(counted, vasal_editor_theme_small, false),
		head.y + 4.0f, vasal_editor_theme_small, false, vasal_editor_theme_dim);
	const Rectangle bar = vasal_editor_form_take(form, 4.0f);
	DrawRectangleRec(bar, vasal_editor_theme_raised);
	DrawRectangleRec((Rectangle){ bar.x, bar.y, (bar.width * (float)total)
		/ (float)vasal_engine_rules_vintaine_max, bar.height },
		vasal_editor_theme_accent);
	const char* const items = _vasal_editor_rules_view_unit_items(rules,
		"empty");
	for (uint32_t s = 0U; s < vasal_engine_rules_slots; s++) {
		const Rectangle row = vasal_editor_form_take(form,
			vasal_editor_form_row);
		int unit = (slots[s].unit < rules->unit_count)
			? ((int)slots[s].unit + 1) : 0;
		int count = (int)slots[s].count;
		const Rectangle choice = {
			row.x, row.y, row.width * 0.62f, row.height
		};
		const Rectangle number = {
			row.x + (row.width * 0.65f), row.y, row.width * 0.35f, row.height
		};
		const bool_t chosen = vasal_editor_form_dropdown(choice, items, &unit,
			_vasal_editor_rules_view_id(page, vintaine, s));
		const bool_t counted_again = vasal_editor_form_spinner(number, &count,
			0, (int)vasal_engine_rules_vintaine_max,
			_vasal_editor_rules_view_id(page, vintaine, 0x10U + s));
		if (chosen || counted_again) {
			vasal_engine_rules_s* const changed =
				_vasal_editor_rules_view_begin(
					_vasal_editor_rules_view_id(page, vintaine, 0x10U + s));
			vasal_engine_rules_slot_s* const slot =
				(vasal_engine_rules_vintaines > vintaine)
				? &changed->start[vintaine][s] : &changed->camp[s];
			const int chosen_unit = unit - 1;
			slot->unit = (0 == unit) ? vasal_engine_rules_none
				: (uint8_t)chosen_unit;
			slot->count = (0 == unit) ? 0U : (uint8_t)count;
			if (chosen && (0 != unit) && (0U == slot->count)) {
				slot->count = 1U;
			}
			_vasal_editor_rules_view_end(state, "A vintaine holds at most "
				"20 troops.");
		}
	}
	vasal_editor_form_space(form, 10.0f);
}

// The starting army: the centenar and the vintaines.
// spec: editor.rules#4
static void _vasal_editor_rules_view_army(vasal_editor_state_s* const state,
		vasal_editor_form_s* const form) {
	const vasal_engine_rules_s* const rules = vasal_editor_ruleset_rules();
	vasal_editor_form_heading(form, "Starting army");
	vasal_editor_form_paragraph(form, "What every vasal starts with: five "
		"vintaines of up to 20 troops, each of up to four unit types, and "
		"a centenar leading them.");
	int centenar = (rules->centenar < rules->unit_count)
		? ((int)rules->centenar + 1) : 0;
	if (vasal_editor_form_choice(form, "Centenar",
			_vasal_editor_rules_view_unit_items(rules, "none"), &centenar,
			_vasal_editor_rules_view_id(2U, 0xffU, 0U))) {
		vasal_engine_rules_s* const changed =
			_vasal_editor_rules_view_begin(0U);
		const int chosen = centenar - 1;
		changed->centenar = (0 == centenar) ? vasal_engine_rules_none
			: (uint8_t)chosen;
		_vasal_editor_rules_view_end(state, "That unit type does not exist.");
	}
	vasal_editor_form_space(form, 8.0f);
	for (uint32_t v = 0U; v < vasal_engine_rules_vintaines; v++) {
		_vasal_editor_rules_view_slots(state, form, 2U, v,
			TextFormat("Vintaine %u", (unsigned int)v + 1U));
	}
}

// The bandits of a camp.
// spec: editor.rules#4
static void _vasal_editor_rules_view_camp(vasal_editor_state_s* const state,
		vasal_editor_form_s* const form) {
	vasal_editor_form_heading(form, "Bandit camp");
	vasal_editor_form_paragraph(form, "Every bandit camp of a map holds one "
		"vintaine of these bandits, who guard it.");
	_vasal_editor_rules_view_slots(state, form, 3U,
		vasal_engine_rules_vintaines, "Bandits");
}

// A settlement type's offer of what a bit stands for.
static void _vasal_editor_rules_view_offer(vasal_editor_state_s* const state,
		vasal_editor_form_s* const form, const size_t type, const uint32_t bit,
		const char* const name) {
	const vasal_engine_rules_s* const rules = vasal_editor_ruleset_rules();
	const uint32_t flags =
		rules->settlements[type][vasal_engine_rules_settlement_flags];
	bool_t offered = 0U != (flags & bit);
	if (vasal_editor_form_check(form, name, NULL, &offered)) {
		vasal_engine_rules_s* const changed =
			_vasal_editor_rules_view_begin(0U);
		changed->settlements[type][vasal_engine_rules_settlement_flags] ^= bit;
		_vasal_editor_rules_view_end(state, "That unit type does not exist.");
	}
}

// The settlement types' defaults and offers.
// spec: editor.rules#4
static void _vasal_editor_rules_view_settlements(
		vasal_editor_state_s* const state, vasal_editor_form_s* const form) {
	const vasal_engine_rules_s* const rules = vasal_editor_ruleset_rules();
	vasal_editor_form_heading(form, "Settlement types");
	vasal_editor_form_paragraph(form, "What a new settlement of each type "
		"starts with when it is placed on a map. Each settlement can be "
		"changed on the map afterwards.");
	for (size_t t = 0U; t < (size_t)vasal_engine_map_settlement_types; t++) {
		const Rectangle head = vasal_editor_form_take(form, 64.0f);
		vasal_editor_art_picture(vasal_engine_sprites_type(
			(vasal_engine_map_settlement_type_e)t), (Rectangle){ head.x,
			head.y, 80.0f, head.height });
		vasal_editor_theme_text(_vasal_editor_rules_view_type_names[t],
			head.x + 92.0f, head.y + 20.0f, vasal_editor_theme_large, true,
			vasal_editor_theme_foreground);
		for (size_t f = 0U; f < (size_t)vasal_engine_rules_settlement_flags;
				f++) {
			const vasal_engine_rules_field_s* const description =
				vasal_engine_rules_settlement_field(
					(vasal_engine_rules_settlement_e)f);
			int value = (int)rules->settlements[t][f];
			const uint32_t key = _vasal_editor_rules_view_id(4U, (uint32_t)t,
				(uint32_t)f);
			if (vasal_editor_form_number(form, description->label,
					description->help, &value, (int)description->minimum,
					(int)description->maximum, key)) {
				vasal_engine_rules_s* const changed =
					_vasal_editor_rules_view_begin(key);
				changed->settlements[t][f] = (uint32_t)value;
				_vasal_editor_rules_view_end(state,
					"That value is out of range.");
			}
		}
		vasal_editor_form_label(form, "Offers", false);
		_vasal_editor_rules_view_offer(state, form, t,
			vasal_engine_map_flag_centenars, "Centenars");
		for (uint32_t u = 0U; u < rules->unit_count; u++) {
			_vasal_editor_rules_view_offer(state, form, t,
				(uint32_t)1U << (u + 1U), rules->units[u].name);
		}
		vasal_editor_form_space(form, 12.0f);
	}
}

// The combat values.
// spec: editor.rules#4
static void _vasal_editor_rules_view_combat(vasal_editor_state_s* const state,
		vasal_editor_form_s* const form) {
	const vasal_engine_rules_s* const rules = vasal_editor_ruleset_rules();
	vasal_editor_form_heading(form, "Combat");
	vasal_editor_form_paragraph(form, "Values every fight uses, whatever the "
		"unit types. Times are in ticks, 20 to a second.");
	for (size_t f = 0U; f < (size_t)vasal_engine_rules_combat_fields; f++) {
		const vasal_engine_rules_field_s* const description =
			vasal_engine_rules_combat_field((vasal_engine_rules_combat_e)f);
		int value = (int)rules->combat[f];
		const uint32_t key = _vasal_editor_rules_view_id(5U, (uint32_t)f, 0U);
		if (vasal_editor_form_number(form, description->label,
				description->help, &value, (int)description->minimum,
				(int)description->maximum, key)) {
			vasal_engine_rules_s* const changed =
				_vasal_editor_rules_view_begin(key);
			changed->combat[f] = (uint32_t)value;
			_vasal_editor_rules_view_end(state, "That value is out of range.");
		}
	}
}

// The rules' name, and what to do with them.
// spec: editor.rules#2
static void _vasal_editor_rules_view_general(vasal_editor_state_s* const state,
		vasal_editor_form_s* const form) {
	const vasal_engine_rules_s* const rules = vasal_editor_ruleset_rules();
	vasal_editor_form_heading(form, "General");
	char name[vasal_engine_map_name_capacity];
	(void)memcpy(name, rules->name, sizeof(name));
	if (vasal_editor_form_text(form, "Name", name, sizeof(name),
			_vasal_editor_rules_view_id(0U, 0U, 0U))) {
		vasal_engine_rules_s* const changed = _vasal_editor_rules_view_begin(
			_vasal_editor_rules_view_id(0U, 0U, 0U));
		(void)memcpy(changed->name, name, sizeof(name));
		_vasal_editor_rules_view_end(state, "The rules need a name.");
	}
	uint8_t file[vasal_engine_rules_capacity];
	const size_t size = vasal_engine_rules_write(rules, file, sizeof(file));
	vasal_editor_form_label(form, TextFormat("%u unit types, a %u byte file.",
		(unsigned int)rules->unit_count, (unsigned int)size), true);
	vasal_editor_form_space(form, 6.0f);
	vasal_editor_form_paragraph(form, "The rules hold everything about "
		"troops and combat: the unit types and how they fight, the army a "
		"vasal starts with, the bandits of a camp, what settlements offer, "
		"and the combat values. One rules file serves every map. Save it "
		"with Ctrl+S, open one with Ctrl+O, and upload it with the map on "
		"the Server page: players get it when they join.");
	vasal_editor_form_space(form, 6.0f);
	const Rectangle row = vasal_editor_form_take(form, vasal_editor_form_row);
	if (0 != GuiButton(vasal_editor_form_part(row, 3U, 0U),
			GuiIconText(ICON_FILE_NEW, "Standard"))) {
		state->command = vasal_editor_state_command_new_rules;
	}
	if (0 != GuiButton(vasal_editor_form_part(row, 3U, 1U),
			GuiIconText(ICON_FILE_OPEN, "Open..."))) {
		state->command = vasal_editor_state_command_open;
	}
	if (0 != GuiButton(vasal_editor_form_part(row, 3U, 2U),
			GuiIconText(ICON_FILE_SAVE, "Save"))) {
		state->command = vasal_editor_state_command_save_rules;
	}
}

// spec: editor.rules
void vasal_editor_rules_view_update(vasal_editor_state_s* const state,
	const Rectangle left, const Rectangle page) {
	_vasal_editor_rules_view_nav(state, left);
	DrawRectangleRec(page, vasal_editor_theme_background);
	// The content keeps to a column in the middle of wide pages.
	const float width = fminf(page.width, _vasal_editor_rules_view_width);
	vasal_editor_form_s form;
	vasal_editor_form_begin(&form, page,
		&_vasal_editor_rules_view_page_scrolls[state->page], 24.0f);
	form.x = page.x + ((page.width - width) / 2.0f) + 24.0f;
	form.width = width - 54.0f;
	if (vasal_editor_state_page_general == state->page) {
		_vasal_editor_rules_view_general(state, &form);
	} else if (vasal_editor_state_page_unit == state->page) {
		_vasal_editor_rules_view_unit(state, &form);
	} else if (vasal_editor_state_page_army == state->page) {
		_vasal_editor_rules_view_army(state, &form);
	} else if (vasal_editor_state_page_camp == state->page) {
		_vasal_editor_rules_view_camp(state, &form);
	} else if (vasal_editor_state_page_settlements == state->page) {
		_vasal_editor_rules_view_settlements(state, &form);
	} else {
		_vasal_editor_rules_view_combat(state, &form);
	}
	vasal_editor_form_end(&form);
}


/// The rules are small, so an operation keeps a whole copy of them as they
/// were before; undoing swaps it back in and keeps the copy for redoing.
/// The history is a ring of copies.

#include "editor/ruleset.h"

#include <string.h>

#define _vasal_editor_ruleset_history 32U

static vasal_engine_rules_s _vasal_editor_ruleset_current;
static vasal_engine_rules_s _vasal_editor_ruleset_before;
static vasal_engine_rules_s
	_vasal_editor_ruleset_undo[_vasal_editor_ruleset_history];
static vasal_engine_rules_s
	_vasal_editor_ruleset_redo[_vasal_editor_ruleset_history];
static size_t _vasal_editor_ruleset_undo_count = 0U;
static size_t _vasal_editor_ruleset_redo_count = 0U;
static uint32_t _vasal_editor_ruleset_key = 0U;
static bool_t _vasal_editor_ruleset_ready = false;
static uint32_t _vasal_editor_ruleset_revision = 0U;

static void _vasal_editor_ruleset_forget(void) {
	_vasal_editor_ruleset_revision++;
	_vasal_editor_ruleset_undo_count = 0U;
	_vasal_editor_ruleset_redo_count = 0U;
	_vasal_editor_ruleset_key = 0U;
}

vasal_engine_rules_s* vasal_editor_ruleset_rules(void) {
	if (!_vasal_editor_ruleset_ready) {
		vasal_editor_ruleset_create();
	}
	return &_vasal_editor_ruleset_current;
}

// spec: editor.rules#1
void vasal_editor_ruleset_create(void) {
	vasal_engine_rules_standard(&_vasal_editor_ruleset_current);
	_vasal_editor_ruleset_ready = true;
	_vasal_editor_ruleset_forget();
}

// spec: editor.rules#2
uint32_t vasal_editor_ruleset_open(const uint8_t* const data,
	const size_t size) {
	static vasal_engine_rules_s read;
	uint16_t version = 0U;
	const vasal_engine_rules_read_e outcome =
		vasal_engine_rules_read(data, size, &read, &version);
	if (vasal_engine_rules_read_ok == outcome) {
		_vasal_editor_ruleset_current = read;
		_vasal_editor_ruleset_ready = true;
		_vasal_editor_ruleset_forget();
	}
	return (uint32_t)outcome;
}

void vasal_editor_ruleset_begin(const uint32_t key) {
	(void)vasal_editor_ruleset_rules();
	_vasal_editor_ruleset_before = _vasal_editor_ruleset_current;
	// A merged operation keeps the copy from its first part.
	if ((0U != key) && (key == _vasal_editor_ruleset_key)
			&& (0U < _vasal_editor_ruleset_undo_count)) {
		_vasal_editor_ruleset_before = _vasal_editor_ruleset_undo
			[(_vasal_editor_ruleset_undo_count - 1U)
				% _vasal_editor_ruleset_history];
		_vasal_editor_ruleset_undo_count--;
	}
	_vasal_editor_ruleset_key = key;
}

// spec: editor.rules#3
bool_t vasal_editor_ruleset_end(void) {
	if (!vasal_engine_rules_valid(&_vasal_editor_ruleset_current)) {
		_vasal_editor_ruleset_current = _vasal_editor_ruleset_before;
		_vasal_editor_ruleset_key = 0U;
		return false;
	}
	if (vasal_engine_rules_equal(&_vasal_editor_ruleset_before,
			&_vasal_editor_ruleset_current)) {
		_vasal_editor_ruleset_key = 0U;
		return true;
	}
	_vasal_editor_ruleset_undo[_vasal_editor_ruleset_undo_count
		% _vasal_editor_ruleset_history] = _vasal_editor_ruleset_before;
	_vasal_editor_ruleset_undo_count++;
	if (_vasal_editor_ruleset_undo_count > _vasal_editor_ruleset_history) {
		_vasal_editor_ruleset_undo_count = _vasal_editor_ruleset_history;
	}
	_vasal_editor_ruleset_redo_count = 0U;
	_vasal_editor_ruleset_revision++;
	return true;
}

bool_t vasal_editor_ruleset_undo(void) {
	if (0U == _vasal_editor_ruleset_undo_count) {
		return false;
	}
	_vasal_editor_ruleset_undo_count--;
	_vasal_editor_ruleset_redo[_vasal_editor_ruleset_redo_count
		% _vasal_editor_ruleset_history] = _vasal_editor_ruleset_current;
	_vasal_editor_ruleset_redo_count++;
	_vasal_editor_ruleset_current = _vasal_editor_ruleset_undo
		[_vasal_editor_ruleset_undo_count % _vasal_editor_ruleset_history];
	_vasal_editor_ruleset_key = 0U;
	_vasal_editor_ruleset_revision++;
	return true;
}

bool_t vasal_editor_ruleset_redo(void) {
	if (0U == _vasal_editor_ruleset_redo_count) {
		return false;
	}
	_vasal_editor_ruleset_redo_count--;
	_vasal_editor_ruleset_undo[_vasal_editor_ruleset_undo_count
		% _vasal_editor_ruleset_history] = _vasal_editor_ruleset_current;
	_vasal_editor_ruleset_undo_count++;
	_vasal_editor_ruleset_current = _vasal_editor_ruleset_redo
		[_vasal_editor_ruleset_redo_count % _vasal_editor_ruleset_history];
	_vasal_editor_ruleset_key = 0U;
	_vasal_editor_ruleset_revision++;
	return true;
}

uint32_t vasal_editor_ruleset_revision(void) {
	return _vasal_editor_ruleset_revision;
}

// Copies a name, cut to fit with room for a suffix, and appends the
// suffix.
static void _vasal_editor_ruleset_name(char* const name,
		const char* const base, const char* const suffix) {
	const size_t room = vasal_engine_map_name_capacity - 1U - strlen(suffix);
	size_t at = 0U;
	while ((at < room) && ('\0' != base[at])) {
		name[at] = base[at];
		at++;
	}
	for (size_t i = 0U; '\0' != suffix[i]; i++) {
		name[at] = suffix[i];
		at++;
	}
	while (at < vasal_engine_map_name_capacity) {
		name[at] = '\0';
		at++;
	}
}

// spec: editor.rules#4
uint8_t vasal_editor_ruleset_add_unit(void) {
	vasal_engine_rules_s* const rules = vasal_editor_ruleset_rules();
	if (vasal_engine_rules_units_max <= rules->unit_count) {
		return vasal_engine_rules_none;
	}
	static vasal_engine_rules_s standard;
	vasal_engine_rules_standard(&standard);
	const uint8_t index = rules->unit_count;
	vasal_editor_ruleset_begin(0U);
	vasal_engine_rules_unit_s* const unit = &rules->units[index];
	*unit = standard.units[0];
	const uint32_t count = (uint32_t)index + 1U;
	const char ones = "0123456789"[count % 10U];
	char number[4];
	number[0] = ' ';
	number[1] = ones;
	number[2] = '\0';
	number[3] = '\0';
	if (10U <= count) {
		number[1] = "0123456789"[count / 10U];
		number[2] = ones;
	}
	_vasal_editor_ruleset_name(unit->name, "unit", number);
	// A color of its own, turning around the wheel with each new type.
	static const uint32_t colors[] = {
		0xd05050ffU, 0x50a0d0ffU, 0x60c060ffU, 0xd0a040ffU,
		0xa060d0ffU, 0x40c0b0ffU, 0xd07090ffU, 0x9090a0ffU
	};
	unit->color = colors[index % (sizeof(colors) / sizeof(colors[0]))];
	rules->unit_count++;
	return vasal_editor_ruleset_end() ? index : vasal_engine_rules_none;
}

// spec: editor.rules#4
uint8_t vasal_editor_ruleset_duplicate_unit(const uint8_t unit) {
	vasal_engine_rules_s* const rules = vasal_editor_ruleset_rules();
	if ((unit >= rules->unit_count)
			|| (vasal_engine_rules_units_max <= rules->unit_count)) {
		return vasal_engine_rules_none;
	}
	const uint8_t index = rules->unit_count;
	vasal_editor_ruleset_begin(0U);
	rules->units[index] = rules->units[unit];
	_vasal_editor_ruleset_name(rules->units[index].name,
		rules->units[unit].name, " copy");
	rules->unit_count++;
	return vasal_editor_ruleset_end() ? index : vasal_engine_rules_none;
}

// What a reference to a unit type becomes: removed is gone (none), later
// ones move down; a and b trade places.
static uint8_t _vasal_editor_ruleset_follow(const uint8_t unit,
		const uint8_t removed, const uint8_t a, const uint8_t b) {
	uint8_t followed = unit;
	if (vasal_engine_rules_none == unit) {
		followed = unit;
	} else if (vasal_engine_rules_none != removed) {
		if (unit == removed) {
			followed = vasal_engine_rules_none;
		} else if (unit > removed) {
			followed = (uint8_t)(unit - 1U);
		} else {
			// Before the removed type: unchanged.
		}
	} else if (unit == a) {
		followed = b;
	} else if (unit == b) {
		followed = a;
	} else {
		// Neither of the swapped types.
	}
	return followed;
}

uint32_t vasal_editor_ruleset_flags_after(const uint32_t flags,
	const uint8_t removed, const uint8_t a, const uint8_t b) {
	uint32_t after = flags & vasal_engine_map_flag_centenars;
	for (uint8_t u = 0U; u < vasal_engine_rules_units_max; u++) {
		const uint32_t bit = (uint32_t)1U << ((uint32_t)u + 1U);
		const uint8_t followed = _vasal_editor_ruleset_follow(u, removed, a,
			b);
		if ((0U != (flags & bit)) && (vasal_engine_rules_none != followed)) {
			after |= (uint32_t)1U << ((uint32_t)followed + 1U);
		}
	}
	return after;
}

// Makes every reference of the rules follow a removal or swap.
static void _vasal_editor_ruleset_references(vasal_engine_rules_s* const rules,
		const uint8_t removed, const uint8_t a, const uint8_t b) {
	for (size_t v = 0U; v <= vasal_engine_rules_vintaines; v++) {
		vasal_engine_rules_slot_s* const slots = (v
			< vasal_engine_rules_vintaines) ? rules->start[v] : rules->camp;
		for (size_t s = 0U; s < vasal_engine_rules_slots; s++) {
			slots[s].unit = _vasal_editor_ruleset_follow(slots[s].unit,
				removed, a, b);
			if (vasal_engine_rules_none == slots[s].unit) {
				slots[s].count = 0U;
			}
		}
	}
	rules->centenar = _vasal_editor_ruleset_follow(rules->centenar, removed,
		a, b);
	for (size_t t = 0U; t < (size_t)vasal_engine_map_settlement_types; t++) {
		uint32_t* const flags =
			&rules->settlements[t][vasal_engine_rules_settlement_flags];
		*flags = vasal_editor_ruleset_flags_after(*flags, removed, a, b);
	}
}

// spec: editor.rules#4
bool_t vasal_editor_ruleset_remove_unit(const uint8_t unit) {
	vasal_engine_rules_s* const rules = vasal_editor_ruleset_rules();
	if ((unit >= rules->unit_count) || (1U >= rules->unit_count)) {
		return false;
	}
	vasal_editor_ruleset_begin(0U);
	for (size_t u = unit; (u + 1U) < rules->unit_count; u++) {
		rules->units[u] = rules->units[u + 1U];
	}
	rules->unit_count--;
	(void)memset(&rules->units[rules->unit_count], 0,
		sizeof(rules->units[0]));
	_vasal_editor_ruleset_references(rules, unit, vasal_engine_rules_none,
		vasal_engine_rules_none);
	return vasal_editor_ruleset_end();
}

bool_t vasal_editor_ruleset_swap_units(const uint8_t unit) {
	vasal_engine_rules_s* const rules = vasal_editor_ruleset_rules();
	if ((unit + 1U) >= rules->unit_count) {
		return false;
	}
	vasal_editor_ruleset_begin(0U);
	vasal_engine_rules_unit_s* const first = &rules->units[unit];
	vasal_engine_rules_unit_s* const second = &rules->units[unit + 1U];
	static vasal_engine_rules_unit_s kept;
	kept = *first;
	*first = *second;
	*second = kept;
	_vasal_editor_ruleset_references(rules, vasal_engine_rules_none, unit,
		(uint8_t)(unit + 1U));
	return vasal_editor_ruleset_end();
}

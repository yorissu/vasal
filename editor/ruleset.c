
/// The rules are small, so an operation keeps a whole copy of them as they
/// were before; undoing swaps it back in and keeps the copy for redoing.
/// The history is a ring of copies.

#include "editor/ruleset.h"

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

static void _vasal_editor_ruleset_forget(void) {
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
	return true;
}

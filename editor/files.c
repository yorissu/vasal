
/// The page keeps a queue of what arrived; each frame the editor asks it
/// for the next item, which the page copies into the buffers and hands
/// over at once. Strings for the page go through the text buffer, never
/// into a script, so nothing the user typed is run.

#include "editor/files.h"
#include "engine/mapfile.h"

#include <emscripten/emscripten.h>

#include <string.h>

#define _vasal_editor_files_text_size 512U

static uint8_t _vasal_editor_files_data[vasal_engine_mapfile_capacity];
static char _vasal_editor_files_message[_vasal_editor_files_text_size];
static uint32_t _vasal_editor_files_saved = 0U;
static uint32_t _vasal_editor_files_received = 0U;
static uint32_t _vasal_editor_files_from = 0U;
static bool_t _vasal_editor_files_arrived = false;
static uint32_t _vasal_editor_files_request = 0U;
static uint32_t _vasal_editor_files_status = 0U;
static bool_t _vasal_editor_files_replied = false;

uint8_t* vasal_editor_files_buffer(void) {
	return _vasal_editor_files_data;
}

uint32_t vasal_editor_files_capacity(void) {
	return (uint32_t)sizeof(_vasal_editor_files_data);
}

uint32_t vasal_editor_files_size(void) {
	return _vasal_editor_files_saved;
}

char* vasal_editor_files_text(void) {
	return _vasal_editor_files_message;
}

uint32_t vasal_editor_files_text_capacity(void) {
	return (uint32_t)sizeof(_vasal_editor_files_message);
}

void vasal_editor_files_opened(const uint32_t size, const uint32_t origin) {
	_vasal_editor_files_received = size;
	_vasal_editor_files_from = origin;
	_vasal_editor_files_arrived = true;
}

void vasal_editor_files_answered(const uint32_t request,
	const uint32_t status) {
	_vasal_editor_files_request = request;
	_vasal_editor_files_status = status;
	_vasal_editor_files_replied = true;
}

// Whether data starts with a magic word of eight letters.
static bool_t _vasal_editor_files_starts(const uint8_t* const data,
		const size_t size, const char* const magic) {
	if (8U > size) {
		return false;
	}
	for (size_t i = 0U; i < 8U; i++) {
		if (data[i] != (uint8_t)magic[i]) {
			return false;
		}
	}
	return true;
}

// spec: editor.files#2
vasal_editor_files_kind_e vasal_editor_files_kind(const uint8_t* const data,
	const size_t size) {
	vasal_editor_files_kind_e kind = vasal_editor_files_kind_unknown;
	if (_vasal_editor_files_starts(data, size, "vasalmap")) {
		kind = vasal_editor_files_kind_map;
	} else if (_vasal_editor_files_starts(data, size, "vasalrul")) {
		kind = vasal_editor_files_kind_rules;
	} else {
		// Neither.
	}
	return kind;
}

// spec: editor.files
void vasal_editor_files_choose(void) {
	emscripten_run_script("vasalEditorOpen()");
}

bool_t vasal_editor_files_take(vasal_editor_files_arrival_s* const arrival,
	bool_t* const answered, vasal_editor_files_answer_s* const answer) {
	emscripten_run_script("vasalEditorNext()");
	*answered = _vasal_editor_files_replied;
	if (_vasal_editor_files_replied) {
		_vasal_editor_files_replied = false;
		answer->request =
			(vasal_editor_files_request_e)_vasal_editor_files_request;
		answer->status = _vasal_editor_files_status;
		answer->text = _vasal_editor_files_message;
	}
	if (!_vasal_editor_files_arrived) {
		return false;
	}
	_vasal_editor_files_arrived = false;
	arrival->data = _vasal_editor_files_data;
	arrival->size = (size_t)_vasal_editor_files_received;
	arrival->origin = (vasal_editor_files_origin_e)_vasal_editor_files_from;
	return true;
}

// Writes a name safe for a file name into the text buffer: letters,
// digits and hyphens, others as underscores, and a default for none.
static void _vasal_editor_files_name(const char* const name,
		const char* const fallback) {
	size_t at = 0U;
	for (size_t i = 0U; (i < vasal_engine_map_name_capacity)
			&& ('\0' != name[i]); i++) {
		const char character = name[i];
		const bool_t plain = ((character >= 'a') && (character <= 'z'))
			|| ((character >= 'A') && (character <= 'Z'))
			|| ((character >= '0') && (character <= '9'))
			|| ('-' == character);
		_vasal_editor_files_message[at] = '_';
		if (plain) {
			_vasal_editor_files_message[at] = character;
		}
		at++;
	}
	if (0U == at) {
		for (size_t i = 0U; '\0' != fallback[i]; i++) {
			_vasal_editor_files_message[at] = fallback[i];
			at++;
		}
	}
	_vasal_editor_files_message[at] = '\0';
}

// Copies strings into the text buffer, a line each.
static void _vasal_editor_files_lines(const char* const first,
		const char* const second) {
	size_t at = 0U;
	const size_t room = sizeof(_vasal_editor_files_message) - 2U;
	for (size_t i = 0U; ('\0' != first[i]) && (at < room); i++) {
		_vasal_editor_files_message[at] = first[i];
		at++;
	}
	_vasal_editor_files_message[at] = '\n';
	at++;
	for (size_t i = 0U; ('\0' != second[i]) && (at < room); i++) {
		_vasal_editor_files_message[at] = second[i];
		at++;
	}
	_vasal_editor_files_message[at] = '\0';
}

static bool_t _vasal_editor_files_write_map(
		const vasal_engine_map_s* const map) {
	const size_t size = vasal_engine_mapfile_write(map,
		_vasal_editor_files_data, sizeof(_vasal_editor_files_data));
	_vasal_editor_files_saved = (uint32_t)size;
	return 0U != size;
}

static bool_t _vasal_editor_files_write_rules(
		const vasal_engine_rules_s* const rules) {
	const size_t size = vasal_engine_rules_write(rules,
		_vasal_editor_files_data, sizeof(_vasal_editor_files_data));
	_vasal_editor_files_saved = (uint32_t)size;
	return 0U != size;
}

// spec: editor.files
bool_t vasal_editor_files_save_map(const vasal_engine_map_s* const map) {
	if (!_vasal_editor_files_write_map(map)) {
		return false;
	}
	_vasal_editor_files_name(map->name, "map");
	emscripten_run_script("vasalEditorSave('.vasalmap')");
	return true;
}

// spec: editor.rules#2
bool_t vasal_editor_files_save_rules(const vasal_engine_rules_s* const rules) {
	if (!_vasal_editor_files_write_rules(rules)) {
		return false;
	}
	_vasal_editor_files_name(rules->name, "rules");
	emscripten_run_script("vasalEditorSave('.vasalrules')");
	return true;
}

// spec: editor.rules#5
bool_t vasal_editor_files_upload_map(const vasal_engine_map_s* const map,
	const char* const address, const char* const key) {
	if (!_vasal_editor_files_write_map(map)) {
		return false;
	}
	_vasal_editor_files_lines(address, key);
	emscripten_run_script("vasalEditorUpload(0, 'map')");
	return true;
}

// spec: editor.rules#5
bool_t vasal_editor_files_upload_rules(
	const vasal_engine_rules_s* const rules, const char* const address,
	const char* const key) {
	if (!_vasal_editor_files_write_rules(rules)) {
		return false;
	}
	_vasal_editor_files_lines(address, key);
	emscripten_run_script("vasalEditorUpload(1, 'rules')");
	return true;
}

// spec: editor.rules#7
void vasal_editor_files_fetch(const vasal_editor_files_kind_e kind,
	const char* const address) {
	_vasal_editor_files_lines(address, "");
	if (vasal_editor_files_kind_map == kind) {
		emscripten_run_script("vasalEditorFetch(2, 'map')");
	} else {
		emscripten_run_script("vasalEditorFetch(3, 'rules')");
	}
}

// spec: editor.files#3
void vasal_editor_files_keep_map(const vasal_engine_map_s* const map) {
	if (_vasal_editor_files_write_map(map)) {
		emscripten_run_script("vasalEditorKeep('map')");
	}
}

// spec: editor.files#3
void vasal_editor_files_keep_rules(const vasal_engine_rules_s* const rules) {
	if (_vasal_editor_files_write_rules(rules)) {
		emscripten_run_script("vasalEditorKeep('rules')");
	}
}

// spec: editor.files#3
void vasal_editor_files_restore(void) {
	emscripten_run_script("vasalEditorRestore()");
}

void vasal_editor_files_remember(const char* const address,
	const char* const key) {
	_vasal_editor_files_lines(address, (NULL == key) ? "" : key);
	if (NULL == key) {
		emscripten_run_script("vasalEditorRemember(false)");
	} else {
		emscripten_run_script("vasalEditorRemember(true)");
	}
}

// Copies the text buffer from at up to the end of its line into a buffer.
// Returns where the next line starts.
static size_t _vasal_editor_files_line(const size_t start, char* const into,
		const size_t capacity) {
	size_t at = start;
	size_t length = 0U;
	while (('\0' != _vasal_editor_files_message[at])
			&& ('\n' != _vasal_editor_files_message[at])) {
		if ((length + 1U) < capacity) {
			into[length] = _vasal_editor_files_message[at];
			length++;
		}
		at++;
	}
	into[length] = '\0';
	return ('\n' == _vasal_editor_files_message[at]) ? (at + 1U) : at;
}

bool_t vasal_editor_files_recall(char* const address,
	const size_t address_capacity, char* const key,
	const size_t key_capacity) {
	_vasal_editor_files_message[0] = '\0';
	emscripten_run_script("vasalEditorRecall()");
	const size_t next = _vasal_editor_files_line(0U, address,
		address_capacity);
	(void)_vasal_editor_files_line(next, key, key_capacity);
	return '\0' != key[0];
}

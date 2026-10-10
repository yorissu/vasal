
/// The buffer shared with the page, and the two calls into it.

#include "editor/files.h"
#include "engine/mapfile.h"

#include <emscripten/emscripten.h>

static uint8_t _vasal_editor_files_data[vasal_engine_mapfile_capacity];
static uint32_t _vasal_editor_files_saved = 0U;
static uint32_t _vasal_editor_files_received = 0U;
static bool_t _vasal_editor_files_arrived = false;

uint8_t* vasal_editor_files_buffer(void) {
	return _vasal_editor_files_data;
}

uint32_t vasal_editor_files_capacity(void) {
	return (uint32_t)sizeof(_vasal_editor_files_data);
}

uint32_t vasal_editor_files_size(void) {
	return _vasal_editor_files_saved;
}

void vasal_editor_files_opened(const uint32_t size) {
	_vasal_editor_files_received = size;
	_vasal_editor_files_arrived = true;
}

// spec: editor.files
void vasal_editor_files_choose(void) {
	emscripten_run_script("vasalEditorOpen()");
}

bool_t vasal_editor_files_take(size_t* const size) {
	if (!_vasal_editor_files_arrived) {
		return false;
	}
	_vasal_editor_files_arrived = false;
	*size = (size_t)_vasal_editor_files_received;
	return true;
}

// spec: editor.files
bool_t vasal_editor_files_save(const vasal_engine_map_s* const map) {
	const size_t size = vasal_engine_mapfile_write(map,
		_vasal_editor_files_data, sizeof(_vasal_editor_files_data));
	if (0U == size) {
		return false;
	}
	_vasal_editor_files_saved = (uint32_t)size;

	// The download is named after the map, with only letters, digits,
	// hyphens and underscores, so the name is safe inside the script.
	static const char start[] = "vasalEditorSave('";
	static const char end[] = "')";
	char script[sizeof(start) + vasal_engine_map_name_capacity + sizeof(end)];
	size_t at = 0U;
	for (size_t i = 0U; '\0' != start[i]; i++) {
		script[at] = start[i];
		at++;
	}
	bool_t named = false;
	for (size_t i = 0U; '\0' != map->name[i]; i++) {
		const char character = map->name[i];
		const bool_t plain = ((character >= 'a') && (character <= 'z'))
			|| ((character >= 'A') && (character <= 'Z'))
			|| ((character >= '0') && (character <= '9'))
			|| ('-' == character);
		script[at] = '_';
		if (plain) {
			script[at] = character;
		}
		at++;
		named = true;
	}
	if (!named) {
		for (size_t i = 0U; i < 3U; i++) {
			script[at] = "map"[i];
			at++;
		}
	}
	for (size_t i = 0U; '\0' != end[i]; i++) {
		script[at] = end[i];
		at++;
	}
	script[at] = '\0';
	emscripten_run_script(script);
	return true;
}

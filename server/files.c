
/// Loads the web files with POSIX calls into one static buffer, and finds
/// them by request path.

#include "server/files.h"
#include "server/log.h"
#include "engine/mapfile.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string.h>

#define _vasal_server_files_html "text/html; charset=utf-8"
#define _vasal_server_files_script "text/javascript; charset=utf-8"
#define _vasal_server_files_wasm "application/wasm"
#define _vasal_server_files_binary "application/octet-stream"

// The files, in the order they are loaded. Emscripten names the script and
// WebAssembly after the page.
static vasal_server_files_file_s _vasal_server_files[] = {
	{ "/",            "client.html", _vasal_server_files_html,   NULL, 0U,
		false },
	{ "/client.js",   "client.js",   _vasal_server_files_script, NULL, 0U,
		false },
	{ "/client.wasm", "client.wasm", _vasal_server_files_wasm,   NULL, 0U,
		false },
	{ "/map",         NULL,          _vasal_server_files_binary, NULL, 0U,
		true }
};

// The map, read once to check it before it is served.
static vasal_engine_map_s _vasal_server_files_map;

#define _vasal_server_files_count \
	(sizeof(_vasal_server_files) / sizeof(_vasal_server_files[0]))

static uint8_t _vasal_server_files_storage[vasal_server_files_capacity];

// Reads a whole open file, named label in messages, into the storage, from
// offset on, and closes it. Returns false when it can't be read or does not
// fit.
static bool_t _vasal_server_files_read(const int descriptor,
		const char* const label, vasal_server_files_file_s* const file,
		size_t* const offset) {
	if (0 > descriptor) {
		vasal_server_log_write(vasal_server_log_level_error,
			"can't open web file", label);
		return false;
	}

	bool_t read_all = false;
	struct stat status;
	if ((0 != fstat(descriptor, &status)) || !S_ISREG(status.st_mode)) {
		vasal_server_log_write(vasal_server_log_level_error,
			"web file is not a regular file", label);
	} else if ((size_t)status.st_size
			> (vasal_server_files_capacity - *offset)) {
		vasal_server_log_write(vasal_server_log_level_error,
			"web files are larger than the server allows", label);
	} else {
		const size_t size = (size_t)status.st_size;
		size_t done = 0U;
		read_all = true;
		while (read_all && (done < size)) {
			const ssize_t count = read(descriptor,
				&_vasal_server_files_storage[*offset + done], size - done);
			if (0 >= count) {
				vasal_server_log_write(vasal_server_log_level_error,
					"can't read web file", label);
				read_all = false;
			} else {
				done += (size_t)count;
			}
		}
		if (read_all) {
			file->data = &_vasal_server_files_storage[*offset];
			file->size = size;
			*offset += size;
		}
	}

	(void)close(descriptor);
	return read_all;
}

// Messages for the outcomes of reading a map file.
static const char* const _vasal_server_files_map_problems[] = {
	"read", "not a map file", "made for another version", "cut short",
	"corrupted", "holding a value out of range"
};

// Loads the map file into the last entry and checks that it reads as a
// map.
static bool_t _vasal_server_files_load_map(const char* const path,
		size_t* const offset) {
	vasal_server_files_file_s* const file =
		&_vasal_server_files[_vasal_server_files_count - 1U];
	const int descriptor = open(path, O_RDONLY | O_CLOEXEC);
	if (!_vasal_server_files_read(descriptor, path, file, offset)) {
		return false;
	}
	uint16_t version = 0U;
	const vasal_engine_mapfile_read_e outcome = vasal_engine_mapfile_read(
		file->data, file->size, &_vasal_server_files_map, &version);
	if (vasal_engine_mapfile_read_ok != outcome) {
		vasal_server_log_write(vasal_server_log_level_error,
			"the map file is", _vasal_server_files_map_problems[outcome]);
		file->data = NULL;
		return false;
	}
	return true;
}

// spec: server.web mapfile.validation
bool_t vasal_server_files_load(const char* const folder,
	const char* const map) {
	const int directory = open(folder, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
	if (0 > directory) {
		vasal_server_log_write(vasal_server_log_level_error,
			"can't open the web folder", folder);
		return false;
	}

	bool_t loaded = true;
	size_t offset = 0U;
	for (size_t i = 0U; loaded && (i < _vasal_server_files_count); i++) {
		vasal_server_files_file_s* const file = &_vasal_server_files[i];
		if (NULL != file->name) {
			const int descriptor = openat(directory, file->name,
				O_RDONLY | O_CLOEXEC);
			loaded = _vasal_server_files_read(descriptor, file->name, file,
				&offset);
		}
	}
	(void)close(directory);

	if (loaded && (NULL != map)) {
		loaded = _vasal_server_files_load_map(map, &offset);
	}
	return loaded;
}

// spec: server.web#1
const vasal_server_files_file_s* vasal_server_files_find(
	const char* const path, const size_t length) {
	for (size_t i = 0U; i < _vasal_server_files_count; i++) {
		const vasal_server_files_file_s* const file = &_vasal_server_files[i];
		const bool_t served = !file->optional || (NULL != file->data);
		if (served && (strlen(file->path) == length)) {
			bool_t same = true;
			for (size_t j = 0U; same && (j < length); j++) {
				same = file->path[j] == path[j];
			}
			if (same) {
				return file;
			}
		}
	}
	return NULL;
}

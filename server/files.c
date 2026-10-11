
/// The web files are read into one block of memory at start-up and never
/// change. The map and the rules have a block each, read at start-up and
/// replaced by uploads: an upload is checked, written next to the file it
/// replaces and renamed over it, so a crash leaves the old or the new file,
/// never half of one, and only then copied into the block.

#include "server/files.h"
#include "engine/mapfile.h"
#include "engine/rules.h"
#include "server/log.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdio.h>
#include <string.h>

#define _vasal_server_files_html "text/html; charset=utf-8"
#define _vasal_server_files_script "text/javascript; charset=utf-8"
#define _vasal_server_files_wasm "application/wasm"
#define _vasal_server_files_binary "application/octet-stream"

// The files, in the order they are loaded. Emscripten names the script and
// WebAssembly after the page. The map and the rules come last.
static vasal_server_files_file_s _vasal_server_files[] = {
	{ "/",            "client.html", _vasal_server_files_html,   NULL, 0U,
		false },
	{ "/client.js",   "client.js",   _vasal_server_files_script, NULL, 0U,
		false },
	{ "/client.wasm", "client.wasm", _vasal_server_files_wasm,   NULL, 0U,
		false },
	{ "/map",         NULL,          _vasal_server_files_binary, NULL, 0U,
		true },
	{ "/rules",       NULL,          _vasal_server_files_binary, NULL, 0U,
		true }
};

#define _vasal_server_files_count \
	(sizeof(_vasal_server_files) / sizeof(_vasal_server_files[0]))
#define _vasal_server_files_map_entry (_vasal_server_files_count - 2U)
#define _vasal_server_files_rules_entry (_vasal_server_files_count - 1U)

static uint8_t _vasal_server_files_storage[vasal_server_files_capacity];
static uint8_t _vasal_server_files_map_block[vasal_engine_mapfile_capacity];
static uint8_t _vasal_server_files_rules_block[vasal_engine_rules_capacity];

// Where the map and the rules live on disk, NULL when they have no file.
static const char* _vasal_server_files_map_path = NULL;
static const char* _vasal_server_files_rules_path = NULL;

// Reads a whole open file, named label in messages, into a block of
// capacity bytes from offset on, and closes it. Returns false when it
// can't be read or does not fit.
static bool_t _vasal_server_files_read(const int descriptor,
		const char* const label, uint8_t* const block, const size_t capacity,
		size_t* const offset, vasal_server_files_file_s* const file) {
	if (0 > descriptor) {
		vasal_server_log_write(vasal_server_log_level_error,
			"can't open file", label);
		return false;
	}

	bool_t read_all = false;
	struct stat status;
	if ((0 != fstat(descriptor, &status)) || !S_ISREG(status.st_mode)) {
		vasal_server_log_write(vasal_server_log_level_error,
			"file is not a regular file", label);
	} else if ((size_t)status.st_size > (capacity - *offset)) {
		vasal_server_log_write(vasal_server_log_level_error,
			"file is larger than the server allows", label);
	} else {
		const size_t size = (size_t)status.st_size;
		size_t done = 0U;
		read_all = true;
		while (read_all && (done < size)) {
			const ssize_t count = read(descriptor, &block[*offset + done],
				size - done);
			if (0 >= count) {
				vasal_server_log_write(vasal_server_log_level_error,
					"can't read file", label);
				read_all = false;
			} else {
				done += (size_t)count;
			}
		}
		if (read_all) {
			file->data = &block[*offset];
			file->size = size;
			*offset += size;
		}
	}

	(void)close(descriptor);
	return read_all;
}

// Loads the map or the rules from a path into its block and checks it.
// A path without a file is fine: the map is then not served yet, the
// rules are the standard ones; uploads will write it.
static bool_t _vasal_server_files_load_content(const char* const path,
		const vasal_server_upload_kind_e kind) {
	const bool_t map = vasal_server_upload_map == kind;
	vasal_server_files_file_s* const file = &_vasal_server_files[map
		? _vasal_server_files_map_entry : _vasal_server_files_rules_entry];
	uint8_t* const block = map ? _vasal_server_files_map_block
		: _vasal_server_files_rules_block;
	const size_t capacity = map ? sizeof(_vasal_server_files_map_block)
		: sizeof(_vasal_server_files_rules_block);
	if ((NULL == path) || (0 != access(path, F_OK))) {
		if (NULL != path) {
			vasal_server_log_write(vasal_server_log_level_information,
				map ? "no map file yet, until one is uploaded"
					: "no rules file yet: serving the standard rules", path);
		}
		if (!map) {
			// spec: rules.content#3 rules.content#6
			static vasal_engine_rules_s standard;
			vasal_engine_rules_standard(&standard);
			file->size = vasal_engine_rules_write(&standard, block, capacity);
			file->data = block;
		}
		return true;
	}
	size_t offset = 0U;
	const int descriptor = open(path, O_RDONLY | O_CLOEXEC);
	if (!_vasal_server_files_read(descriptor, path, block, capacity, &offset,
			file)) {
		return false;
	}
	const char* const problem = vasal_server_upload_check(kind, file->data,
		file->size);
	if (NULL != problem) {
		vasal_server_log_write(vasal_server_log_level_error, problem, path);
		file->data = NULL;
		file->size = 0U;
		return false;
	}
	return true;
}

// spec: server.web mapfile.validation rules.content#6
bool_t vasal_server_files_load(const char* const folder,
	const char* const map, const char* const rules) {
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
			loaded = _vasal_server_files_read(descriptor, file->name,
				_vasal_server_files_storage, vasal_server_files_capacity,
				&offset, file);
		}
	}
	(void)close(directory);

	_vasal_server_files_map_path = map;
	_vasal_server_files_rules_path = rules;
	_vasal_server_files[_vasal_server_files_map_entry].data = NULL;
	_vasal_server_files[_vasal_server_files_map_entry].size = 0U;
	return loaded
		&& _vasal_server_files_load_content(map, vasal_server_upload_map)
		&& _vasal_server_files_load_content(rules, vasal_server_upload_rules);
}

// Writes a file in full to a path: into the path with ".new" appended,
// then renamed over it. Returns false when that failed.
static bool_t _vasal_server_files_save(const char* const path,
		const uint8_t* const data, const size_t size) {
	char staged[4096];
	const size_t length = strlen(path);
	if ((length + 5U) > sizeof(staged)) {
		return false;
	}
	static const char suffix[] = ".new";
	for (size_t i = 0U; i < length; i++) {
		staged[i] = path[i];
	}
	for (size_t i = 0U; i < sizeof(suffix); i++) {
		staged[length + i] = suffix[i];
	}
	const int descriptor = open(staged, O_WRONLY | O_CREAT | O_TRUNC
		| O_CLOEXEC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (0 > descriptor) {
		return false;
	}
	size_t done = 0U;
	bool_t written = true;
	while (written && (done < size)) {
		const ssize_t count = write(descriptor, &data[done], size - done);
		written = 0 < count;
		if (written) {
			done += (size_t)count;
		}
	}
	written = written && (0 == fsync(descriptor));
	written = (0 == close(descriptor)) && written;
	return written && (0 == rename(staged, path));
}

// spec: rules.content#7
const char* vasal_server_files_replace(const vasal_server_upload_kind_e kind,
	const uint8_t* const data, const size_t size) {
	const char* const problem = vasal_server_upload_check(kind, data, size);
	if (NULL != problem) {
		return problem;
	}
	const bool_t map = vasal_server_upload_map == kind;
	const char* const path = map ? _vasal_server_files_map_path
		: _vasal_server_files_rules_path;
	if ((NULL != path) && !_vasal_server_files_save(path, data, size)) {
		vasal_server_log_write(vasal_server_log_level_error,
			"can't save the uploaded file", path);
		return "the server could not save the file\n";
	}
	vasal_server_files_file_s* const file = vasal_server_files_content(kind);
	uint8_t* const block = map ? _vasal_server_files_map_block
		: _vasal_server_files_rules_block;
	(void)memcpy(block, data, size);
	file->data = block;
	file->size = size;
	vasal_server_log_write(vasal_server_log_level_information,
		map ? "a new map is served" : "new rules are served",
		(NULL == path) ? "(kept in memory only)" : path);
	return NULL;
}

vasal_server_files_file_s* vasal_server_files_content(
	const vasal_server_upload_kind_e kind) {
	return &_vasal_server_files[(vasal_server_upload_map == kind)
		? _vasal_server_files_map_entry : _vasal_server_files_rules_entry];
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


/// The web files the server serves: the client's page, script and
/// WebAssembly, and the match's map file when there is one. The map editor
/// is not among them: it is a standalone file.
/// The list is fixed; the files are loaded into memory at start-up and
/// nothing else is ever read from disk, so no request path can reach any
/// other file.

#ifndef vasal__server__files_h
#define vasal__server__files_h

#include "engine/types.h"

/// The most bytes all files may take together.
#define vasal_server_files_capacity ((size_t)64U * 1024U * 1024U)

/// One file the server serves.
typedef struct vasal_server_files_file_s {
	/// The request path it is served at, such as "/client.js".
	const char*    path;
	/// Its name in the web folder; NULL for the map, which comes from a
	/// path of its own.
	const char*    name;
	/// Its content type.
	const char*    type;
	/// Its content, once loaded; NULL before.
	const uint8_t* data;
	/// Its size in bytes.
	size_t         size;
	/// Whether the server may run without it (the map); until it is
	/// loaded, it is not served.
	bool_t         optional;
} vasal_server_files_file_s;

/// Loads every file from the folder into memory, and the map file from its
/// path unless map is NULL. The map must read as a valid map file. Returns
/// false, after logging which file failed and why, when a file is missing,
/// unreadable or an invalid map, or the files together exceed
/// vasal_server_files_capacity.
bool_t vasal_server_files_load(const char* const folder,
	const char* const map);

/// Finds the file served at a request path (length bytes, not terminated),
/// or returns NULL. Only exact matches count.
const vasal_server_files_file_s* vasal_server_files_find(
	const char* const path, const size_t length);

#endif

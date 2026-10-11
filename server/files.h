
/// The web files the server serves: the client's page, script and
/// WebAssembly, the match's map file when there is one, and its rules file
/// (the standard rules without one). The map editor is not among them: it
/// is a standalone file. The map and the rules can be replaced by uploads
/// (server/upload.h).
/// The list is fixed; the files are loaded into memory at start-up and
/// nothing else is ever read from disk, so no request path can reach any
/// other file.

#ifndef vasal__server__files_h
#define vasal__server__files_h

#include "server/upload.h"

/// The most bytes all files may take together.
#define vasal_server_files_capacity ((size_t)64U * 1024U * 1024U)

/// One file the server serves.
typedef struct vasal_server_files_file_s {
	/// The request path it is served at, such as "/client.js".
	const char*    path;
	/// Its name in the web folder; NULL for the map and the rules, which
	/// come from paths of their own.
	const char*    name;
	/// Its content type.
	const char*    type;
	/// Its content, once loaded; NULL before.
	const uint8_t* data;
	/// Its size in bytes.
	size_t         size;
	/// Whether it is served only once it is loaded (the map and the
	/// rules).
	bool_t         optional;
} vasal_server_files_file_s;

/// Loads every file from the folder into memory, the map file from its
/// path and the rules file from its path. Either path may be NULL, or name
/// a file that doesn't exist yet: the map is then not served until one is
/// uploaded, and the standard rules are served. Uploads are written to
/// those paths. Returns false, after logging which file failed and why,
/// when a web file is missing or unreadable, a map or rules file doesn't
/// read, or the files together exceed vasal_server_files_capacity.
bool_t vasal_server_files_load(const char* const folder,
	const char* const map, const char* const rules);

/// Replaces the map or the rules with an uploaded file: checks it, writes
/// it to its path when it has one, and serves it from then on. Returns
/// NULL when it did, else a sentence saying why not; the old one stays
/// then.
const char* vasal_server_files_replace(const vasal_server_upload_kind_e kind,
	const uint8_t* const data, const size_t size);

/// The served map or rules file.
vasal_server_files_file_s* vasal_server_files_content(
	const vasal_server_upload_kind_e kind);

/// Finds the file served at a request path (length bytes, not terminated),
/// or returns NULL. Only exact matches count.
const vasal_server_files_file_s* vasal_server_files_find(
	const char* const path, const size_t length);

#endif

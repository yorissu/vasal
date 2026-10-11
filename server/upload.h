
/// Uploads of the game's content from the editor: a POST of a map file to
/// /upload/map or of a rules file to /upload/rules, with the admin key in
/// the query (?key=...). These are the checks, free of any I/O, so they
/// can be tested; the network loop receives the file and server/files.c
/// keeps it.

#ifndef vasal__server__upload_h
#define vasal__server__upload_h

#include "engine/types.h"

/// What an upload replaces.
typedef enum vasal_server_upload_kind_e {
	vasal_server_upload_map,
	vasal_server_upload_rules,
	/// The path is no upload's.
	vasal_server_upload_none
} vasal_server_upload_kind_e;

/// What an upload path (length bytes) asks to replace.
vasal_server_upload_kind_e vasal_server_upload_kind(const char* const path,
	const size_t length);

/// Whether a query (length bytes) holds the admin key as key=..., the
/// whole of it. Always false without a key (NULL or empty): uploads are
/// off then. Compares in a time that doesn't depend on where the key
/// differs.
bool_t vasal_server_upload_authorized(const char* const query,
	const size_t length, const char* const key);

/// The largest file of a kind, in bytes.
size_t vasal_server_upload_capacity(const vasal_server_upload_kind_e kind);

/// Checks a file of a kind: NULL when it reads well, else a sentence
/// saying why not, for the response.
const char* vasal_server_upload_check(const vasal_server_upload_kind_e kind,
	const uint8_t* const data, const size_t size);

#endif

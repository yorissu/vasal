
/// The files are checked by the same engine readers the clients use, so a
/// file the server takes is one every client can read.

#include "server/upload.h"
#include "engine/mapfile.h"
#include "engine/rules.h"

#include <string.h>

// Sentences for the outcomes of reading map and rules files, which share
// their order.
static const char* const _vasal_server_upload_problems[] = {
	NULL,
	"that is not a map or rules file of this kind\n",
	"that file was made for another version\n",
	"that file is cut short or too long\n",
	"that file is corrupted\n",
	"that file holds a value out of range or a missing reference\n"
};

// Whether the path is the given text.
static bool_t _vasal_server_upload_is(const char* const path,
		const size_t length, const char* const text) {
	return (strlen(text) == length) && (0 == strncmp(path, text, length));
}

// spec: rules.content#7
vasal_server_upload_kind_e vasal_server_upload_kind(const char* const path,
	const size_t length) {
	vasal_server_upload_kind_e kind = vasal_server_upload_none;
	if (_vasal_server_upload_is(path, length, "/upload/map")) {
		kind = vasal_server_upload_map;
	} else if (_vasal_server_upload_is(path, length, "/upload/rules")) {
		kind = vasal_server_upload_rules;
	} else {
		// No upload.
	}
	return kind;
}

// spec: rules.content#7
bool_t vasal_server_upload_authorized(const char* const query,
	const size_t length, const char* const key) {
	if ((NULL == key) || ('\0' == key[0]) || (4U > length)
			|| (0 != strncmp(query, "key=", 4U))) {
		return false;
	}
	const size_t given = length - 4U;
	const size_t wanted = strlen(key);
	uint32_t difference = 0U;
	if (given != wanted) {
		difference = 1U;
	}
	for (size_t i = 0U; i < wanted; i++) {
		const uint8_t a = (uint8_t)key[i];
		const uint8_t b = (i < given) ? (uint8_t)query[4U + i] : 0U;
		difference |= (uint32_t)a ^ (uint32_t)b;
	}
	return 0U == difference;
}

size_t vasal_server_upload_capacity(const vasal_server_upload_kind_e kind) {
	size_t capacity = 0U;
	if (vasal_server_upload_map == kind) {
		capacity = vasal_engine_mapfile_capacity;
	} else if (vasal_server_upload_rules == kind) {
		capacity = vasal_engine_rules_capacity;
	} else {
		// Nothing can be uploaded there.
	}
	return capacity;
}

// spec: rules.content#7 mapfile.validation#1
const char* vasal_server_upload_check(const vasal_server_upload_kind_e kind,
	const uint8_t* const data, const size_t size) {
	static vasal_engine_map_s map;
	static vasal_engine_rules_s rules;
	uint16_t version = 0U;
	size_t outcome = 1U;
	if (vasal_server_upload_map == kind) {
		outcome = (size_t)vasal_engine_mapfile_read(data, size, &map,
			&version);
	} else if (vasal_server_upload_rules == kind) {
		outcome = (size_t)vasal_engine_rules_read(data, size, &rules,
			&version);
	} else {
		// Not an upload.
	}
	return _vasal_server_upload_problems[outcome];
}

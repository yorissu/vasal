
/// Tests of uploads: their paths, the admin key, the checks, and the
/// served files they replace, on a web folder made in /tmp.

#include "server/files.h"
#include "server/upload.h"
#include "engine/mapfile.h"
#include "engine/rules.h"

#include "cairo.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdio.h>
#include <string.h>

static uint8_t _vasal_tests_server_upload_file[vasal_engine_mapfile_capacity];
static vasal_engine_rules_s _vasal_tests_server_upload_rules;
static vasal_engine_map_s _vasal_tests_server_upload_map;

// spec: rules.content#7
cairo_test_new(server_upload, paths_name_what_they_replace) {
	cairo_assert_eq(vasal_server_upload_kind("/upload/map", 11U),
		vasal_server_upload_map);
	cairo_assert_eq(vasal_server_upload_kind("/upload/rules", 13U),
		vasal_server_upload_rules);
	cairo_assert_eq(vasal_server_upload_kind("/upload/rule", 12U),
		vasal_server_upload_none);
	cairo_assert_eq(vasal_server_upload_kind("/map", 4U),
		vasal_server_upload_none);
}

// spec: rules.content#7
cairo_test_new(server_upload, only_the_admin_key_may_upload) {
	cairo_assert_true(vasal_server_upload_authorized("key=open sesame", 15U,
		"open sesame"));
	cairo_assert_false(vasal_server_upload_authorized("key=open sesam", 14U,
		"open sesame"));
	cairo_assert_false(vasal_server_upload_authorized("key=open sesame!", 16U,
		"open sesame"));
	cairo_assert_false(vasal_server_upload_authorized("kex=open sesame", 15U,
		"open sesame"));
	cairo_assert_false(vasal_server_upload_authorized("", 0U, "open sesame"));
	// No key: uploads are off.
	cairo_assert_false(vasal_server_upload_authorized("key=", 4U, NULL));
	cairo_assert_false(vasal_server_upload_authorized("key=", 4U, ""));
}

// spec: rules.content#7
cairo_test_new(server_upload, files_are_checked_as_clients_read_them) {
	vasal_engine_rules_standard(&_vasal_tests_server_upload_rules);
	const size_t size = vasal_engine_rules_write(
		&_vasal_tests_server_upload_rules, _vasal_tests_server_upload_file,
		sizeof(_vasal_tests_server_upload_file));
	cairo_assert_true(NULL == vasal_server_upload_check(
		vasal_server_upload_rules, _vasal_tests_server_upload_file, size));
	// A rules file is no map.
	cairo_assert_true(NULL != vasal_server_upload_check(
		vasal_server_upload_map, _vasal_tests_server_upload_file, size));
	_vasal_tests_server_upload_file[20] ^= 0x5aU;
	cairo_assert_true(NULL != vasal_server_upload_check(
		vasal_server_upload_rules, _vasal_tests_server_upload_file, size));
	cairo_assert_gt(vasal_server_upload_capacity(vasal_server_upload_map),
		vasal_server_upload_capacity(vasal_server_upload_rules));
}

// Writes a file into a folder.
static void _vasal_tests_server_upload_put(const char* const folder,
		const char* const name, const uint8_t* const data, const size_t size) {
	char path[256];
	(void)snprintf(path, sizeof(path), "%s/%s", folder, name);
	const int descriptor = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (0 <= descriptor) {
		// A short write shows as the file being refused in the test.
		if (0 > write(descriptor, data, size)) {
			(void)unlink(path);
		}
		(void)close(descriptor);
	}
}

// A web folder in /tmp with the client's three files.
static const char* _vasal_tests_server_upload_folder(void) {
	static char folder[] = "/tmp/vasal_tests_upload_XXXXXX";
	static bool_t made = false;
	if (!made) {
		made = NULL != mkdtemp(folder);
	}
	static const uint8_t page[] = "page";
	_vasal_tests_server_upload_put(folder, "client.html", page, 4U);
	_vasal_tests_server_upload_put(folder, "client.js", page, 4U);
	_vasal_tests_server_upload_put(folder, "client.wasm", page, 4U);
	return folder;
}

// spec: rules.content#6 rules.content#3
cairo_test_new(server_upload, the_standard_rules_are_served_without_a_file) {
	const char* const folder = _vasal_tests_server_upload_folder();
	char rules[256];
	(void)snprintf(rules, sizeof(rules), "%s/missing.vasalrules", folder);
	cairo_assert_true(vasal_server_files_load(folder, NULL, rules));
	const vasal_server_files_file_s* const file =
		vasal_server_files_find("/rules", 6U);
	cairo_assert_true(NULL != file);
	uint16_t version = 0U;
	cairo_assert_eq(vasal_engine_rules_read(file->data, file->size,
		&_vasal_tests_server_upload_rules, &version),
		vasal_engine_rules_read_ok);
	cairo_assert_true(0 == strcmp(_vasal_tests_server_upload_rules.name,
		"standard"));
	// No map until one is uploaded.
	cairo_assert_true(NULL == vasal_server_files_find("/map", 4U));
}

// spec: rules.content#7 rules.content#6
cairo_test_new(server_upload, uploads_are_kept_and_served) {
	const char* const folder = _vasal_tests_server_upload_folder();
	char rules[256];
	char map[256];
	(void)snprintf(rules, sizeof(rules), "%s/game.vasalrules", folder);
	(void)snprintf(map, sizeof(map), "%s/game.vasalmap", folder);
	(void)unlink(rules);
	(void)unlink(map);
	cairo_assert_true(vasal_server_files_load(folder, map, rules));

	vasal_engine_rules_standard(&_vasal_tests_server_upload_rules);
	(void)strcpy(_vasal_tests_server_upload_rules.name, "designed");
	size_t size = vasal_engine_rules_write(&_vasal_tests_server_upload_rules,
		_vasal_tests_server_upload_file,
		sizeof(_vasal_tests_server_upload_file));
	cairo_assert_true(NULL == vasal_server_files_replace(
		vasal_server_upload_rules, _vasal_tests_server_upload_file, size));
	const vasal_server_files_file_s* const served =
		vasal_server_files_find("/rules", 6U);
	cairo_assert_eq(served->size, size);
	cairo_assert_true(0 == memcmp(served->data,
		_vasal_tests_server_upload_file, size));
	cairo_assert_eq(access(rules, F_OK), 0);

	(void)vasal_engine_map_clear(&_vasal_tests_server_upload_map, 30U, 20U);
	size = vasal_engine_mapfile_write(&_vasal_tests_server_upload_map,
		_vasal_tests_server_upload_file,
		sizeof(_vasal_tests_server_upload_file));
	cairo_assert_true(NULL == vasal_server_files_replace(
		vasal_server_upload_map, _vasal_tests_server_upload_file, size));
	cairo_assert_true(NULL != vasal_server_files_find("/map", 4U));

	// A broken upload is refused, and the served rules stay.
	cairo_assert_true(NULL != vasal_server_files_replace(
		vasal_server_upload_rules, _vasal_tests_server_upload_file, 10U));
	uint16_t version = 0U;
	cairo_assert_eq(vasal_engine_rules_read(served->data, served->size,
		&_vasal_tests_server_upload_rules, &version),
		vasal_engine_rules_read_ok);
	cairo_assert_true(0 == strcmp(_vasal_tests_server_upload_rules.name,
		"designed"));

	// The server started again reads what was uploaded.
	cairo_assert_true(vasal_server_files_load(folder, map, rules));
	cairo_assert_true(NULL != vasal_server_files_find("/map", 4U));
	const vasal_server_files_file_s* const again =
		vasal_server_files_find("/rules", 6U);
	cairo_assert_eq(vasal_engine_rules_read(again->data, again->size,
		&_vasal_tests_server_upload_rules, &version),
		vasal_engine_rules_read_ok);
	cairo_assert_true(0 == strcmp(_vasal_tests_server_upload_rules.name,
		"designed"));
}

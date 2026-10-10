
/// Tests of how the server answers requests: which status and which file.

#include "server/response.h"

#include "cairo.h"

#include <string.h>

static vasal_server_response_s _vasal_tests_server_response_get(
		const char* const path) {
	const vasal_server_http_request_s request = {
		.method      = vasal_server_http_method_get,
		.path        = path,
		.path_length = strlen(path)
	};
	return vasal_server_response_choose(vasal_server_http_parse_complete,
		&request);
}

// spec: server.web#1 editor.platform#2
cairo_test_new(server_response, unknown_paths_are_not_found) {
	static const char* const paths[] = {
		"/nothing", "/../etc/passwd", "//client.js", "/client.js/",
		"/CLIENT.JS", "/client.js%00", "/./client.js", "/editor/",
		"/client.html", "/client", "/editor", "/editor.html", "/editor.js",
		"/editor.wasm", "/map"
	};
	for (size_t i = 0U; i < (sizeof paths / sizeof paths[0]); i++) {
		const vasal_server_response_s response =
			_vasal_tests_server_response_get(paths[i]);
		cairo_assert_eq(response.status, vasal_server_http_status_not_found);
		cairo_assert_true(NULL == response.file);
	}
}

// spec: server.web#2
cairo_test_new(server_response, only_get_is_allowed) {
	const vasal_server_http_request_s request = {
		.method      = vasal_server_http_method_other,
		.path        = "/",
		.path_length = 1U
	};
	const vasal_server_response_s response = vasal_server_response_choose(
		vasal_server_http_parse_complete, &request);
	cairo_assert_eq(response.status,
		vasal_server_http_status_method_not_allowed);
	cairo_assert_true(NULL == response.file);
}

// spec: server.web#3
cairo_test_new(server_response, files_have_their_content_types) {
	static const char* const pairs[][2] = {
		{ "/",            "text/html; charset=utf-8" },
		{ "/client.js",   "text/javascript; charset=utf-8" },
		{ "/client.wasm", "application/wasm" }
	};
	for (size_t i = 0U; i < (sizeof pairs / sizeof pairs[0]); i++) {
		const vasal_server_response_s response =
			_vasal_tests_server_response_get(pairs[i][0]);
		cairo_assert_eq(response.status, vasal_server_http_status_ok);
		cairo_assert_true(NULL != response.file);
		cairo_assert_true(0 == strcmp(response.file->type, pairs[i][1]));
	}
}

// spec: network.handshake#2
cairo_test_new(server_response, malformed_requests_are_bad) {
	const vasal_server_http_request_s request = { 0 };
	const vasal_server_response_s response = vasal_server_response_choose(
		vasal_server_http_parse_malformed, &request);
	cairo_assert_eq(response.status, vasal_server_http_status_bad_request);
	cairo_assert_true(NULL == response.file);
}

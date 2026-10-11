
/// Tests of reading request heads and writing response heads.

#include "server/http.h"

#include "cairo.h"

#include <string.h>

static vasal_server_http_parse_e _vasal_tests_server_http_parse(
		const char* const text, vasal_server_http_request_s* const request) {
	return vasal_server_http_parse(text, strlen(text), request);
}

// spec: server.web#1
cairo_test_new(server_http, reads_a_get_request) {
	vasal_server_http_request_s request = { 0 };
	const char* const text =
		"GET /client.js HTTP/1.1\r\nHost: localhost\r\nAccept: */*\r\n\r\n";
	cairo_assert_eq(_vasal_tests_server_http_parse(text, &request),
		vasal_server_http_parse_complete);
	cairo_assert_eq(request.method, vasal_server_http_method_get);
	cairo_assert_eq(request.path_length, strlen("/client.js"));
	cairo_assert_eq(0, memcmp(request.path, "/client.js", request.path_length));
}

// spec: server.web#1
cairo_test_new(server_http, path_ends_at_query_and_fragment) {
	vasal_server_http_request_s request = { 0 };
	cairo_assert_eq(_vasal_tests_server_http_parse(
		"GET /?name=arn HTTP/1.1\r\n\r\n", &request),
		vasal_server_http_parse_complete);
	cairo_assert_eq(request.path_length, 1U);
	cairo_assert_eq(_vasal_tests_server_http_parse(
		"GET /editor#map HTTP/1.0\r\n\r\n", &request),
		vasal_server_http_parse_complete);
	cairo_assert_eq(request.path_length, strlen("/editor"));
}

// spec: server.web#2
cairo_test_new(server_http, other_methods_are_other) {
	vasal_server_http_request_s request = { 0 };
	cairo_assert_eq(_vasal_tests_server_http_parse(
		"PUT / HTTP/1.1\r\nContent-Length: 0\r\n\r\n", &request),
		vasal_server_http_parse_complete);
	cairo_assert_eq(request.method, vasal_server_http_method_other);
	cairo_assert_eq(_vasal_tests_server_http_parse(
		"GETS / HTTP/1.1\r\n\r\n", &request),
		vasal_server_http_parse_complete);
	cairo_assert_eq(request.method, vasal_server_http_method_other);
}

// spec: network.handshake#2
cairo_test_new(server_http, waits_for_the_whole_head) {
	vasal_server_http_request_s request = { 0 };
	cairo_assert_eq(_vasal_tests_server_http_parse("", &request),
		vasal_server_http_parse_incomplete);
	cairo_assert_eq(_vasal_tests_server_http_parse(
		"GET / HTTP/1.1\r\nHost: x\r\n", &request),
		vasal_server_http_parse_incomplete);
}

// spec: network.handshake#2
cairo_test_new(server_http, refuses_malformed_heads) {
	static const char* const heads[] = {
		"GET client.js HTTP/1.1\r\n\r\n",
		"GET  / HTTP/1.1\r\n\r\n",
		"GET / HTTP/2.0\r\n\r\n",
		"GET / http/1.1\r\n\r\n",
		" / HTTP/1.1\r\n\r\n",
		"GET /\x01 HTTP/1.1\r\n\r\n",
		"GET / HTTP/1.1\r\nHost localhost\r\n\r\n",
		"GET / HTTP/1.1\r\n: no name\r\n\r\n",
		"GET / HTTP/1.1\r\nHost: a\nb\r\n\r\n",
		"GET / HTTP/1.1\r\nHost: a\x7f\r\n\r\n",
		"GET / HTTP/1.1\n\r\n\r\n"
	};
	for (size_t i = 0U; i < (sizeof heads / sizeof heads[0]); i++) {
		vasal_server_http_request_s request = { 0 };
		cairo_assert_eq(_vasal_tests_server_http_parse(heads[i], &request),
			vasal_server_http_parse_malformed);
	}
}

// spec: network.handshake#2
cairo_test_new(server_http, refuses_heads_beyond_the_capacity) {
	static char head[vasal_server_http_request_capacity];
	(void)memset(head, 'a', sizeof head);
	(void)memcpy(head, "GET / HTTP/1.1\r\nX: ", 19U);
	vasal_server_http_request_s request = { 0 };
	cairo_assert_eq(vasal_server_http_parse(head, sizeof head, &request),
		vasal_server_http_parse_malformed);
}

// spec: server.web#3
cairo_test_new(server_http, writes_a_response_head) {
	char head[512];
	const size_t length = vasal_server_http_head(head, sizeof head - 1U,
		vasal_server_http_status_ok, "application/wasm", 1193104U);
	cairo_assert_gt(length, 0U);
	head[length] = '\0';
	cairo_assert_true(0 == strncmp(head, "HTTP/1.1 200 OK\r\n", 17U));
	cairo_assert_true(NULL != strstr(head,
		"Content-Type: application/wasm\r\n"));
	cairo_assert_true(NULL != strstr(head, "Content-Length: 1193104\r\n"));
	cairo_assert_true(NULL != strstr(head, "Connection: close\r\n"));
	cairo_assert_true(0 == strcmp(&head[length - 4U], "\r\n\r\n"));
}

// spec: server.web#2
cairo_test_new(server_http, method_not_allowed_names_get) {
	char head[512];
	const size_t length = vasal_server_http_head(head, sizeof head - 1U,
		vasal_server_http_status_method_not_allowed, "text/plain", 0U);
	cairo_assert_gt(length, 0U);
	head[length] = '\0';
	cairo_assert_true(0 == strncmp(head, "HTTP/1.1 405 ", 13U));
	cairo_assert_true(NULL != strstr(head, "\r\nAllow: GET, POST\r\n"));
}

// spec: rules.content#7
cairo_test_new(server_http, reads_an_upload_head) {
	vasal_server_http_request_s request = { 0 };
	static const char head[] = "POST /upload/rules?key=s3cret#x HTTP/1.1\r\n"
		"Host: localhost\r\ncontent-LENGTH:  1234 \r\n\r\nbody";
	cairo_assert_eq(_vasal_tests_server_http_parse(head, &request),
		vasal_server_http_parse_complete);
	cairo_assert_eq(request.method, vasal_server_http_method_post);
	cairo_assert_eq(request.path_length, 13U);
	cairo_assert_true(0 == strncmp(request.path, "/upload/rules", 13U));
	cairo_assert_eq(request.query_length, 10U);
	cairo_assert_true(0 == strncmp(request.query, "key=s3cret", 10U));
	cairo_assert_eq(request.content_length, 1234U);
	cairo_assert_eq(request.head_length, sizeof(head) - 5U);
	// A length that isn't a number is malformed.
	cairo_assert_eq(_vasal_tests_server_http_parse("POST / HTTP/1.1\r\n"
		"Content-Length: 12a\r\n\r\n", &request),
		vasal_server_http_parse_malformed);
}

// spec: rules.content#7
cairo_test_new(server_http, responses_may_be_read_by_any_page) {
	char head[512];
	const size_t length = vasal_server_http_head(head, sizeof head - 1U,
		vasal_server_http_status_ok, "text/plain", 3U);
	head[length] = '\0';
	cairo_assert_true(NULL != strstr(head,
		"\r\nAccess-Control-Allow-Origin: *\r\n"));
}

// spec: server.web#3
cairo_test_new(server_http, refuses_heads_that_do_not_fit) {
	char head[32];
	cairo_assert_eq(vasal_server_http_head(head, sizeof head,
		vasal_server_http_status_ok, "text/html", 10U), 0U);
}

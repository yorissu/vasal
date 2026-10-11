
/// Reads request heads strictly (RFC 9112's request line and field syntax,
/// without obsolete forms) and writes response heads.

#include "server/http.h"

#include <string.h>

// Whether the data from at on, before end, starts with the text.
static bool_t _vasal_server_http_matches(const char* const data,
		const size_t end, const size_t at, const char* const text) {
	const size_t length = strlen(text);
	if ((at + length) > end) {
		return false;
	}
	for (size_t i = 0U; i < length; i++) {
		if (text[i] != data[at + i]) {
			return false;
		}
	}
	return true;
}

// The status lines, in the order of vasal_server_http_status_e.
static const char* const _vasal_server_http_status_lines[] = {
	"200 OK",
	"400 Bad Request",
	"404 Not Found",
	"405 Method Not Allowed",
	"403 Forbidden",
	"413 Content Too Large",
	"503 Service Unavailable"
};

// The error bodies, in the order of vasal_server_http_status_e.
static const char* const _vasal_server_http_reasons[] = {
	"",
	"bad request\n",
	"not found\n",
	"method not allowed\n",
	"wrong admin key, or uploads are off\n",
	"the file is too large\n",
	"another upload is going on, try again\n"
};

// Finds the empty line ending a head. Returns the head's size including it,
// or 0 when it has not been received.
static size_t _vasal_server_http_head_end(const char* const data,
		const size_t size) {
	for (size_t i = 3U; i < size; i++) {
		if (('\n' == data[i]) && ('\r' == data[i - 1U])
				&& ('\n' == data[i - 2U]) && ('\r' == data[i - 3U])) {
			return i + 1U;
		}
	}
	return 0U;
}

// A token character (RFC 9110): letters, digits and a few symbols.
static bool_t _vasal_server_http_token(const char character) {
	if (((character >= 'a') && (character <= 'z'))
			|| ((character >= 'A') && (character <= 'Z'))
			|| ((character >= '0') && (character <= '9'))) {
		return true;
	}
	return ('\0' != character)
		&& (NULL != strchr("!#$%&'*+-.^_`|~", (int)character));
}

// A visible ASCII character, as allowed in a request target.
static bool_t _vasal_server_http_visible(const char character) {
	return (character > ' ') && (character <= '~');
}

// A character allowed in a field value: tab, visible ASCII, space, and
// bytes above ASCII.
static bool_t _vasal_server_http_field_value(const char character) {
	const uint8_t byte = (uint8_t)character;
	return (0x09U == byte) || ((0x20U <= byte) && (0x7fU != byte));
}

// Whether a field's name, from name to before its colon, is
// Content-Length, in any case.
static bool_t _vasal_server_http_is_length(const char* const data,
		const size_t name, const size_t end) {
	static const char wanted[] = "content-length";
	if ((end - name) != (sizeof(wanted) - 1U)) {
		return false;
	}
	for (size_t i = 0U; i < (sizeof(wanted) - 1U); i++) {
		uint8_t code = (uint8_t)data[name + i];
		if ((code >= (uint8_t)'A') && (code <= (uint8_t)'Z')) {
			code = (uint8_t)(code + 0x20U);
		}
		if (code != (uint8_t)wanted[i]) {
			return false;
		}
	}
	return true;
}

// Reads a Content-Length value: digits only, around optional spaces, at
// most 9 of them. Returns false when it isn't one.
static bool_t _vasal_server_http_length(const char* const data,
		const size_t start, const size_t end, size_t* const length) {
	size_t at = start;
	while ((at < end) && ((' ' == data[at]) || ('\t' == data[at]))) {
		at++;
	}
	size_t value = 0U;
	size_t digits = 0U;
	while ((at < end) && (data[at] >= '0') && (data[at] <= '9')) {
		const uint8_t code = (uint8_t)data[at];
		value = (value * 10U) + ((size_t)code - (size_t)'0');
		digits++;
		at++;
	}
	while ((at < end) && ((' ' == data[at]) || ('\t' == data[at]))) {
		at++;
	}
	*length = value;
	return (0U < digits) && (9U >= digits) && (at == end);
}

// Reads the header fields, from at to the end of the head. Each line is a
// token, a colon and a value. A Content-Length field gives the body's
// length.
static bool_t _vasal_server_http_fields(const char* const data,
		const size_t end, const size_t start, size_t* const content_length) {
	size_t at = start;
	*content_length = 0U;
	while ((at + 2U) <= end) {
		if (('\r' == data[at]) && ('\n' == data[at + 1U])) {
			return (at + 2U) == end;
		}
		const size_t name = at;
		while ((at < end) && _vasal_server_http_token(data[at])) {
			at++;
		}
		if ((name == at) || (at >= end) || (':' != data[at])) {
			return false;
		}
		const bool_t length = _vasal_server_http_is_length(data, name, at);
		at++;
		const size_t value = at;
		while ((at < end) && _vasal_server_http_field_value(data[at])) {
			at++;
		}
		if (((at + 1U) >= end) || ('\r' != data[at])
				|| ('\n' != data[at + 1U])) {
			return false;
		}
		if (length && !_vasal_server_http_length(data, value, at,
				content_length)) {
			return false;
		}
		at += 2U;
	}
	return false;
}

// Reads a request target from at on: visible characters, whose path ends
// at a query or fragment. Gives the path's length, where the query starts
// (0 for none) and its length. Returns where the target ends.
static size_t _vasal_server_http_target(const char* const data,
		const size_t end, const size_t start, size_t* const path_length,
		size_t* const query, size_t* const query_length) {
	size_t at = start;
	while ((at < end) && _vasal_server_http_visible(data[at])
			&& ('?' != data[at]) && ('#' != data[at])) {
		at++;
	}
	*path_length = at - start;
	if ((at < end) && ('?' == data[at])) {
		at++;
		*query = at;
		while ((at < end) && _vasal_server_http_visible(data[at])
				&& ('#' != data[at])) {
			at++;
		}
		*query_length = at - *query;
	}
	while ((at < end) && _vasal_server_http_visible(data[at])) {
		at++;
	}
	return at;
}

// spec: network.handshake#2
vasal_server_http_parse_e vasal_server_http_parse(const char* const data,
	const size_t size, vasal_server_http_request_s* const request) {
	const size_t end = _vasal_server_http_head_end(data, size);
	if (0U == end) {
		return (size >= vasal_server_http_request_capacity)
			? vasal_server_http_parse_malformed
			: vasal_server_http_parse_incomplete;
	}
	if (end > vasal_server_http_request_capacity) {
		return vasal_server_http_parse_malformed;
	}

	// The method: a token followed by one space.
	size_t at = 0U;
	while ((at < end) && _vasal_server_http_token(data[at])) {
		at++;
	}
	const size_t method_length = at;
	if ((0U == method_length) || (16U < method_length) || (' ' != data[at])) {
		return vasal_server_http_parse_malformed;
	}
	at++;

	// The target: visible characters starting with a slash, followed by one
	// space.
	const size_t target = at;
	size_t path_length = 0U;
	size_t query = 0U;
	size_t query_length = 0U;
	at = _vasal_server_http_target(data, end, at, &path_length, &query,
		&query_length);
	if ((target == at) || ('/' != data[target]) || (' ' != data[at])) {
		return vasal_server_http_parse_malformed;
	}
	at++;

	// The version, then the end of the request line.
	if (!_vasal_server_http_matches(data, end, at, "HTTP/1.1\r\n")
			&& !_vasal_server_http_matches(data, end, at, "HTTP/1.0\r\n")) {
		return vasal_server_http_parse_malformed;
	}
	at += 10U;

	size_t content_length = 0U;
	if (!_vasal_server_http_fields(data, end, at, &content_length)) {
		return vasal_server_http_parse_malformed;
	}

	const bool_t get = (3U == method_length) && ('G' == data[0])
		&& ('E' == data[1]) && ('T' == data[2]);
	const bool_t post = (4U == method_length) && ('P' == data[0])
		&& ('O' == data[1]) && ('S' == data[2]) && ('T' == data[3]);
	request->method = vasal_server_http_method_other;
	if (get) {
		request->method = vasal_server_http_method_get;
	} else if (post) {
		request->method = vasal_server_http_method_post;
	} else {
		// Neither.
	}
	request->path = &data[target];
	request->path_length = path_length;
	request->query = (0U == query) ? &data[target + path_length] : &data[query];
	request->query_length = query_length;
	request->content_length = content_length;
	request->head_length = end;
	return vasal_server_http_parse_complete;
}

// Appends text to the buffer. Returns false when it does not fit.
static bool_t _vasal_server_http_append(char* const buffer,
		const size_t capacity, size_t* const used, const char* const text) {
	const size_t length = strlen(text);
	if (length > (capacity - *used)) {
		return false;
	}
	for (size_t i = 0U; i < length; i++) {
		buffer[*used + i] = text[i];
	}
	*used += length;
	return true;
}

// Appends a number in decimal.
static bool_t _vasal_server_http_append_number(char* const buffer,
		const size_t capacity, size_t* const used, const size_t number) {
	char digits[24];
	size_t count = 0U;
	size_t rest = number;
	do {
		digits[count] = "0123456789"[rest % 10U];
		count++;
		rest /= 10U;
	} while (0U != rest);

	char text[24];
	for (size_t i = 0U; i < count; i++) {
		text[i] = digits[count - 1U - i];
	}
	text[count] = '\0';
	return _vasal_server_http_append(buffer, capacity, used, text);
}

// spec: server.web#2 server.web#3
size_t vasal_server_http_head(char* const buffer, const size_t capacity,
	const vasal_server_http_status_e status, const char* const type,
	const size_t length) {
	size_t used = 0U;
	bool_t fits = _vasal_server_http_append(buffer, capacity, &used,
		"HTTP/1.1 ");
	fits = fits && _vasal_server_http_append(buffer, capacity, &used,
		_vasal_server_http_status_lines[status]);
	fits = fits && _vasal_server_http_append(buffer, capacity, &used,
		"\r\nContent-Type: ");
	fits = fits && _vasal_server_http_append(buffer, capacity, &used, type);
	fits = fits && _vasal_server_http_append(buffer, capacity, &used,
		"\r\nContent-Length: ");
	fits = fits && _vasal_server_http_append_number(buffer, capacity, &used,
		length);
	if (vasal_server_http_status_method_not_allowed == status) {
		fits = fits && _vasal_server_http_append(buffer, capacity, &used,
			"\r\nAllow: GET, POST");
	}
	// spec: rules.content#7
	fits = fits && _vasal_server_http_append(buffer, capacity, &used,
		"\r\nCache-Control: no-cache"
		"\r\nX-Content-Type-Options: nosniff"
		"\r\nAccess-Control-Allow-Origin: *"
		"\r\nConnection: close\r\n\r\n");
	return fits ? used : 0U;
}

const char* vasal_server_http_reason(const vasal_server_http_status_e status) {
	return _vasal_server_http_reasons[status];
}


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
	"405 Method Not Allowed"
};

// The error bodies, in the order of vasal_server_http_status_e.
static const char* const _vasal_server_http_reasons[] = {
	"",
	"bad request\n",
	"not found\n",
	"method not allowed\n"
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

// Reads the header fields, from at to the end of the head. Each line is a
// token, a colon and a value.
static bool_t _vasal_server_http_fields(const char* const data,
		const size_t end, const size_t start) {
	size_t at = start;
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
		at++;
		while ((at < end) && _vasal_server_http_field_value(data[at])) {
			at++;
		}
		if (((at + 1U) >= end) || ('\r' != data[at])
				|| ('\n' != data[at + 1U])) {
			return false;
		}
		at += 2U;
	}
	return false;
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
	// space. The path ends at a query or fragment.
	const size_t target = at;
	size_t path_length = 0U;
	bool_t path_ended = false;
	while ((at < end) && _vasal_server_http_visible(data[at])) {
		if (('?' == data[at]) || ('#' == data[at])) {
			path_ended = true;
		}
		if (!path_ended) {
			path_length++;
		}
		at++;
	}
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

	if (!_vasal_server_http_fields(data, end, at)) {
		return vasal_server_http_parse_malformed;
	}

	const bool_t get = (3U == method_length) && ('G' == data[0])
		&& ('E' == data[1]) && ('T' == data[2]);
	request->method = get ? vasal_server_http_method_get
		: vasal_server_http_method_other;
	request->path = &data[target];
	request->path_length = path_length;
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
			"\r\nAllow: GET");
	}
	fits = fits && _vasal_server_http_append(buffer, capacity, &used,
		"\r\nCache-Control: no-cache"
		"\r\nX-Content-Type-Options: nosniff"
		"\r\nConnection: close\r\n\r\n");
	return fits ? used : 0U;
}

const char* vasal_server_http_reason(const vasal_server_http_status_e status) {
	return _vasal_server_http_reasons[status];
}


/// The part of HTTP/1.1 the server needs: reading a request's head (request
/// line and header fields) and writing a response's head. Requests are only
/// read, never trusted: anything unusual is malformed.

#ifndef vasal__server__http_h
#define vasal__server__http_h

#include "engine/types.h"

/// The largest request head the server accepts, in bytes.
#define vasal_server_http_request_capacity 8192U

/// A request method, as far as the server cares.
typedef enum vasal_server_http_method_e {
	vasal_server_http_method_get,
	vasal_server_http_method_other
} vasal_server_http_method_e;

/// A request whose head was read.
typedef struct vasal_server_http_request_s {
	/// Its method.
	vasal_server_http_method_e method;
	/// Its path: the target up to a query or fragment. Points into the data
	/// that was parsed and is not terminated.
	const char*                path;
	/// The path's length in bytes.
	size_t                     path_length;
} vasal_server_http_request_s;

/// How far reading a request got.
typedef enum vasal_server_http_parse_e {
	/// The head has not been received completely yet.
	vasal_server_http_parse_incomplete,
	/// The head was read; the request holds it.
	vasal_server_http_parse_complete,
	/// The head breaks the protocol or is larger than the capacity.
	vasal_server_http_parse_malformed
} vasal_server_http_parse_e;

/// The response statuses the server sends.
typedef enum vasal_server_http_status_e {
	vasal_server_http_status_ok,
	vasal_server_http_status_bad_request,
	vasal_server_http_status_not_found,
	vasal_server_http_status_method_not_allowed
} vasal_server_http_status_e;

/// Reads a request head from the first size bytes received. The request is
/// filled in only when the outcome is complete.
vasal_server_http_parse_e vasal_server_http_parse(const char* const data,
	const size_t size, vasal_server_http_request_s* const request);

/// Writes a response head for a body of length bytes and type into buffer.
/// Returns its length, or 0 when it does not fit into capacity bytes. The
/// connection is always closed after the response.
size_t vasal_server_http_head(char* const buffer, const size_t capacity,
	const vasal_server_http_status_e status, const char* const type,
	const size_t length);

/// The short text body sent with an error status, such as "not found\n".
const char* vasal_server_http_reason(const vasal_server_http_status_e status);

#endif

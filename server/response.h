
/// Decides how the server answers a request: which status, and which web
/// file, if any. It holds no state and does no I/O, so every answer can be
/// tested without a network.

#ifndef vasal__server__response_h
#define vasal__server__response_h

#include "server/files.h"
#include "server/http.h"

/// An answer to a request.
typedef struct vasal_server_response_s {
	/// The status to send.
	vasal_server_http_status_e        status;
	/// The file to send with status ok; NULL for the error statuses, which
	/// send their reason as text.
	const vasal_server_files_file_s* file;
} vasal_server_response_s;

/// Chooses the answer to a request head that was read with the outcome
/// given: complete or malformed, never incomplete.
vasal_server_response_s vasal_server_response_choose(
	const vasal_server_http_parse_e outcome,
	const vasal_server_http_request_s* const request);

#endif

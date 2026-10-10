
/// Maps a request to bad request, method not allowed, not found, or one of
/// the web files.

#include "server/response.h"

// spec: server.web#1 server.web#2
vasal_server_response_s vasal_server_response_choose(
	const vasal_server_http_parse_e outcome,
	const vasal_server_http_request_s* const request) {
	vasal_server_response_s response = {
		.status = vasal_server_http_status_bad_request,
		.file   = NULL
	};
	if (vasal_server_http_parse_complete != outcome) {
		return response;
	}
	if (vasal_server_http_method_get != request->method) {
		response.status = vasal_server_http_status_method_not_allowed;
		return response;
	}

	const vasal_server_files_file_s* const file =
		vasal_server_files_find(request->path, request->path_length);
	if (NULL == file) {
		response.status = vasal_server_http_status_not_found;
	} else {
		response.status = vasal_server_http_status_ok;
		response.file = file;
	}
	return response;
}

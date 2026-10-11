
/// The match server: reads its settings, loads the web files, and runs the
/// network loop until it is stopped.

#include "server/files.h"
#include "server/log.h"
#include "server/network.h"
#include "server/options.h"

// spec: server.process#1 server.process#2
int main(const int count, char* arguments[]) {
	vasal_server_options_s options;
	const char* error = NULL;
	const vasal_server_options_parse_e parse =
		vasal_server_options_parse(count, arguments, &options, &error);
	if (vasal_server_options_parse_help == parse) {
		vasal_server_log_text(vasal_server_options_usage);
		return 0;
	}
	if (vasal_server_options_parse_invalid == parse) {
		vasal_server_log_write(vasal_server_log_level_error, error, NULL);
		vasal_server_log_text(vasal_server_options_usage);
		return 2;
	}

	if (!vasal_server_files_load(options.web, options.map, options.rules)) {
		return 1;
	}
	return vasal_server_network_run(&options) ? 0 : 1;
}

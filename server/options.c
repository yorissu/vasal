
/// Reads the server's command line: --address, --port, --web, --map,
/// --rules, --admin-key and --help.

#include "server/options.h"

#include <arpa/inet.h>
#include <netinet/in.h>

#include <string.h>

const char* const vasal_server_options_usage =
	"usage: vasal_server --web <folder> [--map <file>] [--rules <file>]\n"
	"                    [--admin-key <key>] [--address <ipv4>]\n"
	"                    [--port <port>]\n"
	"  --web        folder holding the client's files\n"
	"  --map        the match's map file, served to the clients\n"
	"  --rules      the game's rules file, served to the clients (the\n"
	"               standard rules until there is one)\n"
	"  --admin-key  the key the editor gives to upload maps and rules,\n"
	"               which replace the files above; no uploads without it\n"
	"  --address    address to listen on (default 127.0.0.1)\n"
	"  --port       port to listen on (default 8080)\n"
	"  --help       print this text\n";

// Reads a port number: 1 to 65535, digits only.
static bool_t _vasal_server_options_port(const char* const text,
		uint16_t* const port) {
	uint32_t value = 0U;
	size_t length = 0U;
	while ('\0' != text[length]) {
		const char digit = text[length];
		if ((digit < '0') || (digit > '9') || (5U <= length)) {
			return false;
		}
		const uint8_t code = (uint8_t)digit;
		value = (value * 10U) + ((uint32_t)code - (uint32_t)'0');
		length++;
	}
	if ((0U == length) || (0U == value) || (65535U < value)) {
		return false;
	}
	*port = (uint16_t)value;
	return true;
}

vasal_server_options_parse_e vasal_server_options_parse(const int count,
	char* const* const arguments, vasal_server_options_s* const options,
	const char** const error) {
	options->address = "127.0.0.1";
	options->port = 8080U;
	options->web = NULL;
	options->map = NULL;
	options->rules = NULL;
	options->admin_key = NULL;
	*error = NULL;

	int index = 1;
	while (index < count) {
		const char* const name = arguments[index];
		if (0 == strcmp(name, "--help")) {
			return vasal_server_options_parse_help;
		}

		const bool_t known = (0 == strcmp(name, "--address"))
			|| (0 == strcmp(name, "--port")) || (0 == strcmp(name, "--web"))
			|| (0 == strcmp(name, "--map")) || (0 == strcmp(name, "--rules"))
			|| (0 == strcmp(name, "--admin-key"));
		if (!known) {
			*error = "unknown option";
			return vasal_server_options_parse_invalid;
		}
		if ((index + 1) >= count) {
			*error = "an option is missing its value";
			return vasal_server_options_parse_invalid;
		}
		const char* const value = arguments[index + 1];

		if (0 == strcmp(name, "--address")) {
			struct in_addr address;
			if (1 != inet_pton(AF_INET, value, &address)) {
				*error = "--address is not an IPv4 address";
				return vasal_server_options_parse_invalid;
			}
			options->address = value;
		} else if (0 == strcmp(name, "--port")) {
			if (!_vasal_server_options_port(value, &options->port)) {
				*error = "--port is not a number from 1 to 65535";
				return vasal_server_options_parse_invalid;
			}
		} else if (0 == strcmp(name, "--map")) {
			options->map = value;
		} else if (0 == strcmp(name, "--rules")) {
			options->rules = value;
		} else if (0 == strcmp(name, "--admin-key")) {
			options->admin_key = value;
		} else {
			options->web = value;
		}
		index += 2;
	}

	if (NULL == options->web) {
		*error = "--web is required";
		return vasal_server_options_parse_invalid;
	}
	return vasal_server_options_parse_run;
}

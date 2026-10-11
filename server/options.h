
/// The server's settings, read from its command line.

#ifndef vasal__server__options_h
#define vasal__server__options_h

#include "engine/types.h"

/// The settings. The strings point into the command line.
typedef struct vasal_server_options_s {
	/// The IPv4 address to listen on, in dotted form.
	const char* address;
	/// The TCP port to listen on.
	uint16_t    port;
	/// The folder holding the client's files.
	const char* web;
	/// The match's map file and rules file, or NULL for none; uploads
	/// replace them.
	const char* map;
	const char* rules;
	/// The key uploads must give, or NULL: uploads are off then.
	const char* admin_key;
} vasal_server_options_s;

/// What the command line asks for.
typedef enum vasal_server_options_parse_e {
	/// Run the server with the settings read.
	vasal_server_options_parse_run,
	/// Print the usage text and stop.
	vasal_server_options_parse_help,
	/// The command line is wrong; the error says why.
	vasal_server_options_parse_invalid
} vasal_server_options_parse_e;

/// The usage text, for --help and for invalid command lines.
extern const char* const vasal_server_options_usage;

/// Reads the command line (count and arguments as main gets them) into
/// options, starting from the defaults: address 127.0.0.1, port 8080, no
/// map, rules or admin key, and no web folder, which is required. When the
/// command line is invalid, error points at a sentence saying what is
/// wrong.
vasal_server_options_parse_e vasal_server_options_parse(const int count,
	char* const* const arguments, vasal_server_options_s* const options,
	const char** const error);

#endif

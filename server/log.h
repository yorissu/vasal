
/// The server's log: one line per event on standard output, so it ends up
/// wherever the process's output goes (a terminal, Docker's log, a file).
/// This is the only server module that uses stdio.

#ifndef vasal__server__log_h
#define vasal__server__log_h

#include "engine/types.h"

/// How serious a log line is.
typedef enum vasal_server_log_level_e {
	vasal_server_log_level_information,
	vasal_server_log_level_warning,
	vasal_server_log_level_error
} vasal_server_log_level_e;

/// Writes "<level>: <message>", followed by ": <detail>" when detail is not
/// NULL.
void vasal_server_log_write(const vasal_server_log_level_e level,
	const char* const message, const char* const detail);

/// Writes "<level>: <message>: <number>".
void vasal_server_log_number(const vasal_server_log_level_e level,
	const char* const message, const uint64_t number);

/// Writes text as it is, without a level, such as the usage text.
void vasal_server_log_text(const char* const text);

#endif

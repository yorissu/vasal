
/// Formats log lines and writes them to standard output, flushing each one,
/// since standard output is fully buffered when it is not a terminal.

#include "server/log.h"

#include <inttypes.h>
#include <stdio.h>

static const char* const _vasal_server_log_levels[] = {
	"information",
	"warning",
	"error"
};

static const char* _vasal_server_log_level(
		const vasal_server_log_level_e level) {
	const size_t index = (size_t)level;
	const size_t count = sizeof(_vasal_server_log_levels)
		/ sizeof(_vasal_server_log_levels[0]);
	if (count <= index) {
		return "unknown";
	}
	return _vasal_server_log_levels[index];
}

void vasal_server_log_write(const vasal_server_log_level_e level,
	const char* const message, const char* const detail) {
	if (NULL == detail) {
		(void)printf("%s: %s\n", _vasal_server_log_level(level), message);
	} else {
		(void)printf("%s: %s: %s\n", _vasal_server_log_level(level), message,
			detail);
	}
	(void)fflush(stdout);
}

void vasal_server_log_number(const vasal_server_log_level_e level,
	const char* const message, const uint64_t number) {
	(void)printf("%s: %s: %" PRIu64 "\n", _vasal_server_log_level(level),
		message, number);
	(void)fflush(stdout);
}

void vasal_server_log_text(const char* const text) {
	(void)fputs(text, stdout);
	(void)fflush(stdout);
}


/// Stop signals through Linux's signalfd.

#include "server/signals.h"

#include <sys/signalfd.h>
#include <unistd.h>

#include <signal.h>

// spec: server.process#2
int vasal_server_signals_open(void) {
	sigset_t stops;
	if ((0 != sigemptyset(&stops)) || (0 != sigaddset(&stops, SIGINT))
			|| (0 != sigaddset(&stops, SIGTERM))
			|| (0 != sigprocmask(SIG_BLOCK, &stops, NULL))) {
		return -1;
	}
	if (SIG_ERR == signal(SIGPIPE, SIG_IGN)) {
		return -1;
	}
	return signalfd(-1, &stops, SFD_NONBLOCK | SFD_CLOEXEC);
}

bool_t vasal_server_signals_read(const int descriptor) {
	struct signalfd_siginfo information;
	const ssize_t count = read(descriptor, &information,
		sizeof(information));
	return (ssize_t)sizeof(information) == count;
}

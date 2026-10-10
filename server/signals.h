
/// Turns the stop signals (SIGINT, SIGTERM) into a file descriptor that the
/// network loop waits on like any socket, so a stop request is handled in
/// the loop rather than in an asynchronous signal handler. This is the only
/// server module that uses signal.h.

#ifndef vasal__server__signals_h
#define vasal__server__signals_h

#include "engine/types.h"

/// Blocks SIGINT and SIGTERM for the process and returns a descriptor that
/// becomes readable when one arrives, or -1 on failure. SIGPIPE is ignored,
/// so writing to a closed connection fails instead of ending the process.
int vasal_server_signals_open(void);

/// Consumes the pending signal on the descriptor. Returns true when a stop
/// signal was read.
bool_t vasal_server_signals_read(const int descriptor);

#endif

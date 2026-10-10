
/// The server's network loop: one thread, non-blocking TCP sockets and
/// epoll. For now it serves the web files over HTTP; the game's WebSocket
/// connections join it in milestone 3.

#ifndef vasal__server__network_h
#define vasal__server__network_h

#include "server/options.h"

/// The most connections open at once; more are closed as they arrive.
#define vasal_server_network_connections 256U

/// How long a connection may take to send its request, and how long a
/// response may stall, in milliseconds.
#define vasal_server_network_timeout_ms 10000

/// Listens on the address and port in the options and serves requests until
/// SIGINT or SIGTERM arrives. Returns false, after logging why, when the
/// loop could not start or failed.
bool_t vasal_server_network_run(const vasal_server_options_s* const options);

#endif

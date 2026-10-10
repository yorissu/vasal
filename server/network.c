
/// The epoll loop. Every descriptor is registered with an id in its event
/// data: the index of its connection, or one of two ids for the listening
/// socket and the signal descriptor. A connection reads one request head,
/// sends one response and closes; requests never stay open longer than
/// vasal_server_network_timeout_ms without progress.

#include "server/network.h"
#include "server/http.h"
#include "server/log.h"
#include "server/response.h"
#include "server/signals.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>
#include <time.h>

#define _vasal_server_network_events 64
#define _vasal_server_network_backlog 64
#define _vasal_server_network_head_capacity 512U
#define _vasal_server_network_listener_id 0xffffffffU
#define _vasal_server_network_signals_id 0xfffffffeU

// One connection. A free one has socket -1.
typedef struct _vasal_server_network_connection_s {
	// Bytes of the request head received so far.
	size_t         received;
	size_t         head_size;
	const uint8_t* body;
	size_t         body_size;
	// Bytes of head and body sent so far.
	size_t         sent;
	// When the connection is closed unless it makes progress.
	int64_t        deadline;
	int            socket;
	// Whether the request was read and the response is being sent.
	bool_t         responding;
	char           head[_vasal_server_network_head_capacity];
	// The request head received so far.
	char           request[vasal_server_http_request_capacity];
} _vasal_server_network_connection_s;

static _vasal_server_network_connection_s
	_vasal_server_network_connections[vasal_server_network_connections];
static int _vasal_server_network_epoll = -1;

// Milliseconds on the monotonic clock.
static int64_t _vasal_server_network_now(void) {
	struct timespec now = { 0 };
	(void)clock_gettime(CLOCK_MONOTONIC, &now);
	return ((int64_t)now.tv_sec * 1000) + ((int64_t)now.tv_nsec / 1000000);
}

static bool_t _vasal_server_network_nonblocking(const int descriptor) {
	const int flags = fcntl(descriptor, F_GETFL, 0);
	return (0 <= flags)
		&& (0 == fcntl(descriptor, F_SETFL, flags | O_NONBLOCK));
}

static bool_t _vasal_server_network_watch(const int descriptor,
		const int operation, const uint32_t events, const uint32_t id) {
	struct epoll_event event = { 0 };
	event.events = events;
	event.data.u32 = id;
	return 0 == epoll_ctl(_vasal_server_network_epoll, operation, descriptor,
		&event);
}

static void _vasal_server_network_close(
		_vasal_server_network_connection_s* const connection) {
	(void)epoll_ctl(_vasal_server_network_epoll, EPOLL_CTL_DEL,
		connection->socket, NULL);
	(void)close(connection->socket);
	connection->socket = -1;
}

// Opens the listening socket. Returns -1 on failure.
static int _vasal_server_network_listen(
		const vasal_server_options_s* const options) {
	const int listener = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (0 > listener) {
		return -1;
	}

	const int reuse = 1;
	struct sockaddr_in address = { 0 };
	address.sin_family = AF_INET;
	address.sin_port = htons(options->port);
	const bool_t listening =
		(0 == setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse,
			sizeof(reuse)))
		&& (1 == inet_pton(AF_INET, options->address, &address.sin_addr))
		&& (0 == bind(listener, (const struct sockaddr*)&address,
			sizeof(address)))
		&& (0 == listen(listener, _vasal_server_network_backlog))
		&& _vasal_server_network_nonblocking(listener);
	if (!listening) {
		(void)close(listener);
		return -1;
	}
	return listener;
}

// Takes every pending connection into a free slot, or closes it when there
// is none.
static void _vasal_server_network_accept(const int listener) {
	bool_t pending = true;
	while (pending) {
		const int socket = accept(listener, NULL, NULL);
		if (0 > socket) {
			pending = (EINTR == errno);
			continue;
		}

		_vasal_server_network_connection_s* connection = NULL;
		uint32_t id = 0U;
		while ((NULL == connection)
				&& (id < vasal_server_network_connections)) {
			if (0 > _vasal_server_network_connections[id].socket) {
				connection = &_vasal_server_network_connections[id];
			} else {
				id++;
			}
		}

		if ((NULL == connection) || !_vasal_server_network_nonblocking(socket)
				|| !_vasal_server_network_watch(socket, EPOLL_CTL_ADD,
					EPOLLIN | EPOLLRDHUP, id)) {
			vasal_server_log_write(vasal_server_log_level_warning,
				"connection refused", (NULL == connection)
					? "too many connections" : "can't watch it");
			(void)close(socket);
			continue;
		}

		connection->socket = socket;
		connection->received = 0U;
		connection->responding = false;
		connection->sent = 0U;
		connection->deadline = _vasal_server_network_now()
			+ vasal_server_network_timeout_ms;
	}
}

// Sends as much of the response as the socket takes. Closes the connection
// when all is sent or sending fails.
static void _vasal_server_network_write(
		_vasal_server_network_connection_s* const connection) {
	const size_t total = connection->head_size + connection->body_size;
	while (connection->sent < total) {
		ssize_t count = 0;
		if (connection->sent < connection->head_size) {
			count = send(connection->socket,
				&connection->head[connection->sent],
				connection->head_size - connection->sent, MSG_NOSIGNAL);
		} else {
			const size_t offset = connection->sent - connection->head_size;
			count = send(connection->socket, &connection->body[offset],
				connection->body_size - offset, MSG_NOSIGNAL);
		}

		if (0 < count) {
			connection->sent += (size_t)count;
			connection->deadline = _vasal_server_network_now()
				+ vasal_server_network_timeout_ms;
		} else if ((0 > count) && (EAGAIN == errno)) {
			return;
		} else if ((0 > count) && (EINTR == errno)) {
			// Try again.
		} else {
			_vasal_server_network_close(connection);
			return;
		}
	}
	(void)shutdown(connection->socket, SHUT_WR);
	_vasal_server_network_close(connection);
}

// Logs a request: its status and path.
static void _vasal_server_network_log(
		const vasal_server_http_parse_e outcome,
		const vasal_server_http_request_s* const request,
		const vasal_server_response_s* const response) {
	char path[128];
	size_t length = 0U;
	if (vasal_server_http_parse_complete == outcome) {
		length = request->path_length;
		if (length >= sizeof(path)) {
			length = sizeof(path) - 1U;
		}
		for (size_t i = 0U; i < length; i++) {
			path[i] = request->path[i];
		}
	}
	path[length] = '\0';

	if (vasal_server_http_status_ok == response->status) {
		vasal_server_log_write(vasal_server_log_level_information, "served",
			path);
	} else {
		vasal_server_log_write(vasal_server_log_level_warning,
			"refused", (0U == length) ? "malformed request" : path);
	}
}

// Prepares the response to a request head that was read, and starts
// sending it.
static void _vasal_server_network_respond(
		_vasal_server_network_connection_s* const connection,
		const vasal_server_http_parse_e outcome,
		const vasal_server_http_request_s* const request,
		const uint32_t id) {
	const vasal_server_response_s response =
		vasal_server_response_choose(outcome, request);
	_vasal_server_network_log(outcome, request, &response);

	const char* type = "text/plain; charset=utf-8";
	if (NULL == response.file) {
		const char* const reason = vasal_server_http_reason(response.status);
		connection->body = (const uint8_t*)reason;
		connection->body_size = strlen(reason);
	} else {
		type = response.file->type;
		connection->body = response.file->data;
		connection->body_size = response.file->size;
	}

	connection->head_size = vasal_server_http_head(connection->head,
		sizeof(connection->head), response.status, type,
		connection->body_size);
	connection->responding = true;
	connection->sent = 0U;
	connection->deadline = _vasal_server_network_now()
		+ vasal_server_network_timeout_ms;
	if ((0U == connection->head_size)
			|| !_vasal_server_network_watch(connection->socket, EPOLL_CTL_MOD,
				EPOLLOUT, id)) {
		_vasal_server_network_close(connection);
		return;
	}
	_vasal_server_network_write(connection);
}

// Receives what the socket has and responds once the head is complete.
static void _vasal_server_network_read(
		_vasal_server_network_connection_s* const connection,
		const uint32_t id) {
	bool_t more = true;
	while (more
			&& (connection->received < vasal_server_http_request_capacity)) {
		const ssize_t count = recv(connection->socket,
			&connection->request[connection->received],
			vasal_server_http_request_capacity - connection->received, 0);
		if (0 < count) {
			connection->received += (size_t)count;
		} else if ((0 > count) && (EINTR == errno)) {
			// Try again.
		} else if ((0 > count) && (EAGAIN == errno)) {
			more = false;
		} else {
			// The peer closed the connection, or it failed.
			_vasal_server_network_close(connection);
			return;
		}
	}

	vasal_server_http_request_s request = { 0 };
	const vasal_server_http_parse_e outcome = vasal_server_http_parse(
		connection->request, connection->received, &request);
	if (vasal_server_http_parse_incomplete != outcome) {
		_vasal_server_network_respond(connection, outcome, &request, id);
	}
}

static void _vasal_server_network_event(const uint32_t id,
		const uint32_t events) {
	_vasal_server_network_connection_s* const connection =
		&_vasal_server_network_connections[id];
	if (0 > connection->socket) {
		return;
	}
	if (0U != (events & (uint32_t)(EPOLLERR | EPOLLHUP))) {
		_vasal_server_network_close(connection);
	} else if (!connection->responding) {
		if (0U != (events & (uint32_t)(EPOLLIN | EPOLLRDHUP))) {
			_vasal_server_network_read(connection, id);
		}
	} else if (0U != (events & (uint32_t)EPOLLOUT)) {
		_vasal_server_network_write(connection);
	} else {
		// Nothing to do yet.
	}
}

// Closes connections that made no progress in time.
static void _vasal_server_network_expire(void) {
	const int64_t now = _vasal_server_network_now();
	for (size_t i = 0U; i < vasal_server_network_connections; i++) {
		_vasal_server_network_connection_s* const connection =
			&_vasal_server_network_connections[i];
		if ((0 <= connection->socket) && (connection->deadline < now)) {
			_vasal_server_network_close(connection);
		}
	}
}

// spec: server.web server.process#2
bool_t vasal_server_network_run(const vasal_server_options_s* const options) {
	for (size_t i = 0U; i < vasal_server_network_connections; i++) {
		_vasal_server_network_connections[i].socket = -1;
	}

	const int listener = _vasal_server_network_listen(options);
	if (0 > listener) {
		vasal_server_log_write(vasal_server_log_level_error,
			"can't listen on the address and port", options->address);
		return false;
	}
	_vasal_server_network_epoll = epoll_create1(EPOLL_CLOEXEC);
	const int signals = vasal_server_signals_open();
	if ((0 > _vasal_server_network_epoll) || (0 > signals)
			|| !_vasal_server_network_watch(listener, EPOLL_CTL_ADD, EPOLLIN,
				_vasal_server_network_listener_id)
			|| !_vasal_server_network_watch(signals, EPOLL_CTL_ADD, EPOLLIN,
				_vasal_server_network_signals_id)) {
		vasal_server_log_write(vasal_server_log_level_error,
			"can't start the network loop", NULL);
		return false;
	}
	vasal_server_log_write(vasal_server_log_level_information,
		"listening on", options->address);
	vasal_server_log_number(vasal_server_log_level_information,
		"port", options->port);

	bool_t running = true;
	bool_t failed = false;
	while (running) {
		struct epoll_event events[_vasal_server_network_events];
		const int count = epoll_wait(_vasal_server_network_epoll, events,
			_vasal_server_network_events, 1000);
		if ((0 > count) && (EINTR != errno)) {
			vasal_server_log_write(vasal_server_log_level_error,
				"waiting for events failed", NULL);
			failed = true;
			running = false;
		}
		for (int i = 0; i < count; i++) {
			const uint32_t id = events[i].data.u32;
			if (_vasal_server_network_signals_id == id) {
				if (vasal_server_signals_read(signals)) {
					vasal_server_log_write(vasal_server_log_level_information,
						"stopping", NULL);
					running = false;
				}
			} else if (_vasal_server_network_listener_id == id) {
				_vasal_server_network_accept(listener);
			} else {
				_vasal_server_network_event(id, events[i].events);
			}
		}
		_vasal_server_network_expire();
	}

	for (size_t i = 0U; i < vasal_server_network_connections; i++) {
		if (0 <= _vasal_server_network_connections[i].socket) {
			_vasal_server_network_close(&_vasal_server_network_connections[i]);
		}
	}
	(void)close(listener);
	(void)close(signals);
	(void)close(_vasal_server_network_epoll);
	return !failed;
}

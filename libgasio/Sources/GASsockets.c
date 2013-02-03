
#include <string.h>		// memset, strerror
#include <errno.h>		// errno, EAGAIN
#include <unistd.h>		// close
#include <fcntl.h>		// fcntl
#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <netdb.h>		// hostent
#endif

#include "GASdefinitions.h"
#include "GASsupport.h"
#include "GASclient.h"
#include "GASsockets.h"
#include "GASthreads.h"
#include "GASepoll.h"
#include "GASkqueue.h"
#include "GASiocp.h"


////////////////  SOCKETS  ////////////////


int
gas_init_sockets (void)
{
#ifdef	WIN32
	// init the winsock libraries
	WSADATA wsaData;
	if ( WSAStartup(MAKEWORD(2,2), &wsaData) != 0 )
		return gas_error_message ("Unable to initialize WinSock 1.1\n");
#endif	// WIN32
	return 0;
}


gas_socket_t
gas_create_server_socket (char* address, int port) {
	gas_socket_t server_socket;
	if ((server_socket = gas_create_socket()) == INVALID_SOCKET)
		return -1;
	if (gas_bind_socket (server_socket, address, port) < 0) {
		server_socket = gas_close_socket (server_socket);
		return -1;
	}
	if (gas_start_listening (server_socket) < 0) {
		server_socket = gas_close_socket (server_socket);
		return -1;
	}
	return server_socket;
}

gas_socket_t
gas_create_socket ()
{
	gas_socket_t a_socket;
#ifdef WIN32
	char __attribute__((unused)) yes=1;
#else  // WIN32
	int yes=1;
#endif // WIN32

	if ((a_socket = socket(PF_INET, SOCK_STREAM, 0/*IPPROTO_TCP*/)) == INVALID_SOCKET)
		return gas_error_message ("Error creating socket: %s\n", strerror (errno));

	// reuse local address. not required for iocp?
	if (setsockopt(a_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0)
		gas_error_message ("Error setting SO_REUSEADDR for socket: %s\n", strerror (errno));
	return a_socket;
}

gas_socket_t
gas_close_socket (gas_socket_t a_socket)
{
	if (a_socket >= 0)
#ifdef WIN32
		closesocket (a_socket);
#else  // WIN32
		close (a_socket);
#endif // WIN32
	return -1;
}

void
gas_block_socket (gas_socket_t a_socket, int block) {
#ifndef WIN32
	int flag = fcntl(a_socket, F_GETFL, 0);
	if (block)
		fcntl(a_socket, F_SETFL, flag & ~O_NONBLOCK);
	else
		fcntl(a_socket, F_SETFL, flag | O_NONBLOCK);
#endif // WIN32
}

int
gas_bind_socket (gas_socket_t server_socket, char* address, int port)
{
	struct sockaddr_in sin;

	memset(&sin, 0, sizeof sin);
	sin.sin_family = AF_INET;
	if (address==NULL || *address=='\0')
		sin.sin_addr.s_addr = htonl(INADDR_ANY);
	else
		sin.sin_addr.s_addr = inet_addr(address);
	sin.sin_port = htons(port);

	if (bind(server_socket, (struct sockaddr *) &sin, sizeof sin) == SOCKET_ERROR)
		return gas_error_message ("Error binding socket: %s\n", strerror (errno));
	return 0;
}

int
gas_start_listening (gas_socket_t server_socket)
{
	if (listen(server_socket, GAS_LISTEN_QUEUE_SIZE) < 0)
		return gas_error_message ("Error listening to socket: %s\n", strerror (errno));
	return 0;
}


gas_client_info *
gas_do_accept (void *tpi, gas_socket_t server_socket)
{
	gas_debug_message (GAS_IO, "gas_do_accept\n");
	GAS_THREADS_INFO *ti = (GAS_THREADS_INFO *) tpi;
	struct sockaddr client_addr;
	gas_socklen_t client_addr_len = sizeof client_addr;

	// accept connection
	gas_socket_t client_socket = accept (server_socket, &client_addr, &client_addr_len);
	if (client_socket < 0) {
		if (!ti->stop)
			gas_debug_message (GAS_IO, "Error accepting connection: %s\n", strerror (errno));
		return NULL;
	}

	if (ti->networks_info != NULL) {
		char address_string[30];
		if (!gas_validate_network (tpi, &client_addr, address_string)) {
			gas_error_message ("Forbidden user: %s\n", address_string);
			client_socket = gas_close_socket ( client_socket );
			return NULL;
		}
	}

#ifndef WIN32
	// set non blocking
	if (ti->clients_are_non_blocking)
		gas_block_socket (client_socket, GAS_FALSE);
#endif // WIN32
	// allocate client
	gas_client_info *ci = gas_create_client(tpi, client_socket);

	// keep adress and port port for reference (debug printing)
	struct sockaddr_in *client_addr_in;
	client_addr_in = (struct sockaddr_in *) &client_addr;
	memcpy (&ci->remote_addr, &client_addr_in->sin_addr, sizeof(struct in_addr));	// keep client address for reference
	ci->remote_port = ntohs (client_addr_in->sin_port);								// and remote port
	getsockname (client_socket, &client_addr, &client_addr_len);
	memcpy (&ci->local_addr, &client_addr_in->sin_addr, sizeof(struct in_addr));	// keep server address for reference
	ci->local_port = ntohs (client_addr_in->sin_port);								// and local port

	gas_do_callback (ci, GAS_CLIENT_CREATE);

	gas_debug_message (GAS_IO, "accept:: socket=%d, ci=%6x\n", client_socket, ci);
#ifndef WIN32
	if (ti->clients_are_non_blocking)
		gas_add_client_to_poll (ci);
#endif
	return ci;
}

int
gas_do_read (gas_client_info *ci)
{
	gas_debug_message (GAS_IO, "gas_do_read\n");
	GAS_THREADS_INFO *ti = (GAS_THREADS_INFO *)ci->tpi;
	int size;

	gas_trim_buffer (ci->rb);
	GAS_CI_MARK_END (ci->rb);

	do {
		size = recv (ci->socket, GAS_CI_BUFFER_END(ci->rb), GAS_CI_BUFFER_SPACE(ci->rb), 0);
		gas_debug_message (GAS_IO, "recv::<< socket=%d, ci=%6x, n=%d, errno=%d\n", ci->socket, ci, size, errno);
		if (size < 0) {
			if (errno == EAGAIN) {
				gas_debug_message (GAS_IO, "recv: EAGAIN\n");
				break;
			}
			if (!((GAS_THREADS_INFO *)ci->tpi)->stop)
				gas_error_message("recv: %s\n", strerror (errno));
			ci->error = GAS_TRUE;
			return 0;
		}
		else if (size == 0) {
			ci->read_end = GAS_TRUE;
			return 0;
		}
		GAS_CI_GROW_BUFFER (ci->rb, size);
		*GAS_CI_BUFFER_END(ci->rb) = '\0';		// recv don't terminate buffer with '\0'
		gas_realloc_buffer (ci->rb);
		gas_debug_message (GAS_DETAIL, "recv::<< \"%s\"\n", GAS_CI_BUFFER_MARK(ci->rb));
	} while (ti->clients_are_non_blocking);

	gas_adjust_stats();

	return GAS_CI_DATA_SIZE(ci->rb);
}

int
gas_do_write (gas_client_info *ci)
{
	gas_debug_message (GAS_IO, "gas_do_write. can_write==%d\n", ci->can_write);
	if (!ci->can_write && !ci->write_pending) {
		errno = EAGAIN;
		return -1;
	}

	ci->callback_has_written = GAS_TRUE;
	int written = 0;
	gas_client_buffer *cb = ci->use_eb? ci->eb: ci->wb;
	GAS_CI_MARK_START (cb);

	while (GAS_CI_DATA_SIZE(cb) > 0) {
#ifndef WIN32
		GAS_THREADS_INFO *ti = (GAS_THREADS_INFO *)ci->tpi;
		if (ti->clients_are_non_blocking && !ci->use_write_events)
			gas_block_socket (ci->socket, GAS_TRUE);				// tweak when output don't use events
#endif // WIN32
		int size = send (ci->socket, GAS_CI_BUFFER_START(cb), GAS_CI_DATA_SIZE(cb), 0);
#ifndef WIN32
		if (ti->clients_are_non_blocking && !ci->use_write_events)
			gas_block_socket (ci->socket, GAS_FALSE);				// tweak when output don't use events
#endif // WIN32
		gas_debug_message (GAS_IO, "send::>> socket=%d, ci=%6x, n=%d, errno=%d\n", ci->socket, ci, size, errno);
		if (size < 0) {
			if (errno == EAGAIN) {
				gas_error_message("send: EAGAIN\n");
				if (ci->use_write_events) {
					ci->can_write = GAS_FALSE;
					ci->write_pending = GAS_TRUE;
				}
				break;
			}
			if (!((GAS_THREADS_INFO *)ci->tpi)->stop)
				gas_error_message("send: %s\n", strerror (errno));
			ci->error = GAS_TRUE;
			return written;
		}
		GAS_CI_WANE_BUFFER (cb, size);
		written += size;
		gas_debug_message (GAS_DETAIL, "send::>> \"%s\"\n", GAS_CI_BUFFER_MARK(cb));
		if (GAS_CI_DATA_SIZE(cb) > 0)
			gas_debug_message (GAS_IO, "send: written=%d: head=%d, mark=%d, tail=%d\n",
					written, cb->head, cb->mark, cb->tail);
	}
	if (gas_trim_buffer(cb) == 0)
		ci->use_eb = GAS_FALSE;
	if (GAS_CI_DATA_SIZE(cb) == 0)
		ci->write_pending = GAS_FALSE;
	return written;
}

int
gas_query_write (gas_client_info *ci)
{
	gas_debug_message (GAS_IO, "gas_query_write. can_write==%d\n", ci->can_write);
	if (ci->can_write) {
#ifdef WIN32
		if (ci->overlapped != NULL)
			return gas_do_write_iocp (ci);
		else
#endif
			return gas_do_write (ci);
	}
	errno = EAGAIN;
	return -1;
}

void
gas_do_callback (gas_client_info *ci, int op)
{
	GAS_THREADS_INFO *ti = (GAS_THREADS_INFO *) ci->tpi;
	if (ci->step < 0)
		ci->step = 0;
	if (ti->callback) {
		ci->callback_has_written = GAS_FALSE;
		(*ti->callback) (ci, op);
	}
	if (ci->callback_has_written)
		GAS_CI_CLEAR_BUFFER (ci->rb);
}

void
gas_get_hostname( char *hostname, int namesize )
{
	struct in_addr in;
	struct hostent *h;

	gethostname(hostname,namesize);
	h = gethostbyname(hostname);
	memcpy(&in,h->h_addr,4);
	h = gethostbyaddr((char *)&in,4,PF_INET);
	if (h!=NULL)
		strcpy(hostname,h->h_name);
}

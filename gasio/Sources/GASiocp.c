
#ifdef WIN32


// iocp_echo_server.c
// -----------------------------------------------------------------------------

#include <stdio.h>
#include <winsock2.h> // needed by "mswsock.h"
#include <mswsock.h>  // for AcceptEx
#include <windows.h>
#include <errno.h>

#include "GASsupport.h"
#include "GASsockets.h"
#include "GASthreads.h"
#include "GASiocp.h"


// agnostic

GAS_POLLER_INFO *
gas_create_poller (void* parent, char* address, int port, char *networks, void (*callback)(gas_client_info *ci,int op))
{
	gas_socket_t server_socket;
	if ((server_socket = gas_create_server_socket (address, port)) < 0)
		return NULL;

	GAS_POLLER_INFO *pi = calloc (1, sizeof(GAS_POLLER_INFO));
	pi->parent = parent;
	pi->info_type = GAS_TP_POLLER;
	pi->allow_tasks = GAS_FALSE;
	pi->clients_are_non_blocking = GAS_TRUE;
	gas_preset_client_config (pi);
	pi->server_socket = server_socket;
	pi->actual_connections = 0;
	gas_assign_networks ((void *)pi, networks);
	pi->callback = callback;
	pi->stop = GAS_FALSE;
	gas_init_clients (pi);

	return pi;
}

int
gas_start_poller (GAS_POLLER_INFO *pi)
{
	pi->accept_ci = gas_create_client (pi, 0);
	pi->accept_ci->operation = GAS_OP_ACCEPT;
	pi->accept_ci->overlapped = calloc(1, sizeof(WSAOVERLAPPED));

	// create io completion port
	pi->cpl_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (!pi->cpl_port) {
		gas_error_message ("Error %d in create_io_completion_port()\n", WSAGetLastError());
		return GAS_FALSE;
	}

	// associate socket with io completion port
	if (CreateIoCompletionPort((HANDLE)pi->server_socket, pi->cpl_port,
			(ULONG_PTR)pi->accept_ci, 0) != pi->cpl_port) {
		CloseHandle (pi->cpl_port);
		gas_error_message ("Error %d in create_listening_socket()\n", WSAGetLastError());
		return GAS_FALSE;
	}

	return gas_create_thread (&pi->poll_thread, gas_event_loop, pi);
}

void *
gas_stop_poller (GAS_POLLER_INFO *pi)
{
	pi->stop = GAS_TRUE;

	pi->server_socket = gas_close_socket (pi->server_socket);
	pi->accept_ci = gas_delete_client (pi->accept_ci, GAS_FALSE);

	gas_client_info *ci = pi->first_client;
	while (ci != NULL) {
		gas_client_info *next = ci->next;
		gas_close_socket (ci->socket);
		gas_delete_client (ci, GAS_TRUE);
		ci = next;
	}

	CloseHandle (pi->cpl_port);

	if (pi->poll_thread != 0) {
		WaitForSingleObject (pi->poll_thread,INFINITE);
		pi->poll_thread = 0;
	}

	gas_clean_clients (pi);
	gas_release_networks (pi);

	free (pi);
	return NULL;
}

int
gas_poller_is_alive (GAS_POLLER_INFO *pi)
{
	if (pi->info_type == GAS_TP_POLLER)
		return pi->poll_thread != 0;
	else
		return 0;
}

__stdcall gas_callback_t
gas_event_loop (void *tpi)
{
	GAS_POLLER_INFO *pi = (GAS_POLLER_INFO *)tpi;
	DWORD size;
	BOOL resultOk;
	WSAOVERLAPPED* ovl_res;
	gas_client_info* ci;

	gas_start_accepting (pi);

	while (!pi->stop) {
		resultOk = gas_get_completion_status(pi, &size, &ci, &ovl_res);
		if (!resultOk)
			break;
		switch (ci->operation) {
		case GAS_OP_ACCEPT:
			gas_debug_message (GAS_IO, "Operation ACCEPT completed\n");
			gas_accept_completed (pi, ci, resultOk, size);
			break;
		case GAS_OP_READ:
			gas_debug_message (GAS_IO, "Operation READ completed\n");
			gas_read_completed (ci, resultOk, size);
			break;
		case GAS_OP_WRITE:
			gas_debug_message (GAS_IO, "Operation WRITE completed\n");
			gas_write_completed (ci, resultOk, size);
			break;
		default:
			gas_debug_message (GAS_IO, "Error, unknown operation!!!\n");
			gas_destroy_connection (ci);		 // hope for the best!
			break;
		}
	}

	pi->poll_thread = 0;			// signal end of thread
	return (gas_callback_t) 0;
}


// specific functions for iocp

BOOL
gas_get_completion_status (GAS_POLLER_INFO *pi, DWORD* size, gas_client_info **pci, WSAOVERLAPPED **ovl_res)
{
	BOOL resultOk;
	*ovl_res = NULL;
	*pci = NULL;

	resultOk = GetQueuedCompletionStatus(pi->cpl_port, size, (PULONG_PTR)pci,	ovl_res, INFINITE);

	if (!resultOk) {
		DWORD err = GetLastError();
		if (!pi->stop)
			gas_error_message("Error %d getting completion port status!!!\n", (int)err);
	}

	else if (!*pci || !*ovl_res) {
		resultOk = GAS_FALSE;
		gas_error_message ("Don't know what to do in getting completion port status!!!\n");
	}

	return resultOk;
}

void
gas_start_accepting (GAS_POLLER_INFO *pi)
{
	DWORD addr_buffer_size = sizeof(struct sockaddr_in) + 16;
	pi->accept_ci->socket = gas_create_socket(GAS_FALSE);
	memset(pi->accept_ci->overlapped, 0, sizeof(WSAOVERLAPPED));

	gas_debug_message (GAS_IO, "Started accepting connections...\n");

	// uses listener's completion key and overlapped structure

	// starts asynchronous accept
	if (!AcceptEx(pi->server_socket, pi->accept_ci->socket, gas_get_buffer_data(pi->accept_ci->rb), 0 /* no recv */,
			addr_buffer_size, addr_buffer_size, NULL, (WSAOVERLAPPED *)pi->accept_ci->overlapped)) {
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
			gas_error_message("Error %d in AcceptEx\n", err);
	}
}

void
gas_accept_completed (GAS_POLLER_INFO *pi, gas_client_info *accept_ci, BOOL resultOk, DWORD size)
{
	DWORD addr_buffer_size = sizeof(struct sockaddr_in) + 16;
	struct sockaddr *LocalSockaddr, *RemoteSockaddr;
	int LocalSockaddrLen=0, RemoteSockaddrLen=0;

	if (resultOk) {
		gas_debug_message (GAS_IO, "New connection created\n");

		GetAcceptExSockaddrs (gas_get_buffer_data(pi->accept_ci->rb), 0, addr_buffer_size, addr_buffer_size,
				&LocalSockaddr, &LocalSockaddrLen, &RemoteSockaddr, &RemoteSockaddrLen);

		if (pi->networks_info != NULL) {
			char address_string[30];
			if (!gas_validate_network (pi, RemoteSockaddr, address_string)) {
				gas_error_message ("Forbidden user: %s\n", address_string);
				closesocket(accept_ci->socket);
				// starts waiting for another connection request
				gas_start_accepting(pi);
				return;
			}
		}

		// "updates the context" (whatever that is...)
		setsockopt(accept_ci->socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
				(char *)&pi->server_socket, sizeof(pi->server_socket));

		// associates new socket with completion port
		gas_client_info* ci = gas_create_client (pi, accept_ci->socket);

		struct sockaddr_in * client_addr_in;
		client_addr_in = (struct sockaddr_in *) &RemoteSockaddr;
		memcpy (&ci->remote_addr, &client_addr_in->sin_addr, sizeof(struct in_addr));	// keep client address for reference
		ci->remote_port = ntohs (client_addr_in->sin_port);								// and remote port
		client_addr_in = (struct sockaddr_in *) &LocalSockaddr;
		memcpy (&ci->local_addr, &client_addr_in->sin_addr, sizeof(struct in_addr));	// keep server address for reference
		ci->local_port = ntohs (client_addr_in->sin_port);								// and local port

		gas_do_callback (ci, GAS_CLIENT_CREATE);

		if (CreateIoCompletionPort((HANDLE)ci->socket, pi->cpl_port, (ULONG_PTR)ci, 0) != pi->cpl_port) {
			int err = WSAGetLastError();
			gas_error_message ("Error %d in CreateIoCompletionPort\n", err);
		}
		else {
			// starts receiving from the new connection
			ci->overlapped = gas_create_overlapped(pi);
			gas_start_reading(ci);
		}
		// starts waiting for another connection request
		gas_start_accepting(pi);
	}

	else {		// !resultOk
		int err = GetLastError();
		gas_debug_message (GAS_IO, "Error (%d,%d) in accept, cleaning up and retrying!!!", err, (int)size);
		closesocket(accept_ci->socket);
		gas_start_accepting(pi);
	}
}

void
gas_start_reading (gas_client_info *ci)
{
	DWORD flags=0, bytes=0;

	gas_trim_buffer (ci->rb);
	GAS_CI_MARK_END (ci->rb);

	if (ci->overlapped!=NULL) {
		memset(ci->overlapped, 0, sizeof(WSAOVERLAPPED));
		ci->operation = GAS_OP_READ;
	}
	WSABUF wsabuf = { GAS_CI_BUFFER_SPACE(ci->rb), GAS_CI_BUFFER_END(ci->rb) };

	if (WSARecv(ci->socket, &wsabuf, 1, &bytes, &flags, (WSAOVERLAPPED *)ci->overlapped, NULL) == SOCKET_ERROR) {
		ci->error = GAS_TRUE;
		int err = WSAGetLastError();
		if (err != WSA_IO_PENDING && err != WSAECONNABORTED)
			gas_error_message ("Error %d in WSARecv\n", err);
	}
	else
		gas_debug_message (GAS_IO, "Immediate read\n");
}

void
gas_read_completed (gas_client_info *ci, BOOL resultOk, DWORD size)
{
	if (resultOk) {
		if (size > 0) {
			gas_debug_message (GAS_IO, "Read operation completed, %d bytes read\n", (int)size);
			GAS_CI_GROW_BUFFER (ci->rb, size);
			*GAS_CI_BUFFER_END(ci->rb) = '\0';		// recv don't terminate buffer with '\0'
			gas_realloc_buffer (ci->rb);
			gas_debug_message (GAS_DETAIL, "WSARecv::<< \"%s\"\n", GAS_CI_BUFFER_MARK(ci->rb));
			gas_adjust_stats();
			gas_do_callback (ci, GAS_CLIENT_READ);		// will start another write
			if (!ci->callback_has_written || !ci->use_write_events)
				gas_start_reading (ci); 		// starts another read
		}
		else {	// size == 0
			gas_debug_message (GAS_IO, "Connection closed by client\n");
			ci->read_end = GAS_TRUE;
			gas_destroy_connection (ci);
		}
	}
	else {		// !resultOk, assumes connection was reset
		ci->error = GAS_TRUE;
		int err = GetLastError();
		gas_debug_message (GAS_IO, "Error %d in recv, assuming connection was reset by client\n", err);
		gas_destroy_connection (ci);
	}
}

void
gas_start_writing (gas_client_info *ci)
{
	DWORD bytes=0;

	ci->can_write = GAS_FALSE;
	ci->callback_has_written = GAS_TRUE;
	gas_debug_message (GAS_IO, "gas_start_writing can_write<-%d\n", ci->can_write);

	gas_client_buffer *cb = ci->use_eb? ci->eb: ci->wb;
	GAS_CI_MARK_START (cb);

	if (ci->overlapped!=NULL) {
		memset(ci->overlapped, 0, sizeof(WSAOVERLAPPED));
		ci->operation = GAS_OP_WRITE;
	}
	WSABUF wsabuf = { GAS_CI_DATA_SIZE(cb), GAS_CI_BUFFER_MARK(cb) };

	if (WSASend(ci->socket, &wsabuf, 1, &bytes, 0, (WSAOVERLAPPED *)ci->overlapped, NULL) == SOCKET_ERROR) {
		ci->error = GAS_TRUE;
		int err = WSAGetLastError();
		if (err != WSA_IO_PENDING && err != WSAECONNABORTED)
			gas_error_message ("Error %d in WSASend\n", err);
	}
	else
		gas_debug_message (GAS_IO, "Immediate write\n");
	gas_debug_message (GAS_DETAIL, "WSASend::>> \"%s\"\n", GAS_CI_BUFFER_START(cb));
}

void
gas_write_completed (gas_client_info *ci, BOOL resultOk, DWORD size)
{
	gas_client_buffer *cb = ci->use_eb? ci->eb: ci->wb;
	if (resultOk) {
		if (size > 0) {
			ci->can_write = GAS_TRUE;
			GAS_CI_WANE_BUFFER (cb, size);
			if (gas_trim_buffer(cb) > 0) {
				gas_debug_message (GAS_IO, "Write incomplete\n");
				gas_start_writing (ci); // starts another write
			}
			else {
				gas_debug_message (GAS_IO, "Write operation completed\n");
				ci->use_eb = GAS_FALSE;					// clear external buffer if was used
				gas_do_callback (ci, GAS_CLIENT_WRITE);
				if (!ci->callback_has_written)
					gas_start_reading (ci); 			// starts another read
			}
		}
		else {		// size == 0 (strange!)
			gas_debug_message (GAS_IO, "Connection closed by client!\n");
			gas_destroy_connection (ci);
		}
	}
	else {		// !resultOk, assumes connection was reset
		ci->error = GAS_TRUE;
		int err = GetLastError();
		gas_error_message("Error %d on send, assuming connection was reset!\n", err);
		gas_destroy_connection (ci);
	}
}

// return from a client't callback
int
gas_do_write_iocp (gas_client_info* ci)
{
	if (ci->use_write_events) {
		gas_debug_message (GAS_IO, "gas_do_write_iocp. can_write==%d\n", ci->can_write);
		if (!ci->can_write) {
			errno = EAGAIN;
			return GAS_ERROR;
		}
		gas_start_writing (ci);
		return 0;
	}
	else
		return gas_do_write (ci);
}

WSAOVERLAPPED *
gas_create_overlapped (GAS_POLLER_INFO *pi)
{
	++pi->actual_connections;
	return (WSAOVERLAPPED*) calloc(1, sizeof(WSAOVERLAPPED));
}

void
gas_destroy_connection (gas_client_info *ci)
{
	((GAS_POLLER_INFO *)ci->tpi)->actual_connections--;
	closesocket(ci->socket);
	gas_delete_client(ci, GAS_TRUE);
}


#endif // WIN32

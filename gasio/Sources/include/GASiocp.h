
///////////////  IMPLÉEMENTATION OF IOCP  ////////////////

#ifndef _GAS_IOCP_H_
#define _GAS_IOCP_H_

#ifdef WIN32


#include "GASdefinitions.h"
#include "GASnetworks.h"
#include "GASclient.h"


typedef struct {
	// common; the first elements must be common with THREAD_INFO
	void* parent;
	int info_type;
	int allow_tasks;
	int clients_are_non_blocking;
	gas_client_config cc;
	gas_socket_t server_socket;
	int actual_connections;
	GAS_NETWORK_INFO *networks_info;
	int networks_size;
	void (*callback)(gas_client_info *ci, int can_defer);
	int stop;
	gas_client_info *first_client;
	gas_client_info *last_client;
	gas_mutex_t lock;
	// specific
	HANDLE cpl_port;
	gas_thread_t poll_thread;
	gas_client_info *accept_ci;
} GAS_POLLER_INFO;


#ifdef __cplusplus
extern "C" {
#endif

// common to epoll, kqueue and iocp
GAS_POLLER_INFO* gas_create_poller (void* parent, char* address, int port, char *networks, void (*callback)(gas_client_info *ci,int op));
int   gas_start_poller         (GAS_POLLER_INFO *pi);
void *gas_stop_poller          (GAS_POLLER_INFO *pi);
int   gas_poller_is_alive      (GAS_POLLER_INFO *pi);
__stdcall gas_callback_t gas_event_loop (void *tpi);

// specific
BOOL gas_get_completion_status (GAS_POLLER_INFO *pi, DWORD*, gas_client_info **pci, WSAOVERLAPPED **ovl);
void gas_start_accepting       (GAS_POLLER_INFO *pi);
void gas_accept_completed      (GAS_POLLER_INFO *pi, gas_client_info *accept_ci, BOOL resultOk, DWORD size);
void gas_start_reading         (gas_client_info *ci);
void gas_read_completed        (gas_client_info *ci, BOOL resultOk, DWORD size);
void gas_start_writing         (gas_client_info *ci);
void gas_write_completed       (gas_client_info *ci, BOOL resultOk, DWORD size);
int  gas_do_write_iocp         (gas_client_info *ci);

WSAOVERLAPPED* gas_create_overlapped (GAS_POLLER_INFO *pi);
void gas_destroy_connection (gas_client_info *ci);

#ifdef __cplusplus
}
#endif


#endif	// WIN32

#endif // _GAS_IOCP_H_

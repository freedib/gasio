
///////////////  IMPLEMENTATION OF GENERIC (NO poll, kqueue or IOCP  ////////////////

#ifndef _GAS_GENERIC_H_
#define _GAS_GENERIC_H_

#if !defined(__linux__) && !defined(__APPLE__) && !defined(WIN32)


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
} GAS_POLLER_INFO;



#ifdef __cplusplus
extern "C" {
#endif

// common to epoll, kqueue and iocp
GAS_POLLER_INFO* gas_create_poller (void* parent, char* address, int port, char *networks, void (*callback)(gas_client_info *ci,int op));
int   gas_start_poller           (GAS_POLLER_INFO *pi);
void *gas_stop_poller            (GAS_POLLER_INFO *pi);
int   gas_poller_is_alive        (GAS_POLLER_INFO *pi);
__stdcall gas_callback_t gas_event_loop (void *tpi);

#ifdef __cplusplus
}
#endif


#endif // !defined(__linux__) && !defined(__APPLE__) && !defined(WIN32)

#endif // _GAS_GENERIC_H_

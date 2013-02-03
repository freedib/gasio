
///////////////  IMPLÉEMENTATION OF KQUEUE  ////////////////

#ifndef _GAS_KQUEUE_H_
#define _GAS_KQUEUE_H_

#ifdef __APPLE__


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
	int poll_handler;
	gas_thread_t poll_thread;
	gas_client_info *accept_ci;
} GAS_POLLER_INFO;


#include <sys/event.h>
typedef struct kevent gas_poll_event;

#define GAS_OP(pev) (pev->flags)
#define GAS_EV(pev) (pev->filter)
#define GAS_DATA(pev) (pev->udata)
#define GAS_READ_EV(pev) (pev->filter == EVFILT_READ)
#define GAS_WRITE_EV(pev) (pev->filter == EVFILT_WRITE)
#define GAS_IGNORE_EV(pev) ((pev->flags&EV_DELETE) | ((pev->filter==EVFILT_WRITE) | (pev->flags&EV_ERROR)))
#define GAS_TRUE_EOF_EV(pev) (pev->flags & EV_EOF)
#define GAS_FALSE_EOF_EV(pev) (pev->flags & EV_EOF)


#ifdef __cplusplus
extern "C" {
#endif

// common to epoll, kqueue and iocp
GAS_POLLER_INFO* gas_create_poller   (void* parent, char* address, int port, char *networks, void (*callback)(gas_client_info *ci,int op));
int   gas_start_poller           (GAS_POLLER_INFO *pi);
void *gas_stop_poller            (GAS_POLLER_INFO *pi);
int   gas_poller_is_alive        (GAS_POLLER_INFO *pi);
__stdcall gas_callback_t gas_event_loop (void *tpi);

// specific (epoll or kqueue)
int  gas_create_poll             ();
int  gas_wait_event              (GAS_POLLER_INFO *pi, gas_poll_event *events_queue, int events_queue_size);
void gas_add_server_to_poll      (gas_client_info *ci);
void gas_add_client_to_poll      (gas_client_info *ci);
void gas_remove_client_from_poll (gas_client_info *ci);
void gas_change_poll             (gas_client_info *ci, int operation, int events);
const char *gas_name_event       (int event);
const char *gas_name_operation   (int operation);

#ifdef __cplusplus
}
#endif


#endif // __APPLE__

#endif // _GAS_KQUEUE_H_

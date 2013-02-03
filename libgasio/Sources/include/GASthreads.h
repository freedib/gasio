
////////////////  IMPLEMENTATION OF THREAD LOOPS  ////////////////

#ifndef _GAS_THREADS_H_
#define _GAS_THREADS_H_


#include "GASdefinitions.h"
#include "GASnetworks.h"
#include "GASclient.h"


typedef struct {
	// common; the first elements must be common with GAS_POLLER_INFO
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
	int worker_threads;
	gas_thread_t *threads;
	volatile unsigned running_threads;
} GAS_THREADS_INFO;


#ifdef __cplusplus
extern "C" {
#endif

GAS_THREADS_INFO* gas_create_threads (void* parent, char* address, int port, char *networks, void (*callback)(gas_client_info *ci,int op), int worker_threads);
int   gas_start_threads           (GAS_THREADS_INFO *ti);
void *gas_stop_threads            (GAS_THREADS_INFO *ti);
int   gas_threads_are_running     (GAS_THREADS_INFO *ti);
__stdcall gas_callback_t gas_worker_thread (void *data);
int   gas_create_thread (gas_thread_t *pidthread, __stdcall gas_callback_t (*worker_thread)(void *data), void *info);
void  gas_wait_thread             (gas_thread_t idthread);

#ifdef __cplusplus
}
#endif


#endif // _GAS_THREADS_H_

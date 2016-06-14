
#include <stdlib.h>		// calloc, free
#ifdef __linux__
#include <signal.h>
#endif

#include "GASsupport.h"
#include "GASsockets.h"
#include "GASthreads.h"


GAS_THREADS_INFO *
gas_create_threads  (void* parent, char* address, int port, char *networks, void (*callback)(gas_client_info *ci,int op), int worker_threads)
{
	gas_socket_t server_socket;

	if ((server_socket = gas_create_server_socket (address, port)) < 0)
		return NULL;
	GAS_THREADS_INFO *ti = calloc (1, sizeof(GAS_THREADS_INFO));
	ti->parent = parent;
	ti->info_type = GAS_TP_THREADS;
	ti->allow_tasks = GAS_FALSE;
	ti->clients_are_non_blocking = GAS_FALSE;
	gas_preset_client_config (ti);
	ti->server_socket = server_socket;
	ti->actual_connections = 0;
	gas_assign_networks ((void *)ti, networks);
	ti->callback = callback;
	ti->stop = GAS_FALSE;
	ti->first_client = NULL;
	ti->last_client = NULL;
	gas_init_clients (ti);

	ti->worker_threads = worker_threads;
	ti->threads = (gas_thread_t*) calloc (worker_threads, sizeof(gas_thread_t));
	ti->running_threads = 0;

	return ti;
}

int
gas_start_threads (GAS_THREADS_INFO *ti)
{
	int it;
	for (it = 0; it < ti->worker_threads; ++it)
		if (! gas_create_thread (&ti->threads[it], gas_worker_thread, ti))
			return GAS_FALSE;
	return GAS_TRUE;
}

// not used for now
void *
gas_stop_threads (GAS_THREADS_INFO *ti)
{
	int it;

	ti->stop = GAS_TRUE;
	ti->server_socket = gas_close_socket (ti->server_socket);

	gas_client_info *ci = ti->first_client;
	while (ci != NULL) {
		gas_client_info *next = ci->next;
		gas_close_socket (ci->socket);
		ci = next;
	}

	for (it = 0; it < ti->worker_threads; it++)
		if (ti->threads[it] != 0) {
#ifdef WIN32
			WaitForSingleObject (ti->threads[it],INFINITE);
#else
#ifdef __linux__
			pthread_kill(ti->threads[it], SIGHUP);
#endif
			pthread_join (ti->threads[it], NULL);
#endif
			ti->threads[it] = 0;
		}

	gas_clean_clients (ti);
	gas_release_networks (ti);

	free (ti);
	return NULL;
}

int
gas_threads_are_running (GAS_THREADS_INFO *ti)
{
	if (ti->info_type == GAS_TP_THREADS)
		return ti->running_threads > 0;
	else
		return 0;
}

#ifdef __linux__
void
threads_signal (int sig, siginfo_t *info, void *ucontext)
{
}
#endif

__stdcall gas_callback_t
gas_worker_thread (void *tpi)
{
	GAS_THREADS_INFO *ti = (GAS_THREADS_INFO *) tpi;
	int thread_id = atomic_add_uint(&ti->running_threads, 1);

#ifdef __linux__
	// to awake accept if linux
	struct sigaction sa;
	sa.sa_handler = NULL;
	sa.sa_sigaction = threads_signal;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGHUP, &sa, NULL) < 0) {
		gas_error_message("error setting sigaction");
		return NULL;
	}
#endif

	while (!ti->stop) {
		gas_client_info *ci = gas_do_accept (tpi, ti->server_socket);
		if (ci && !ti->stop) {
			++ti->actual_connections;
			ci->thread_id = thread_id;
			while (!ci->error && !ci->read_end) {
				if (gas_do_read (ci) > 0)
					gas_do_callback (ci, GAS_CLIENT_READ);
			}
			--ti->actual_connections;
			ci->socket = gas_close_socket (ci->socket);
			ci = gas_delete_client (ci, GAS_TRUE);
		}
	}

	atomic_add_uint(&ti->running_threads, -1);
	return (gas_callback_t) 0;
}



#ifdef WIN32
int
gas_create_thread (gas_thread_t *pidthread, __stdcall gas_callback_t (*worker_thread)(void *data), void *info)
{
	unsigned __attribute__((unused)) dwThreadId = 0;
	*pidthread = (HANDLE)_beginthreadex( NULL, 0, worker_thread, info, 0, &dwThreadId);
	if (*pidthread<0)
		return GAS_FALSE;
	return GAS_TRUE;
}
#else  // WIN32
int
gas_create_thread (gas_thread_t *pidthread, gas_callback_t (*worker_thread)(void *data), void *info)
{
	int rc;
	rc = pthread_create (pidthread, NULL, worker_thread, info);
	if (rc != 0)
		return GAS_FALSE;
	return GAS_TRUE;
}
#endif  // WIN32

void
gas_wait_thread (gas_thread_t idthread)
{
#ifdef WIN32
	WaitForSingleObject (idthread,INFINITE);
#else
	pthread_join (idthread, NULL);
#endif // WIN32
}

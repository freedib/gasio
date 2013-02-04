
#include "GAStasks.h"
#include "GASsockets.h"
#include "GASthreads.h"
#include "GASkqueue.h"
#include "GASepoll.h"
#include "GAStasks.h"
#include "GASiocp.h"
#include "GASserver.h"


int
gas_init_servers ()
{
	return gas_init_sockets ();
}

void *
gas_create_server (void* parent, char* address, int port, char* networks, void (*callback)(gas_client_info* ci,int op), int worker_threads)
{
	if (callback==NULL)
		return NULL;
	if (worker_threads > 0)
		return gas_create_threads (parent, address, port, networks, callback, worker_threads);
	else
		return gas_create_poller (parent, address, port, networks, callback);
}

int
gas_start_server (void* tpi)
{
	if (((GAS_THREADS_INFO *)tpi)->info_type == GAS_TP_THREADS)
		return gas_start_threads (tpi);
	else
		return gas_start_poller (tpi);
}

void *
gas_stop_server (void* tpi)
{
	if (tpi == NULL)
		return NULL;
	if (((GAS_THREADS_INFO *)tpi)->info_type == GAS_TP_THREADS)
		return gas_stop_threads ((GAS_THREADS_INFO *)tpi);
	else
		return gas_stop_poller  ((GAS_POLLER_INFO *)tpi);
}

int
gas_server_is_alive (void* tpi)
{
	return gas_poller_is_alive((GAS_POLLER_INFO *)tpi) | gas_threads_are_running((GAS_THREADS_INFO *)tpi);
}

void
gas_set_defaults (void* tpi, int use_write_events, int read_buffer_size, int write_buffer_size, int read_buffer_limit)
{
	GAS_THREADS_INFO *ti = (GAS_THREADS_INFO *)tpi;
	if (use_write_events != GAS_DEFAULT)
		ti->cc.use_write_events = use_write_events;
	if (read_buffer_size != GAS_DEFAULT)
		ti->cc.read_buffer_size = read_buffer_size;
	if (write_buffer_size != GAS_DEFAULT)
		ti->cc.write_buffer_size = write_buffer_size;
	if (read_buffer_limit != GAS_DEFAULT)
		ti->cc.read_buffer_limit = read_buffer_limit;
}

int
gas_enqueue_message (gas_client_info* ci, int operation)
{
	ci->operation = operation;
	return gas_enqueue_task (ci);
}

// append message to internal buffer and send buffer
int
gas_write_message (gas_client_info* ci, char* message)
{
	gas_append_buffer (ci->wb, message);
	return gas_write_internal_buffer (ci);
}

// send internal buffer
int
gas_write_internal_buffer (gas_client_info* ci)
{
	return gas_query_write ((gas_client_info *)ci);
}

// send external buffer after internal buffer written
int
gas_write_external_buffer (gas_client_info* ci, char* buffer, int size)
{
	ci->eb->buffer = buffer;
	ci->eb->head = 0;
	ci->eb->mark = 0;
	ci->eb->tail = size;
	ci->use_eb = GAS_TRUE;
	return gas_query_write ((gas_client_info *)ci);
}


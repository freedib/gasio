
#include <stdlib.h>			// malloc, free
#include <string.h>			// memmove
#ifdef WIN32
#include <winsock2.h> // needed by "mswsock.h"
#include <mswsock.h>  // for AcceptEx
#include <windows.h>
#endif // WIN32

#include "GASdefinitions.h"
#include "GASsupport.h"
#include "GASclient.h"
#include "GASsockets.h"
#include "GASthreads.h"


void
gas_preset_client_config (void *tpi)
{
	GAS_THREADS_INFO *ti = (GAS_THREADS_INFO *) tpi;

	ti->cc.use_write_events  = GAS_TRUE;		// set GAS_TRUE to check polling events on writes. a little bit faster for kqueues

	ti->cc.read_buffer_size  = 150;			// minimum 32 bytes for windows
	ti->cc.write_buffer_size = 500;			// minimum 32 bytes for windows
}

gas_client_info *
gas_create_client (void *tpi, gas_socket_t socket)
{
	GAS_THREADS_INFO *ti = (GAS_THREADS_INFO *) tpi;
	gas_client_info *ci;
	ci = (gas_client_info *) malloc (sizeof(gas_client_info));
	if (!ci) {
		gas_error_message("No memory\n");
		return NULL;
	}
	ci->tpi         = tpi;
	ci->cc          = &ti->cc;
	ci->use_write_events = ti->clients_are_non_blocking? ti->cc.use_write_events: GAS_FALSE;
	ci->socket      = socket;
	memset (&ci->remote_addr, '\0', sizeof(struct in_addr));
	ci->remote_port = 0;
	memset (&ci->local_addr, '\0', sizeof(struct in_addr));
	ci->thread_id   = 0;
	ci->local_port  = 0;
	ci->read_end    = GAS_FALSE;
	ci->error       = GAS_FALSE;

	// create buffers. write size larger than read size for web applications
	if ((ci->eb = gas_create_client_buffer(0)) == NULL) {
		free(ci);
		return NULL;
	}
	if ((ci->wb = gas_create_client_buffer(ci->cc->write_buffer_size)) == NULL) {
		gas_delete_client_buffer (ci->eb);
		free(ci);
		return NULL;
	}
	if ((ci->rb = gas_create_client_buffer(ci->cc->read_buffer_size)) == NULL) {
		gas_delete_client_buffer (ci->wb);
		gas_delete_client_buffer (ci->eb);
		free(ci);
		return NULL;
	}

	ci->use_eb = GAS_FALSE;

	ci->operation = GAS_OP_NONE;
	ci->overlapped = NULL;
	ci->callback_has_written = GAS_FALSE;
	ci->can_write = GAS_TRUE;
	ci->write_pending = GAS_FALSE;

	// add client to the end of clients linked list. used to close connections while stopping server
	gas_debug_message (GAS_IO, "ci list +> %08x < %08x > %08x == %08x <> %08x\n",
		ci->previous, ci, ci->next, ti->first_client, ti->last_client);
	gas_mutex_lock (&ti->lock);
	ci->previous = NULL;
	ci->next = NULL;
	if (ti->first_client == NULL)
		ti->first_client = ci;
	if (ti->last_client != NULL) {
		ti->last_client->next = ci;
		ci->previous = ti->last_client;
	}
	ti->last_client = ci;
	gas_mutex_unlock (&ti->lock);

	ci->step = -1;
	ci->context = NULL;
	gas_debug_message (GAS_IO, "ci list +< %08x < %08x > %08x == %08x <> %08x\n",
			ci->previous, ci, ci->next, ti->first_client, ti->last_client);

	return ci;
}

gas_client_info *
gas_delete_client (gas_client_info *ci, int want_callback)
{
	if (!ci)
		return NULL;

	// adjust clients linked list. used to close connections while stopping server
	GAS_THREADS_INFO *ti = (GAS_THREADS_INFO *) ci->tpi;
	gas_debug_message (GAS_IO, "ci list -> %08x < %08x > %08x == %08x <> %08x\n",
		ci->previous, ci, ci->next, ti->first_client, ti->last_client);

	if (want_callback)
		gas_do_callback (ci, GAS_CLIENT_DELETE);

	gas_mutex_lock (&ti->lock);
	if (ci->previous == NULL && ci->next == NULL) {
		ti->first_client = NULL;
		ti->last_client = NULL;
	}
	else if (ci->next == NULL) {
		ci->previous->next = NULL;
		ti->last_client = ci->previous;
	}
	else if (ci->previous == NULL) {
		ci->next->previous = NULL;
		ti->first_client = ci->next;
	}
	else {
		ci->previous->next = ci->next;
		ci->next->previous = ci->previous;
	}
	gas_mutex_unlock (&ti->lock);
	gas_debug_message (GAS_IO, "ci list -< %08x < %08x > %08x == %08x <> %08x\n",
		ci->previous, ci, ci->next, ti->first_client, ti->last_client);

	// release client's data
	gas_delete_client_buffer (ci->eb);
	gas_delete_client_buffer (ci->wb);
	gas_delete_client_buffer (ci->rb);
	// external buffer ci->eb is not freed. allocated by external program
	if (ci->overlapped)
		free (ci->overlapped);
	free(ci);
	return NULL;
}

gas_client_buffer *
gas_create_client_buffer (int size)
{
	gas_client_buffer* cb;
	cb          = calloc (1, sizeof(gas_client_buffer));
	cb->head    = 0;
	cb->mark    = 0;
	cb->tail    = 0;
	cb->alloced = size;
	if (size > 0) {
		cb->buffer  = (char*) malloc(cb->alloced+1);
		if (!cb->buffer) {
			gas_error_message("No memory\n");
			return NULL;
		}
		cb->buffer[cb->alloced] = '\0';
	}
	else
		cb->buffer = NULL;
	return cb;
}

gas_client_buffer *
gas_delete_client_buffer (gas_client_buffer *cb)
{
	if (cb==NULL)
		return NULL;
	if (cb->alloced>0 && cb->buffer)	// don't free external buffers
		free (cb->buffer);
	if (cb)
		free (cb);
	return NULL;
}

// empty the buffer or shift data to the beginning
int
gas_trim_buffer (gas_client_buffer* cb)
{
	if (cb->head == cb->tail)
		cb->head = cb->tail = 0;
	else if (cb->head>0) {
		memmove(cb->buffer, cb->buffer+cb->head, cb->tail-cb->head+1);
		cb->tail -= cb->head;
		cb->head = 0;
		cb->mark = 0;
	}
	return cb->tail;
}

// grow buffer if full;
char *
gas_realloc_buffer (gas_client_buffer* cb)
{
	if (cb->alloced == cb->tail)
		gas_realloc_buffer_for (cb, cb->alloced);	// double buffer
	return cb->buffer;
}

// adjust buffer to have size free space
char *
gas_realloc_buffer_for (gas_client_buffer* cb, int size)
{
	char *buffer;
	if (GAS_CI_BUFFER_SPACE(cb) > size)
		return cb->buffer;				// enough space
	cb->alloced += size - GAS_CI_BUFFER_SPACE(cb);
	buffer = (char*)realloc(cb->buffer, cb->alloced+1);
	if (!buffer)
		gas_error_message("No memory (realloc)\n");
	else
		cb->buffer = buffer;
	return cb->buffer;
}

void
gas_append_buffer (gas_client_buffer* cb, char* string)
{
	int size = strlen(string);
	gas_realloc_buffer_for (cb, size);
	if (size > GAS_CI_BUFFER_SPACE(cb))
		size = cb->alloced-cb->tail;		// was not able to allocate new buffer
	strncpy (GAS_CI_BUFFER_END(cb), string, size);
	GAS_CI_GROW_BUFFER (cb, size);
	*GAS_CI_BUFFER_END(cb) = '\0';				// if string was trimmed
}

char* gas_get_buffer_data (gas_client_buffer *cb)
{
	return cb->buffer+cb->head;
}


void
gas_init_clients (void *tpi)
{
	GAS_THREADS_INFO *ti = (GAS_THREADS_INFO *) tpi;
	gas_mutex_init(&ti->lock);
}

void
gas_clean_clients (void *tpi)
{
	GAS_THREADS_INFO *ti = (GAS_THREADS_INFO *) tpi;
	gas_mutex_destroy(&ti->lock);
}

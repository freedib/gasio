
#ifndef _GAS_CLIENT_H_
#define _GAS_CLIENT_H_


#include "GASdefinitions.h"

enum // socket operations for IOCP and tasks
{
	GAS_OP_NONE,
	GAS_OP_ACCEPT,
	GAS_OP_READ,
	GAS_OP_WRITE
};

enum // callback operation
{
	GAS_CLIENT_CREATE,
	GAS_CLIENT_DELETE,
	GAS_CLIENT_READ,
	GAS_CLIENT_WRITE,
	GAS_CLIENT_DEFFERED_READ,
	GAS_CLIENT_DEFFERED_WRITE
};


#define GAS_CI_GET_DATA_HEAD(cb)       (cb->buffer + cb->head)
#define GAS_CI_GET_DATA_MARK(cb)       (cb->buffer + cb->mark)
#define GAS_CI_GET_DATA_TAIL(cb)       (cb->buffer + cb->tail)
#define GAS_CI_GET_DATA_SIZE(cb)       (cb->tail - cb->head)
#define GAS_CI_GET_FREE_SPACE(cb)      (cb->allocated - cb->tail)
#define GAS_CI_GET_INCREASED_SIZE(cb)  (cb->tail - cb->mark)
#define GAS_CI_GET_DECREASED_SIZE(cb)  (cb->head - cb->mark)
#define GAS_CI_SLIDE_HEAD(cb,n)        (cb->head += n)
#define GAS_CI_SLIDE_TAIL(cb,n)        (cb->tail += n)
#define GAS_CI_SET_DATA_TAIL(cb,n)     (cb->tail = n)
#define GAS_CI_CLEAR_BUFFER(cb)        (cb->head = cb->mark = cb->tail = 0)
#define GAS_CI_SET_MARK_AT_HEAD(cb)    (cb->mark = cb->head)
#define GAS_CI_SET_MARK_AT_TAIL(cb)    (cb->mark = cb->tail)

typedef struct GAS_CLIENT_INFO   gas_client_info;
typedef struct GAS_CLIENT_BUFFER gas_client_buffer;
typedef struct GAS_CLIENT_CONFIG gas_client_config;

struct GAS_CLIENT_BUFFER {
	char *buffer;	// 1 byte more than allocated
	int allocated;
	int limit;
	int head;
	int mark;		// previous tail after a write
	int tail;
};

// can be adjusted after
struct GAS_CLIENT_CONFIG {
	int   use_write_events;		// GAS_TRUE if checking write events. faster if we don't check write events (on kqueue)
	int   read_buffer_size;		// default read buffer size
	int   write_buffer_size;	// default write buffer size
	int   read_buffer_limit;	// maximum read buffer size
	int   write_buffer_limit;	// maximum write buffer size
};


// the use_ variables can be set during creation callback by application
struct GAS_CLIENT_INFO {
	void* tpi;					// pointer to the server
	gas_client_config *cc;		// reference to common clients config (tpi->cc)
	int use_write_events;		// if GAS_FALSE, may be changed to GAS_TRUE if write would block
	gas_socket_t socket;		// client socket
	struct in_addr remote_addr;	// client address
	int remote_port;			// client remote port
	struct in_addr local_addr;	// server address
	int local_port;				// server port
	int thread_id;				// reference to the thread number for debugging purpose
	gas_client_buffer* rb;		// read buffer
	gas_client_buffer* wb;		// write buffer
	gas_client_buffer* eb;		// external write buffer
	int   use_eb;				// used internally by gas_do_write and gas_start_writing/gas_write_completed
	int   read_end;				// GAS_TRUE if no more data
	int   error;				// GAS_TRUE if I/O error
	int   operation;			// IOCP and tasks operation
	void* overlapped;			// IOCP overlapped buffer
	int   callback_has_written;	// I/O management. indication that data was written during callback
	int   can_write;			// I/O management. if GAS_FALSE, indicates a write started and waiting for readiness signal
	int   write_pending;		// I/O management. indicates that a buffer is ready to be sent on write readiness
	struct GAS_CLIENT_INFO* previous;
	struct GAS_CLIENT_INFO* next;
	int   step;					// set by user to organize read/write sequences
	void* context;				// user defined data
};


#ifdef __cplusplus
extern "C" {
#endif

void               gas_preset_client_config (void *tpi);
gas_client_info*   gas_create_client        (void *tpi, gas_socket_t socket);
gas_client_info*   gas_delete_client        (gas_client_info *ci, int want_callback);
gas_client_buffer* gas_create_client_buffer (int size, int limit);
gas_client_buffer* gas_delete_client_buffer ();
int   gas_trim_buffer        (gas_client_buffer *cb);
char* gas_realloc_buffer     (gas_client_buffer *cb);
char* gas_realloc_buffer_for (gas_client_buffer* cb, int size);
void  gas_append_buffer      (gas_client_buffer* cb, char* string);
char* gas_get_buffer_data    (gas_client_buffer *cb);

void gas_init_clients  (void *tpi);
void gas_clean_clients (void *tpi);

#ifdef __cplusplus
}
#endif


#endif // _GAS_CLIENT_H_

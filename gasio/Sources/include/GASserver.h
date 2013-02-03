
////////////////  IMPLÉEMENTATION OF SERVERS :: BASE API FOR EXTERNAL PROGRAMS  ////////////////

#ifndef _GAS_SERVER_H_
#define _GAS_SERVER_H_


#include "GASclient.h"
#include "GAStasks.h"


#ifdef __cplusplus
extern "C" {
#endif

int    gas_init_servers    ();
void*  gas_create_server   (void* parent, char* address, int port, char* networks, void (*callback)(gas_client_info* ci,int op), int worker_threads);
int    gas_start_server    (void* tpi);
void*  gas_stop_server     (void* tpi);
int    gas_server_is_alive (void* tpi);
void   gas_set_defaults    (void* tpi, int use_write_events, int read_buffer_size, int write_buffer_size);

int    gas_enqueue_message (gas_client_info* ci, int operation);
int    gas_write_message   (gas_client_info* ci, char *message);
int    gas_write_internal_buffer (gas_client_info* ci);
int    gas_write_external_buffer (gas_client_info* ci, char* buffer, int size);

#ifdef __cplusplus
}
#endif


#endif /* _GAS_SERVER_H_ */

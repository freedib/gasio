
///////////////  IMPLÉEMENTATION OF BASIC SOCKETS OPERATIONS  ////////////////

#ifndef _GAS_SOCKET_H_
#define _GAS_SOCKET_H_


#include "GASdefinitions.h"
#include "GASclient.h"


#ifdef __cplusplus
extern "C" {
#endif

int  gas_init_sockets       (void);
gas_socket_t gas_create_server_socket (char* address, int port);
gas_socket_t gas_create_socket  ();
gas_socket_t gas_close_socket   (gas_socket_t a_socket);
void gas_block_socket       (gas_socket_t a_socket, int block);
int  gas_bind_socket        (gas_socket_t server_socket, char* address, int port);
int  gas_start_listening    (gas_socket_t server_socket);
gas_client_info *gas_do_accept  (void *tpi, gas_socket_t server_socket);
int  gas_do_read            (gas_client_info *ci);
int  gas_do_write           (gas_client_info *ci);
int  gas_query_write        (gas_client_info *ci);
void gas_do_callback        (gas_client_info *ci, int op);
void gas_get_hostname       (char *hostname, int namesize);

#ifdef __cplusplus
}
#endif


#endif // _GAS_SOCKET_H_

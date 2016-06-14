
////////////////  NETWORKS SUPPORT : ALLOW FILTERING OF INCOMING NETWORKS  ////////////////

// if networks_list supplied, clients address will be checked against networks list


#ifndef _GAS_NETWORKS_H_
#define _GAS_NETWORKS_H_

#if !defined(__linux__) && !defined(__APPLE__) && !defined(WIN32)
#include <sys/socket.h>	// for generic platform
#endif

#include "GASdefinitions.h"


// network structure
typedef struct {
	struct in_addr mask;				// validity mask
	struct in_addr addr;
} GAS_NETWORK_INFO;


#ifdef __cplusplus
extern "C" {
#endif

void gas_assign_networks   ( void *tpi, char *networks_list );
int  gas_validate_network  ( void *tpi, struct sockaddr *client_address, char *address_string );
void gas_release_networks  ( void *tpi );

#ifdef __cplusplus
}
#endif


#endif // _GAS_NETWORKS_H_

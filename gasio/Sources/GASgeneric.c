
#if !defined(__linux__) && !defined(__APPLE__) && !defined(WIN32)

#include "GASgeneric.h"

// agnostic

GAS_POLLER_INFO *
gas_create_poller (void* parent, char* address, int port, char *networks, void (*callback)(gas_client_info *ci,int op))
{
	return NULL;
}

int
gas_start_poller (GAS_POLLER_INFO *pi)
{
	return GAS_FALSE;
}

void *
gas_stop_poller (GAS_POLLER_INFO *pi)
{
	pi->stop = GAS_TRUE;
	return NULL;
}

int
gas_poller_is_alive (GAS_POLLER_INFO *pi)
{
	return 0;
}



#endif // !defined(__linux__) && !defined(__APPLE__) && !defined(WIN32)

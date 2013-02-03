
#include "GASsockets.h"


#include <stdio.h>		// snprintf, sscanf
#include <stdlib.h>		// calloc, free
#include <string.h>		// strchr, strcpy, strncpy

#include "GASnetworks.h"
#include "GASthreads.h"


#define ADDRESS_SIZE 32

// create valid networks list
void
gas_assign_networks (void *tpi, char *networks_list )
{
	GAS_THREADS_INFO *ti = (GAS_THREADS_INFO *) tpi;		// THREAD_INFO and GAS_POLLER_INFO have the same first elements
	char *pn, *ps;
	int b1, b2, b3, b4;
	static char* masks[] = { "0.0.0.0", "255.0.0.0", "255.255.0.0", "255.255.255.0", "255.255.255.255" };
	char address[ADDRESS_SIZE];

	if (networks_list==NULL || *networks_list=='\0')
		return;

	if (ti->networks_info!=NULL)
		gas_release_networks (tpi);

	// allocate networks
	pn=networks_list;
	ti->networks_size = 0;
	while ((pn = strchr (pn, ':')) != NULL) {
		++pn;
		++ti->networks_size;
	}
	ti->networks_info = calloc (ti->networks_size+1, sizeof(GAS_NETWORK_INFO));

	pn=networks_list;
	ti->networks_size = 0;
	do {
		ps = strchr (pn, ':');
		if (ps != NULL) {
			strncpy (address, pn, ps-pn);
			++ ps;
		}
		else
			strcpy (address, pn);
		pn = ps;

		b1 = b2 = b3 = b4 = 0;
		int parts = sscanf (address, "%d.%d.%d.%d", &b1, &b2, &b3, &b4);
		if (parts<0)
			parts = 0;				// adresse vide.
		snprintf (address, ADDRESS_SIZE, "%d.%d.%d.%d", b1, b2, b3, b4);
		GAS_NETWORK_INFO *ni = &((GAS_NETWORK_INFO *)ti->networks_info)[ti->networks_size];

#ifdef WIN32
		ni->addr.S_un.S_addr = inet_addr(address);
		ni->mask.S_un.S_addr = inet_addr(masks[parts]);
#else
		inet_aton(address, &ni->addr);
		inet_aton(masks[parts], &ni->mask);
#endif
		++ti->networks_size;

		pn = ps;					// passe aux autres réseaux
	} while (pn != NULL);
}

void
gas_release_networks ( void *tpi )
{
	GAS_THREADS_INFO *ti = (GAS_THREADS_INFO *) tpi;		// THREAD_INFO and GAS_POLLER_INFO have the save 5 first elements
	if (ti->networks_info)
		free(ti->networks_info);
	ti->networks_info = NULL;
	ti->networks_size = 0;
}

int
gas_validate_network ( void *tpi, struct sockaddr *client_addr, char *address_string )
{
	GAS_THREADS_INFO *ti = (GAS_THREADS_INFO *) tpi;		// THREAD_INFO and GAS_POLLER_INFO have the save 5 first elements
	int valid = GAS_FALSE, pn;
	struct sockaddr_in *client_addr_in = (struct sockaddr_in *) client_addr;

	if (ti->networks_info==NULL)
		return GAS_TRUE;

	for (pn=0; pn<ti->networks_size; pn++) {
		GAS_NETWORK_INFO *ni = &((GAS_NETWORK_INFO *)ti->networks_info)[pn];
#ifdef WIN32
		if (ni->addr.S_un.S_addr == (client_addr_in->sin_addr.S_un.S_addr & ni->mask.S_un.S_addr) )
#else
		if (ni->addr.s_addr == (client_addr_in->sin_addr.s_addr & ni->mask.s_addr) )
#endif
			valid = GAS_TRUE;
	}
	if (!valid) {
		strcpy (address_string, inet_ntoa(client_addr_in->sin_addr));
		return GAS_FALSE;
	}
	return GAS_TRUE;
}

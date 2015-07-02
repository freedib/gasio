#define BASIC 1

#ifdef BASIC


#include <stdio.h>
#include <GASserver.h>

// default callback. do an echo
void client_callback (gas_client_info *ci, int op) {
    if (op==GAS_CLIENT_READ)
        gas_write_message (ci, gas_get_buffer_data (ci->rb));
}

int main (int argc, char *const argv[]) {
    int port = 8000;
    void* server;

    if (gas_init_servers() < 0)
        return 1;
    if ((server = gas_create_server (NULL, NULL, port, NULL, client_callback, 0)) == NULL)
        return 1;
    if (! gas_start_server (server))
        return 1;

    while (fgetc(stdin) != '!')     // accept messages until user press !
        ;

    gas_stop_server (server);

    return 0;
}


#else	// BASIC


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef WIN32
#include <windows.h>
#else

#include <unistd.h>			// sleep
#endif

#include "GASserver.h"
#include "GASsupport.h"


void help ()
{
	fprintf (stderr, "echo [options]\n");
	fprintf (stderr, "options:\n");
	fprintf (stderr, "  -a 9.9.9.9: server address   default: INADDR_ANY\n");
	fprintf (stderr, "  -p 9999:    port number      default: 8080\n");
	fprintf (stderr, "  -w 9999:    worker threads   default: 0 (synchronous). >0 = asynchronous\n");
	fprintf (stderr, "  -n sss:sss: networks         format: 255:255.255:255.255.255:255.255.255.255\n");
	fprintf (stderr, "\n");
}


// default callback. do an echo
void
client_callback (gas_client_info *ci, int op) {
	char *static_text = "Text for every one\n";
	int test_queues = 1;
	int extra_text = 0;
	switch (op) {
	case GAS_CLIENT_CREATE:
		gas_debug_message (GAS_CLIENT, "client_create\n");
		break;
	case GAS_CLIENT_DELETE:
		gas_debug_message (GAS_CLIENT, "client_delete\n");
		break;
	case GAS_CLIENT_READ:
		gas_debug_message (GAS_CLIENT, "client_read\n");
		if (test_queues && strstr(gas_get_buffer_data(ci->rb),"hello")!=NULL)
			if (gas_enqueue_message (ci, GAS_OP_READ) >= 0)			// should depend on request type: heavy requests
				return;
		// was not able to enqueue; continue as normal read
		/* no break */
	case GAS_CLIENT_DEFFERED_READ:
		gas_debug_message (GAS_CLIENT, "client_deffered_read\n");
		ci->step = 0;
		gas_write_message (ci, gas_get_buffer_data (ci->rb));
		break;
	case GAS_CLIENT_WRITE:
		gas_debug_message (GAS_CLIENT, "client_write\n");
		if (test_queues && ci->step==1 && gas_enqueue_message (ci, GAS_OP_WRITE) >= 0)	// test to enqueue write of static_text
			return;
		// was not able to enqueue; continue as normal read
		/* no break */
	case GAS_CLIENT_DEFFERED_WRITE:
		gas_debug_message (GAS_CLIENT, "client_deffered_write\n");
		switch (++ci->step) {
		case 1:
			if (extra_text)
				gas_write_external_buffer (ci, static_text, strlen(static_text));
			break;
		case 2:
			if (extra_text)
				gas_write_message (ci, "Nice\n");
			break;
		}
		break;
	}
}


int main (int argc, char *const argv[])
{
	// configuration
	char* address[2]         = {NULL,NULL};			// NULL if no address
	int   ports[2]           = {8000,8008};
	int   worker_threads[2]  = {0,2};	// 100		// 0 if asynchronous. for threads, optimum may vary with systems
	char* networks[2]        = {NULL,NULL};			// NULL if no restriction

	void *tps[2]            = {NULL,NULL};
	int it, loop;

#ifdef WANT_TO_TEST_TASKS
	gas_test_tasks ();
	exit (0);
#endif

#ifdef WIN32
	// � �ter en mode production
	setvbuf (stdout, NULL, _IONBF, 0);
	setvbuf (stderr, NULL, _IONBF, 0);
#endif

	while (--argc > 0) {
		char *popt = *++argv;
		if (*popt == '-') {
			while (*popt) {
				switch (*popt++) {
				case '-':
					break;
				case 'h':
					help();
					break;
				case 'a':
					if (--argc>0)
						address[0] = *++argv;
					break;
				case 'A':
					if (--argc>0)
						address[1] = *++argv;
					break;
				case 'p':
					if (--argc>0)
						ports[0] = atoi(*++argv);
					break;
				case 'P':
					if (--argc>0)
						ports[1] = atoi(*++argv);
					break;
				case 'w':
					if (--argc>0)
						worker_threads[0] = atoi(*++argv);
					break;
				case 'W':
					if (--argc>0)
						worker_threads[1] = atoi(*++argv);
					break;
				case 'n':
					if (--argc>0)
						networks[0] = *++argv;
					break;
				case 'N':
					if (--argc>0)
						networks[1] = *++argv;
					break;
				case 'd':
					if (--argc>0)
						gas_set_debug_level (atoi(*++argv), 0, NULL);
					break;
				default:
					fprintf (stderr, "Invalid option: %c\n\n", *popt);
					help();
					break;
				}
			}
		}
	}

	fprintf(stderr,"Listening port: %s:%d, %s:%d\n",
			address[0]==NULL?"-":address[0], ports[0], address[1]==NULL?"-":address[1], ports[1]);
	fprintf(stderr,"Mode: %s, %s\n", worker_threads[0]? "Synchronous": "Asynchronous", worker_threads[1]? "Synchronous": "Asynchronous");
	fprintf(stderr,"Worker threads: %d, %d\n", worker_threads[0], worker_threads[1]);
	fprintf(stderr,"Networks: %s, %s\n", networks[0], networks[1]);

	gas_reset_stats ();

	if (gas_init_servers() < 0)
		exit (1);

	for (loop=0; loop<1; loop++) {

		fprintf (stderr, "start\n");
		for (it=0; it<2; it++) {
			if ((tps[it] = (void *) gas_create_server (NULL, address[it], ports[it], networks[it], client_callback, worker_threads[it])) == NULL)
				exit (1);
			// change default client configuration if required. eg default buffer size
			gas_set_defaults (tps[it], GAS_DEFAULT, GAS_DEFAULT, GAS_DEFAULT, GAS_DEFAULT, GAS_DEFAULT);
			if (! gas_start_server (tps[it]))
				exit (1);
		}

		sleep (1);			// allow threads to wake up

		int count=0;
		while ( gas_server_is_alive(tps[0]) || gas_server_is_alive(tps[1])) {
			sleep(1);
			gas_compute_stats ();
			if (++count>3600)
				break;
		}

		fprintf (stderr, "stop\n");
		for (it=0; it<2; it++)
			tps[it] = gas_stop_server (tps[it]);
	}

	return 0;
}

#endif	// BASIC

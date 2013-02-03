# How to use

To use gasio, you just create a gasio instance indicating the port number, start it and, at the end of the application, stop it.  
The server will run in background interacting with the application thru a callback function

A callback in user space will receive messages and allow to answer.

# A very simple echo program

	#include <stdio.h>
	#include <GASserver.h>
    
	// default callback. do an echo
	void client_callback (gas_client_info *ci, int op) {
		if (op==GAS_CLIENT_READ)
			gas_write_message (ci, gas_get_buffer_data (ci->rb));
	}

	int main (int argc, char *const argv[]) {
		int port = 8080;
		void* server;

		if (gas_init_servers() < 0)
			return 1;
		if ((server = gas_create_server (NULL, NULL, port, NULL, client_callback, 0)) == NULL)
			return 1;
		if (! gas_start_server (server))
			return 1;
	
		while (fgetc(stdin) != '!')		// accept messages until user press !
			;

		gas_stop_server (server);
	
		return 0;
	}

It can be tested with a simple telnet client

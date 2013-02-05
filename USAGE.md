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

# Quick API reference - main

	int gas_init_servers ()
		initalize network layer

	void* gas_create_server (void* parent, char* address, int port, char* networks,
	                         void (*callback)(gas_client_info* ci,int op), int worker_threads)
	Description:
		Create a gasio server. Many servers can be created using different ports
	Arguments:
		parent		Pointer to parent's class (this). May be used in callback routine. NULL in standard C
		address		Binding address on server. Format 255.255.255.255. If NULL, bind to INADDR_ANY
		port		Port for listening
		networks	List of valid networks. Format 255.255.255.255:255.255.255:255.255:255. If NULL, no filtering
		callback	Callback routine
		worker_thread	If 0, use asynchrnous events. If > 0, number of threads request threads
	Return:
		A gasio handle, or NULL if error

	void  gas_set_defaults (void* tpi, int use_write_events,
	                        int read_buffer_size, int write_buffer_size, int read_buffer_limit)
	Description:
		Change defaults for clients. Sould be called after gas_create_server and before gas_start_server.
		All arguments may have a GAS_DEFAULT value.
	Arguments:
		tpi			A gasio handle
		use_write_events	If FALSE (asynchronous), use events only on read
		read_buffer_size	Read buffer size. Default = 150. This size will autogrow up to read_buffer_limit
		write_buffer_size	Write buffer size. Default = 500. This size will autogrow with no limit
		read_buffer_limit	Read buffer limit. Default = -1, no limit.

	int gas_start_server (void* tpi)
	Description:
		Start the gasio server
	Arguments:
		tpi		A gasio handle
	Return:
		GAS_TRUE or GAR_FALSE if error.

	int gas_server_is_alive (void* tpi)
	Description:
		Verify if the gasio server is alive
	Arguments:
		tpi		A gasio handle
	Return:
		GAS_TRUE or GAR_FALSE.

	void* gas_stop_server (void* tpi)
	Description:
		Stop the gasio server
	Arguments:
		tpi		A gasio handle
	Return:
		GAS_TRUE or GAR_FALSE if error.


# Quick API reference - callback

	int gas_enqueue_message (gas_client_info* ci, int operation)

	int gas_write_message   (gas_client_info* ci, char *message)

	int gas_write_internal_buffer (gas_client_info* ci)

	int gas_write_external_buffer (gas_client_info* ci, char* buffer, int size)

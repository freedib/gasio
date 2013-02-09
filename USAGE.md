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

The callback client can receive up to 6 kind of events: GAS_CLIENT_CREATE, GAS_CLIENT_DELETE, GAS_CLIENT_READ, GAS_CLIENT_WRITE, GAS_CLIENT_READ_DEFFERED, GAS_CLIENT_WRITE_DEFFERED

A client_info which caracterize the connection have a read buffer and a write buffer. Many macros and a few functions help manipulating theses buffers to access the input data or create new output data.
Input data doesn't always begin at the start of the buffer. So there are a head pointer and a tail pointer which delimitate the input data. In the case where multiple reads are required, a mark pointer indicates the start of the last read (at the first read, it is the same as the head pointer).
On output, the head and the tail delimitate the data to send. If multiple writes are required, the mark pointer moves toward tail pointer until all data is written.

# Linking with C++

With C++ we want to reference classes. In this case we can do:

In class MyClass

	...
	if ((server = gas_create_server (this, NULL, port, NULL, client_callback, 0)) == NULL)
		return 1;
	...

And in callback

	void client_callback (gas_client_info *ci, int op) {
		GAS_THREADS_INFO* ti = (GAS_THREADS_INFO*) ci->tpi;
		((MyClass *)tpi->parent)->my_callback (ci, op);
	}

# Linking user data

Usually, we will want have user data linked with the connection. GAS_CLIENT_CREATE and GAS_CLIENT_DELETE manage this
 
	// default callback. send a banner after the echo
	void client_callback (gas_client_info *ci, int op) {
		switch (op) {
		...
		case GAS_CLIENT_CREATE:
			ci->context = (void *) malloc (10);
			break;
		case GAS_CLIENT_DELETE:
			if (ci->context)
				free (ci->context);
			break;
		...
		}
	}

# Using multiple writes for one read

In some cases, write data can't be sent in one chunk.  
If use_write_events is set to GAS_FALSE (see gas_set_defaults), should no have problem, writes are synchronous and write operation will wait completion before return, so we can enqueue multiple write. (might be false in heavy traffic conditions).  
If use_write_events is set to GAS_TRUE (the default for asynchronous IO), we have to wait a GAS_WRITE event to make the next write.  

	// default callback. send a banner after the echo
	void client_callback (gas_client_info *ci, int op) {
		char *static_text = "Text for every one\n";
		switch (op) {
		...
		case GAS_CLIENT_READ:
			ci->step = 0;
			gas_write_message (ci, gas_get_buffer_data (ci->rb));
			break;
		case GAS_CLIENT_WRITE:
			switch (++ci->step) {
			case 1:
				gas_write_external_buffer (ci, static_text, strlen(static_text));
				break;
			}
			break;
		...
		}
	}

# Using tasks

Tasks allow to defer big requests, Typical use is shown below. If tasks are supported and message requires heavy treatment, message will be put on a queue and a callback message with GAS_CLIENT_DEFFERED_READ will be called later. If tasks are not supported, flow continues to GAS_CLIENT_DEFFERED_READ case.

	// default callback. do an echo
	void client_callback (gas_client_info *ci, int op) {
		switch (op) {
		...
		case GAS_CLIENT_READ:
			if (strstr(gas_get_buffer_data(ci->rb),"big_request")!=NULL)
				if (gas_enqueue_message (ci, GAS_OP_READ) >= 0)
					return;
			// was not able to enqueue; continue as normal read
			/* no break */
		case GAS_CLIENT_DEFFERED_READ:
			gas_write_message (ci, gas_get_buffer_data (ci->rb));
				break;
		...
		}
	}

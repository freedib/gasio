# Quick API reference

# _GASIO functions - main_

The functions are called from the main loop and used to create, configure, start and stop a gasio server


### gas_init_servers

**Description**  
Initialize the network layer

**Synopsis**  
int gas_init_servers ()

**Return**  
GAS_TRUE or GAS_FALSE if error


### gas_create_server
	
**Description**  
Create a gasio server. Many servers can be created using different ports

**Synopsis**  
void* __gas_create_server__ (void* _parent_, char* _address_, int _port_, char* _networks_,
void (* _callback_ )(gas_client_info* ci, int _op_), int _worker_threads_)

**Arguments**

- parent 
    - Pointer to parent's class (this). May be used in callback routine. NULL in standard C 
- address
    - Binding address on server. Format 255.255.255.255. If NULL, bind to INADDR_ANY
- port
    - Listening port
- networks
    - List of valid networks. Format 255.255.255.255:255.255.255:255.255:255. If NULL, no filtering
- callback
    - Callback routine
- worker_thread
    - If 0, use asynchronous events. If > 0, number of threads request threads

**Return**  
A gasio handle, or NULL if error


### gas_set_defaults

**Description**  
Change defaults for clients. Should be called after gas_create_server and before gas_start_server.  
All arguments may have a GAS_DEFAULT value.

**Synopsis**  
void  gas_set_defaults (void* tpi, int use_write_events, int read_buffer_size, int write_buffer_size, int read_buffer_limit, int write_buffer_limit)
	                        
**Arguments**

- tpi
    - A gasio handle
- use_write_events
    - If GAS_FALSE (asynchronous), use events only on read
- read_buffer_size
    - Read buffer size. Default = 150. This size will autogrow up to read_buffer_limit
- write_buffer_size
    - Write buffer size. Default = 500. This size will autogrow with no limit
- read_buffer_limit
    - Read buffer limit. Default = -1, no limit
- write_buffer_limit
    - Write buffer limit. Default = -1, no limit

**Return**  
None


### gas_start_server

**Description**  
Start the gasio server. The gasio server is run in a separate thread

**Synopsis**  
int gas_start_server (void* tpi)
	                        
**Arguments**

- tpi
    - A gasio handle

**Return**  
GAS_TRUE or GAS_FALSE if error


### gas_server_is_alive

**Description**  
Verify if the gasio server is alive

**Synopsis**  
int gas_server_is_alive (void* tpi)
	                        
**Arguments**

- tpi
    - A gasio handle

**Return**  
GAS_TRUE or GAS_FALSE if error


### gas_stop_server

**Description**  
Stop the gasio server

**Synopsis**  
void* gas_stop_server (void* tpi)
	                        
**Arguments**
- tpi
    - A gasio handle

**Return**  
GAS_TRUE or GAS_FALSE if error


# _Quick API reference - callback_

The goal of the callback function is to send back some data to the client depending on input data.


### client_callback

**Description**  
The callback function

**Synopsis**  
void client_callback (gas_client_info *ci, int op)
	                        
**Arguments**

- ci
    - A pointer to a client_info structure
- op
    - The event type
        - GAS_CLIENT_CREATE
            - called when a new connection is open. allows to associate user data with the connection
        - GAS_CLIENT_DELETE
            - called when the connection is closed. allows to clean any user assiciated with the connection
        - GAS_CLIENT_READ
            - called upon reception of a new message. if the request is not complete, just return from the callback to continue reading
        - GAS_CLIENT_WRITE
            - called after a message have been succesfully written. if multiple messages are writen in sequence after a read, we have to wait this event before the next write
        - GAS_CLIENT_READ_DEFFERED
            - makes senses with epoll and kqueue. if a request is known to be long to execute (complex database query), it is possible to push the message on a worker thread during the GAS_CLIENT_READ event. the GAS_CLIENT_READ_DEFERED is called from the worker thread and doesn't affect the polling loop. 
        - GAS_CLIENT_WRITE_DEFFERED
            - save as GAS_CLIENT_READ_DEFFERED for write events. could be used to send large files.

**Return**  
None

From the callback function, data can be sent back to the client using the following functions

### gas_enqueue_message

**Description**  
Push the message to a queue for deffered treatment

**Synopsis**  
int gas_enqueue_message (gas_client_info* ci, int operation)
	                        
**Arguments**

- ci
    - A pointer to a client_info structure
- operation
    - The same as the callback's op. GAS_CLIENT_CREATE or GAS_CLIENT_WRITE

**Return**  
Number of tasks waiting or GAS_ERROR


### gas_write_internal_buffer

**Description**  
Send the client_info internal buffer to the network

**Synopsis**  
int gas_write_internal_buffer (gas_client_info* ci)
	                        
**Arguments**

- ci
    - A pointer to a client_info structure

**Return**  
Number of bytes written (0 if iocp) or GAS_ERROR


### gas_write_external_buffer

**Description**  
Send an arbitrary buffer to the network. The buffer won't be freed

**Synopsis**  
int gas_write_external_buffer (gas_client_info* ci, char* buffer, int size)
	                        
**Arguments**

- ci
    - A pointer to a client_info structure
- buffer
    - A pointer to the buffer
- size
    - the buffer size

**Return**  
Number of bytes written (0 if iocp) or GAS_ERROR


### gas_write_message

**Description**  
Append the message to the internal buffer and send it to the network

**Synopsis**  
int gas_write_message (gas_client_info* ci, char *message)
	                        
**Arguments**

- ci
    - A pointer to a client_info structure

- message
    - A null terminated string

**Return**  
Number of bytes written (0 if iocp) or GAS_ERROR


# _Quick API reference - buffer manipulations_

Many macros are provided to manipulate the buffer data. Many of these macros are internal usage and would not be userful to the user program. The most importants are in bold.

- __GAS_CI_BUFFER_START(cb)__
    - return the beginning of the data (useful in reading). The function gas_get_buffer_data provide the same functionality
- GAS_CI_BUFFER_MARK(cb)
    - return the beginning of the data mark (useful to display incoming data reading)
- __GAS_CI_BUFFER_END(cb)__
    - return the end of the buffer (useful for appending data for writing)
- __GAS_CI_DATA_SIZE(cb)__
    - return the data length
- GAS_CI_BUFFER_SPACE(cb)
    - return free space in the buffer for writing
- GAS_CI_GROWED_SIZE(cb)
    - return the length of the last read operation
- GAS_CI_WANED_SIZE(cb)
    - return the length of the last write operation
- GAS_CI_GROW_BUFFER(cb,n)
    - adjust the tail pointer. Useful if manually added data to the write buffer
- GAS_CI_WANE_BUFFER(cb,n)
    - adjust the head pointer. Useful to skip input data
- __GAS_CI_SIZE_BUFFER(cb,n)__
    - adjust the tail pointer to a fixed position. Useful when writing raw data in output buffer
- GAS_CI_CLEAR_BUFFER(cb) 
    - empty the buffer
- GAS_CI_MARK_START(cb)
    - reset mark (internal: read)
- GAS_CI_MARK_END(cb)
    - reset mark (internal: write)

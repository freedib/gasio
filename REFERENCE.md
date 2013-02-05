# Quick API reference

# _GASIO functions - main_

### gas_init_servers

**Description**  
Initialize the network layer

**Synopsis**  
`int gas_init_servers ()`

**Return**  
GAS_TRUE or GAS_FALSE if error


### gas_create_server
	
**Description**  
Create a gasio server. Many servers can be created using different ports

**Synopsis**  
`void* gas_create_server (void* parent, char* address, int port, char* networks, void (*callback)(gas_client_info* ci,int op), int worker_threads)`

**Arguments**
<table>
    <tr><td> parent </td><td> Pointer to parent's class (this). May be used in callback routine. NULL in standard C </td></tr>
    <tr><td> address </td><td> Binding address on server. Format 255.255.255.255. If NULL, bind to INADDR_ANY </td></tr>
    <tr><td> port </td><td> Listening port </td></tr>
    <tr><td> networks </td><td> List of valid networks. Format 255.255.255.255:255.255.255:255.255:255. If NULL, no filtering </td></tr>
    <tr><td> callback </td><td> Callback routine </td></tr>
    <tr><td> worker_thread </td><td> If 0, use asynchronous events. If > 0, number of threads request threads </td></tr>
</table>

**Return**  
A gasio handle, or NULL if error


### gas_set_defaults

**Description**  
Change defaults for clients. Should be called after gas_create_server and before gas_start_server.  
All arguments may have a GAS_DEFAULT value.

**Synopsis**  
`void  gas_set_defaults (void* tpi, int use_write_events, int read_buffer_size, int write_buffer_size, int read_buffer_limit)`
	                        
**Arguments**
<table>
    <tr><td> tpi </td><td> A gasio handle </td></tr>
    <tr><td> use_write_events </td><td> If GAS_FALSE (asynchronous), use events only on read </td></tr>
    <tr><td> read_buffer_size </td><td> Read buffer size. Default = 150. This size will autogrow up to read_buffer_limit </td></tr>
    <tr><td> write_buffer_size </td><td> Write buffer size. Default = 500. This size will autogrow with no limit </td></tr>
    <tr><td> read_buffer_limit </td><td> Read buffer limit. Default = -1, no limit </td></tr>
</table>

**Return**  
None


### gas_start_server

**Description**  
Start the gasio server

**Synopsis**  
`int gas_start_server (void* tpi)`
	                        
**Arguments**
<table>
    <tr><td> tpi </td><td> A gasio handle </td></tr>
</table>

**Return**  
GAS_TRUE or GAS_FALSE if error


### gas_server_is_alive

**Description**  
Verify if the gasio server is alive

**Synopsis**  
`int gas_server_is_alive (void* tpi)`
	                        
**Arguments**
<table>
    <tr><td> tpi </td><td> A gasio handle </td></tr>
</table>

**Return**  
GAS_TRUE or GAS_FALSE if error


### gas_stop_server

**Description**  
Stop the gasio server

**Synopsis**  
`void* gas_stop_server (void* tpi)`
	                        
**Arguments**
<table>
    <tr><td> tpi </td><td> A gasio handle </td></tr>
</table>

**Return**  
GAS_TRUE or GAS_FALSE if error


# _Quick API reference - callback_

	int gas_enqueue_message (gas_client_info* ci, int operation)

	int gas_write_message   (gas_client_info* ci, char *message)

	int gas_write_internal_buffer (gas_client_info* ci)

	int gas_write_external_buffer (gas_client_info* ci, char* buffer, int size)

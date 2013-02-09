# GASIO library

General Asynchronous and Synchronous I/O library 

A general network I/O library for Linux, Windows & BSD servers

Provides the TCP layer for game servers and Web servers

# Features
- C10K library. Tested at 60k echo requests
- Asynchronous I/O: epoll, kqueue, iocp
- Synchronous I/O: threads
- Deferred treatment for epoll/kqueue (e.g. database access)
- Very simple interface using callback interface

# Companion programs
- Echo server based on GASIO
- Echo tester for GASIO

# History
This library was first intended for a game server. It should be able to handle > 10 000 simultaneous requests and compute a result within 10 seconds. Web server are not made to handle a lot of simultaneous short dynamic requests. The answer is put a lot of servers or use an efficient requests server.  
The clear answer was epoll/kqueue/iocp but I didn't want to limit to a server technology. We never know where we  can be hosted. Existing libraries like libeio, libev, boost or libowfat could have been nice but are huge and none handled iocp efficiently. So here is my library which get ideas from node.js who's asynchronous model is great

# TODO
- Documentation which will be issued shortly
- A simpler echo program. This one was used to test all possibilities of the library
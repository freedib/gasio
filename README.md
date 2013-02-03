GASIO library

General Asynchronous and Synchronous I/O library 

A general network I/O library for Linux, Windows & BSD servers

Provides the TCP layer for game servers and Web servers

Features
- C10K library. Tested at 60k echo requests
- Asynchronous I/O: epoll, kqueue, iocp
- Synchronous I/O: threads
- Deferred treatment for epoll/kqueue (eg database access)
- Very simple interface using callback interface

Companion programs
- Echo server based on GASIO
- Echo tester for GASIO

Note
Why do not use existing libraries like libeio, libev, boost or libowfat? Because they were huge libraries and none handled iocp efficiently

TODO
- Documentation which will be issued shortly
- A simpler echo program. This one was used to test all possibilities of the library
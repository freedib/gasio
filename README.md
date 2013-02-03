GASIO library

General Asynchronous and Synchronous I/O library 

A general network I/O library for Linux, Windows & BSD servers

Usage: TCP layer for game servers and Web servers

Features
- C10K library. Tested at 60k echo requests
- Asynchronous I/O: epoll, kqueue, iocp
- Synchronous I/O: threads
- Deferred treatment for epoll/kqueue (eg database access)
- Very simple interface using callback interface

TODO
- Documentation which will be issued shortly
- Settings for eclipse (Windows and Linux). Actual ones are for Mac OS X
- A makefile
- A simpler echo program. This one was used to test all possibilities of the library
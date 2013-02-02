gasio
=====

General Asynchronous and Synchronous I/O library 

A general network I/O library for Linux, Windows & BSD servers

features

- C10K library. Tested at 60k echo requests
- Asynchronous I/O: epoll, kqueue, iocp
- Synchronous I/O: threads
- Deffered treatment for epoll/kqueue (eg database access)
- Very simple interface using callback interface

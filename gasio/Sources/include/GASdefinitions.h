
#ifndef _GAS_DEFINITIONS_H_
#define _GAS_DEFINITIONS_H_


#ifdef WIN32
#include <winsock2.h>
#include <process.h>
#else  // WIN32
#include <arpa/inet.h>
#include <pthread.h>
#endif


// if GAS_WITH_TASKS defined, client callback can be enqueued in a separate thread.
// a little bit slower but can be useful for heavy jobs (epoll and kqueues)
// code have not been adapted for WIN32 and anyway not useful for threads or iocp
#ifndef WIN32
#define GAS_WITH_TASKS
#endif


// constants
#define GAS_LISTEN_QUEUE_SIZE 128		// limit on Darwin and Linux = 128
#define GAS_EVENTS_QUEUE_SIZE 256		// initial size. will stay 128 on linux. Can grow on Darwin

#define GAS_TP_THREADS 1
#define GAS_TP_POLLER  2

// common definitions

#define GAS_FALSE 0
#define GAS_TRUE  1
#define GAS_DEFAULT -1

// OS specific definitions

#ifdef WIN32

typedef int      gas_socklen_t;
typedef unsigned gas_socket_t;		// HANDLE
typedef void *   gas_thread_t;		// HANDLE
typedef unsigned gas_callback_t;

#define gas_mutex_t CRITICAL_SECTION

#define gas_mutex_init(lock)    InitializeCriticalSection(lock)
#define gas_mutex_destroy(lock) DeleteCriticalSection(lock)
#define gas_mutex_lock(lock)    EnterCriticalSection(lock)
#define gas_mutex_unlock(lock)  LeaveCriticalSection(lock)

#define sleep(t) Sleep(t*1000)

#else  // WIN32

typedef socklen_t gas_socklen_t;
typedef int       gas_socket_t;
typedef pthread_t gas_thread_t;
typedef void *    gas_callback_t;

#define __stdcall
#define __cdecl

#define gas_mutex_t pthread_mutex_t

#define gas_mutex_init(lock)    pthread_mutex_init(lock,NULL)
#define gas_mutex_destroy(lock) pthread_mutex_destroy(lock)
#define gas_mutex_lock(lock)    pthread_mutex_lock (lock)
#define gas_mutex_unlock(lock)  pthread_mutex_unlock (lock)

#define FAR

#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1

#endif // WIN32

#endif // _GAS_DEFINITIONS_H_


#ifndef _ECHOTEST_H_
#define _ECHOTEST_H_


#define PORT "8080" // the port client will be connecting to

#define MAXDATASIZE (100) // max number of bytes we can get at once

#if !defined(BIND_SOURCE_PORT)
# define BIND_SOURCE_PORT (0)
#endif

#if !defined(SERVER_CLOSE)
# define SERVER_CLOSE (0)
#endif

#define CLOCK_MONOTONIC 1

#ifdef __linux__
#if !defined __timespec_defined
struct timespec {
	__time_t tv_sec;                 /* seconds */
	long int tv_nsec;                /* nanoseconds */
};
#endif	// __timespec_defined
#endif // __linux__

#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
#ifndef _STRUCT_TIMESPEC
#define _STRUCT_TIMESPEC struct timespec
_STRUCT_TIMESPEC {
	__darwin_time_t tv_sec;
	long            tv_nsec;
};
#endif // _STRUCT_TIMESPEC
#endif // __MACH__

#ifdef WIN32
#if !defined __timespec_defined && !defined __struct_timespec_defined
struct timespec {
	long int tv_sec;                 /* seconds */
	long int tv_nsec;                /* nanoseconds */
};
#endif	// __timespec_defined
#endif // WIN32


#ifdef __cplusplus
extern "C" {
#endif

int main(int argc, char *argv[]);
int   gas_create_thread (gas_thread_t *pthread, __stdcall gas_callback_t (*worker_thread)(void *data), void *info);
void  gas_wait_thread  (gas_thread_t idthread);
void  prepare(int sock);
void* do_connect(struct addrinfo *servinfo);
struct addrinfo* getaddr();
void  show_restimes();
void  show_restime_res(int start, int stop, int step);

#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
int clock_gettime(int clk_id, struct timespec *ts);
#endif // __MACH__

#ifdef WIN32

int
clock_gettime(int clk_id, struct timespec *ta);
LARGE_INTEGER getFILETIMEoffset();
#endif	// WIN32


#ifdef __cplusplus
}
#endif

#endif /* _ECHOTEST_H_ */

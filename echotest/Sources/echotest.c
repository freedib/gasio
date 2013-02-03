
// exemple: ./echotest -c 20 -n 10 -h 100 xp

/** Echo client. */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
//#include <sys/types.h>
#ifdef WIN32
#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#endif
#include <time.h>

#include "GASdefinitions.h"
#include "echotest.h"


int g_nloop;
int g_nhello;
int g_noverwrap;
int g_resolve;
int success;
long g_restimes[1000001];

gas_mutex_t g_mutex;

const char *host;
const char *port;


int main(int argc, char *argv[])
{
    struct addrinfo *servinfo;
    int rv;
    int opt;

    int verbose = 0;
    int nthread = 2;
    g_nloop = 1;
    g_nhello = 8000;
    g_noverwrap = 1;

    port = PORT;
    host = NULL;

    while (-1 != (opt = getopt(argc, argv, "n:h:c:p:o:vg"))) {
        switch (opt) {
        case 'n':
            g_nloop = atoi(optarg);
            break;
        case 'h':
            g_nhello = atoi(optarg);
            break;
        case 'c':
            nthread = atoi(optarg);
            break;
        case 'p':
            port = optarg;
            break;
        case 'o':
            g_noverwrap = atoi(optarg);
            break;
        case 'v':
            verbose = 1;
            break;
        case 'g':
            g_resolve = 1;
            break;
        default:
            fprintf(stderr, "Unknown option: %c\n", opt);
            return 1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "usage: client [-vg] [-n connect count] [-h hellos per connec] [-c threads] [-p port] hostname\n");
        return 2;
    }
    host = argv[optind];

#ifdef	WIN32
	// init the winsock libraries
	WSADATA wsaData;
	if ( WSAStartup(MAKEWORD(2,2), &wsaData) != 0 ) {
		fprintf(stderr, "Unable to initialize WinSock 1.1\n");
		return 4;
	}
#endif	// WIN32

    servinfo = NULL;
    if (!g_resolve) {
        servinfo = getaddr();
        if (servinfo == NULL) {
            fprintf(stderr, "Can't resolve %s:%s\n", host, port);
            return 3;
        }
    }

    gas_mutex_init(&g_mutex);

    gas_mutex_lock(&g_mutex);
    long long time_consumed;
    {
        struct timespec t1, t2;
        gas_thread_t *threads = malloc(sizeof(gas_thread_t)*nthread);
        int i;
        for (i = 0; i < nthread; ++i) {
            rv = gas_create_thread (&threads[i], (void*)do_connect, (void*)servinfo);
            if (rv == -1) {
                perror("Failed to create thread");
                return 3;
            }
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        gas_mutex_unlock(&g_mutex);

        for (i = 0; i < nthread; ++i) {
            gas_wait_thread(threads[i]);
        }
        clock_gettime(CLOCK_MONOTONIC, &t2);
        time_consumed  = t2.tv_sec * 1000000000LL + t2.tv_nsec;
        time_consumed -= t1.tv_sec * 1000000000LL + t1.tv_nsec;
        free(threads);
    }

    freeaddrinfo(servinfo); // all done with this structure
    if (verbose)
        show_restimes();

    printf("Throughput: %.2lf [#/sec]\n",
            (success)*1000000000.0/(time_consumed));

    return 0;
}

#ifdef WIN32
int
gas_create_thread (gas_thread_t *pidthread, __stdcall gas_callback_t (*worker_thread)(void *data), void *info)
{
	unsigned __attribute__((unused)) dwThreadId = 0;
	*pidthread = (HANDLE)_beginthreadex( NULL, 0, worker_thread, info, 0, &dwThreadId);
	if (*pidthread<0)
		return GAS_FALSE;
	return GAS_TRUE;
}
#else  // WIN32
int
gas_create_thread (gas_thread_t *pidthread, gas_callback_t (*worker_thread)(void *data), void *info)
{
	int rc;
	rc = pthread_create (pidthread, NULL, worker_thread, info);
	if (rc != 0)
		return GAS_FALSE;
	return GAS_TRUE;
}
#endif  // WIN32

void
gas_wait_thread (gas_thread_t idthread)
{
#ifdef WIN32
	WaitForSingleObject (idthread,INFINITE);
#else
	pthread_join (idthread, NULL);
#endif // WIN32
}

void prepare(int sock)
{
    //int yes=1;

    //setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

#if BIND_SOURCE_PORT
    {
        struct sockaddr_in sin;
        memset(&sin, 0, sizeof sin);
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
        sin.sin_port = htons(20000);
        bind(sock, (struct sockaddr*)&sin, sizeof(sin));
    }
#endif
}

void* do_connect(struct addrinfo *servinfo)
{
    struct addrinfo *p, *pinfo;
    int sockfd, numbytes;  
    char buf[MAXDATASIZE];
    int *socks = NULL;
    int i, j, k;
    struct timespec t1, t2;

    sleep(1);

    socks = malloc(sizeof(int)*g_noverwrap);
    memset(socks, 0, sizeof(int)*g_noverwrap);

    gas_mutex_lock(&g_mutex);
    gas_mutex_unlock(&g_mutex);
    success = 0;
    for (i = 0; i < g_nloop; ++i) {
        // loop through all the results and connect to the first we can
        if (servinfo) {
            pinfo = servinfo;
        } else {
            pinfo = getaddr();
        }
        p = pinfo;
        k = 0;
        for (k=0; k<g_noverwrap; ++k) {
            for (; p != NULL; p = p->ai_next) {
                if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                    perror("client: socket");
                    continue;
                }
                prepare(sockfd);
                if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                    close(sockfd);
                    perror("client: connect");
                    continue;
                }
                socks[k] = sockfd;
                break;
            }
            if (!p) break;
        }

        if (p == NULL) {
            continue;
        }
        for (j=0; j<g_nhello; ++j) {
        clock_gettime(CLOCK_MONOTONIC, &t1);

            for (k=0; k<g_noverwrap; ++k) {
                sockfd = socks[k];
                send(sockfd, "hello\n", 6, 0);
            }
            for (k=0; k<g_noverwrap; ++k) {
                sockfd = socks[k];
                if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) < 0) {
                    perror("recv");
                    close(sockfd);
                    goto exit;
                }
                if (numbytes != 6) {
                    printf("Recieved %d bytes\n", numbytes);
                    goto exit;
                }
                __sync_fetch_and_add(&success, 1);
            }
            clock_gettime(CLOCK_MONOTONIC, &t2);
            long long t = t2.tv_sec * 1000000000LL + t2.tv_nsec;
            t          -= t1.tv_sec * 1000000000LL + t1.tv_nsec;
            t /= 10000; // ns => 10us
            if (t > 1000000) t=1000000;
            __sync_fetch_and_add(g_restimes+t, 1);
        }

#if SERVER_CLOSE
        do {
            numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0);
        } while (numbytes > 0);
#endif

        for (k=0; k<g_noverwrap; ++k) {
            close(socks[k]);
        }
        if (!servinfo) {
            freeaddrinfo(pinfo);
        }
    }
exit:
    free(socks);
    return NULL;
}

struct addrinfo* getaddr()
{
    struct addrinfo hints, *servinfo;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return NULL;
    }

    return servinfo;
}

void show_restimes()
{
    show_restime_res(0, 10, 1);
    show_restime_res(10, 100, 10);
    show_restime_res(100, 1000, 100);
    show_restime_res(1000, 10000, 1000);
    show_restime_res(10000, 100000, 10000);
    show_restime_res(100000, 1000000, 100000);
    printf(" >= 10sec: %ld\n", g_restimes[1000000]);
}

void show_restime_res(int start, int stop, int step)
{
	int i, j;
    for (i = start; i < stop; i += step) {
        long sum = 0;
        for (j = 0; j < step; ++j) sum += g_restimes[i+j];
        if (sum > 0) {
            if (start < 99) {
                printf(" <%5d [us]: %ld\n", (i+step)*10, sum);
            } else {
                printf(" <%5d [ms]: %ld\n", (i+step)/100, sum);
            }
        }
    }
}


#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time

#include <sys/time.h>
#include <mach/clock.h>
#include <mach/mach.h>

int clock_gettime(int clk_id, struct timespec *ts) {
	clock_serv_t cclock;
	mach_timespec_t mts;
	host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
	clock_get_time(cclock, &mts);
	mach_port_deallocate(mach_task_self(), cclock);
	ts->tv_sec = mts.tv_sec;
	ts->tv_nsec = mts.tv_nsec;
	return 0;
}

#endif // __MACH__



#ifdef WIN32

int
clock_gettime(int clk_id, struct timespec *ts)
{
    LARGE_INTEGER           t;
    FILETIME                f;
    double                  microseconds;
    static LARGE_INTEGER    offset;
    static double           frequencyToMicroseconds;
    static int              initialized = 0;
    static BOOL             usePerformanceCounter = 0;

    if (!initialized) {
        LARGE_INTEGER performanceFrequency;
        initialized = 1;
        usePerformanceCounter = QueryPerformanceFrequency(&performanceFrequency);
        if (usePerformanceCounter) {
            QueryPerformanceCounter(&offset);
            frequencyToMicroseconds = (double)performanceFrequency.QuadPart / 1000000.;
        } else {
            offset = getFILETIMEoffset();
            frequencyToMicroseconds = 10.;
        }
    }
    if (usePerformanceCounter) QueryPerformanceCounter(&t);
    else {
        GetSystemTimeAsFileTime(&f);
        t.QuadPart = f.dwHighDateTime;
        t.QuadPart <<= 32;
        t.QuadPart |= f.dwLowDateTime;
    }

    t.QuadPart -= offset.QuadPart;
    microseconds = (double)t.QuadPart / frequencyToMicroseconds;
    t.QuadPart = microseconds;
    ts->tv_sec = t.QuadPart / 1000000;
    ts->tv_nsec = (t.QuadPart % 1000000) * 1000;
    return (0);
}

LARGE_INTEGER
getFILETIMEoffset()
{
    SYSTEMTIME s;
    FILETIME f;
    LARGE_INTEGER t;

    s.wYear = 1970;
    s.wMonth = 1;
    s.wDay = 1;
    s.wHour = 0;
    s.wMinute = 0;
    s.wSecond = 0;
    s.wMilliseconds = 0;
    SystemTimeToFileTime(&s, &f);
    t.QuadPart = f.dwHighDateTime;
    t.QuadPart <<= 32;
    t.QuadPart |= f.dwLowDateTime;
    return (t);
}

#endif	// WIN32



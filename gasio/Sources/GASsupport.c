
#include <stdio.h>			// fprintf, vfprintf
#include <stdarg.h>			// va_start, va_end
#ifdef WIN32
#include <windows.h>
#include <process.h>
#else  // WIN32
#include <unistd.h>			// sysconf
#endif // WIN32
#include <sys/time.h>		// gettimeofday

#include "GASdefinitions.h"
#include "GASsupport.h"


static int visualize  = 0;
static int debugLevel = 0;
static FILE *fdlog = NULL;


////////////////  ERROR MESSAGES  ////////////////

void
gas_set_debug_level (int _visualize, int _debugLevel, FILE *_fdlog)
{
	visualize  = _visualize;
	debugLevel = _debugLevel;
	fdlog = _fdlog;
}

int
gas_error_message (const char *format, ...)
{
	va_list args;

	va_start (args, format);
	vfprintf (stderr, format, args);
	va_end (args);
	return GAS_ERROR;			// arg for exit on fatal errors

	// Consider __LINE__: fprintf (stderr, "Error %d in CreateIoCompletionPort in line %d\n", err, __LINE__));
}


int
gas_debug_message (int level, const char *format, ...)
{
	va_list args;
	va_start (args, format);
	if (level<=visualize) {
		vprintf (format, args);
		fflush (stdout);
	}
	va_end (args);
	va_start (args, format);
	if (level<=debugLevel && fdlog != NULL) {
		vfprintf (fdlog, format, args);
		fflush (fdlog);
	}
	va_end (args);
	return 0;
}


////////////////  STATISTICS  ////////////////


static volatile unsigned long packet_count = 0;
static struct timeval present_time, previous_time;

void gas_reset_stats () {
	packet_count = 0;
	gettimeofday(&previous_time, NULL);
}
void gas_adjust_stats () {
	atomic_add_ulong(&packet_count, 1);
}
void gas_compute_stats () {
	unsigned long count = atomic_add_ulong(&packet_count, 0);
	if (count > 10000) {
	    count = atomic_and_ulong(&packet_count, 0);
		gettimeofday(&present_time, NULL);

		long long present_time_ms  = ((long long)present_time.tv_sec)*1000  + present_time.tv_usec/1000;
		long long previous_time_ms = ((long long)previous_time.tv_sec)*1000 + previous_time.tv_usec/1000;
		long long delta_time_ms = present_time_ms - previous_time_ms;
		fprintf(stderr, "%ld reqs per sec\n", (long)(count*1000/delta_time_ms));
		previous_time = present_time;
	}
}


////////////////  PROCESSOR COUNT  ////////////////


#ifdef WIN32

int
gas_get_processors_count ()
{
	SYSTEM_INFO systemInfo;
	GetSystemInfo( &systemInfo );
	int numCPU = systemInfo.dwNumberOfProcessors;
	return numCPU;
}

#else  // WIN32

int
gas_get_processors_count ()
{
	int numCPU = sysconf (_SC_NPROCESSORS_CONF);
	return numCPU;
}

#endif // WIN32


////////////////  ATOMIC OPERATIONS  ////////////////


#ifdef __ARM__
// implementation of __sync_fetch_and_add_4 for ARM processors
unsigned int
add_uint (unsigned volatile int *var, int val)
{
	unsigned volatile int v = *var;
	*var = val;
	return v;
}
unsigned long
add_ulong (unsigned volatile long *var, int val)
{
	unsigned volatile long v = *var;
	*var = val;
	return v;
}

// implementation of __sync_fetch_and_add_4 for ARM processors
unsigned long
and_ulong (unsigned volatile long *var, int val)
{
	unsigned volatile long v = *var;
	*var &= val;
	return v;
}
#endif	// __ARM__

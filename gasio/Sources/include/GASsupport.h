
#ifndef _GAS_SUPPORT_H_
#define _GAS_SUPPORT_H_


#include <stdio.h>

#define GAS_NONE     0
#define GAS_IO       1
#define GAS_CLIENT   2
#define GAS_DETAIL   3
#define GAS_ALWAYS   4


#ifdef __cplusplus
extern "C" {
#endif

void gas_set_debug_level  ( int _visualize, int _debugLevel, FILE *_fdlog );
int  gas_error_message    ( const char *fmt, ... );
int  gas_debug_message    ( int level, const char *fmt, ... );

void gas_reset_stats   ();
void gas_adjust_stats  ();
void gas_compute_stats ();

int  gas_get_processors_count ();

#if defined(__ARM__) || defined(__X86__)
unsigned int  add_uint  (unsigned volatile int  *var, int val);
unsigned long add_ulong (unsigned volatile long *var, int val);
unsigned long and_ulong (unsigned volatile long *var, int val);
#endif

#ifdef __cplusplus
}
#endif


#endif // _GAS_SUPPORT_H_

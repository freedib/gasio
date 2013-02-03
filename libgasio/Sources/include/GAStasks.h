
////////////////  TASKS SUPPORT : ALLOW DEFFERING JOBS FOR EPOLL AND KQUEUE  ////////////////

#ifndef _GAS_TASKS_H_
#define _GAS_TASKS_H_


#include "GASsockets.h"
#include "GASclient.h"


#ifdef GAS_WITH_TASKS

#define GAS_BLOCK_SIZE 128

typedef struct {
	gas_client_info * ci;
} gas_task_info;

typedef struct {
	pthread_mutex_t lock;
	pthread_cond_t	cond;
	int must_Exit;
	gas_thread_t * threads;
	int max_threads;
	int first_free_thread;
	gas_task_info * tasks;
	int max_tasks;
	int first_free_task;
	int first_task;
} gas_tasks_data;


#ifdef __cplusplus
extern "C" {
#endif

void gas_init_tasks ();
void gas_clean_tasks ();
void gas_push_thread (gas_thread_t thread);
void gas_delete_thread (gas_thread_t thread);
int  gas_get_running_task_threads ();
int  gas_start_one_task_thread ();
void gas_stop_one_task_thread ();
int  gas_push_task (gas_client_info *ci);
gas_client_info *gas_pop_task ();
int	 gas_enqueue_task (gas_client_info *ci);
__stdcall gas_callback_t gas_tasks_thread (void *data);

void gas_test_tasks ();

#ifdef __cplusplus
}
#endif


#else	// GAS_WITH_TASKS

int	 gas_enqueue_task (gas_client_info *ci);

#endif	// GAS_WITH_TASKS

#endif  // _GAS_TASKS_H_

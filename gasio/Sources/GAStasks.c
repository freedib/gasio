
#include "GAStasks.h"


#ifdef GAS_WITH_TASKS


#include <stdlib.h>		// calloc, free, realloc
#include <string.h>		// memmove, memset
#include <unistd.h>		// sleep
#include <stdio.h>		// fprintf

#include "GASsupport.h"
#include "GASsockets.h"
#include "GASthreads.h"


static gas_tasks_data * td;

void
gas_init_tasks ()
{
	if (td != NULL)
		return;
	td = (gas_tasks_data *) calloc (1, sizeof(gas_tasks_data));
	td->threads = (gas_thread_t *) calloc(GAS_BLOCK_SIZE, sizeof(gas_thread_t));
	td->max_threads = 0;
	td->first_free_thread = 0;
	td->tasks = (gas_task_info *) calloc(GAS_BLOCK_SIZE, sizeof(gas_task_info));
	td->max_tasks = 0;
	td->first_free_task = 0;
	td->first_task = 0;
	pthread_mutex_init(&td->lock,NULL);
	pthread_cond_init(&td->cond,NULL);
	td->must_Exit = 0;
}

void
gas_clean_tasks ()
{
	int it;
	td->must_Exit = 1;
	pthread_cond_broadcast(&td->cond);
    for (it=0; it < td->first_free_thread; it++) {
    	if (td->threads[it]!=(gas_thread_t)0)
    		pthread_join(td->threads[it], NULL);
    	td->threads[it] = (gas_thread_t)0;
    }
    if (td)
    	free(td);
    td = NULL;
}

void
gas_push_thread (gas_thread_t thread)
{
	if (td->first_free_thread >= td->max_threads) {
		td->max_threads += GAS_BLOCK_SIZE;
		if (realloc ((void *)td->threads, td->max_threads) == NULL)
			gas_error_message("No memory (realloc)\n");

	}
	td->threads[td->first_free_thread++] = thread;
}

// remove a thread from the list
void
gas_delete_thread (gas_thread_t thread)
{
	int it;
    for (it=0; it < td->first_free_thread; it++) {
    	if (td->threads[it]==thread) {
    		int nthreads = td->first_free_thread - it - 1;
    		if (nthreads > 0)
    			memmove (&td->threads[it], &td->threads[it+1], nthreads*sizeof(gas_thread_t));
    		--td->first_free_thread;
    		break;
    	}
     }
}

int
gas_get_running_task_threads()
{
	return td? td->first_free_thread: 0;
}

int
gas_start_one_task_thread()
{
	if (td==NULL)
		gas_init_tasks ();
	gas_thread_t pid;
	int ret = pthread_create(&pid, 0, gas_tasks_thread, td);
	gas_push_thread(pid);
	return ret;
}

void
gas_stop_one_task_thread()
{
	fprintf (stderr, "gas_stop_one_task_thread\n");
	// push an empty task to signal the end
	if (gas_get_running_task_threads() > 0)
		gas_enqueue_task (NULL);
}

// return -1 if error or number of tasks pushed
int
gas_push_task (gas_client_info *ci)
{
	if (td->first_free_task >= td->max_tasks) {
		td->max_tasks += GAS_BLOCK_SIZE;
		if (realloc ((void *)td->tasks, td->max_tasks) == NULL) {
			gas_error_message("No memory (realloc)\n");
			return -1;
		}
	}
	td->tasks[td->first_free_task++].ci = ci;
	return td->first_free_task - td->first_task;
}

gas_client_info *
gas_pop_task ()
{
	gas_client_info *ci = NULL;
	if (td->first_task >= 0)
		ci = td->tasks[td->first_task++].ci;

	if (td->first_task >= td->first_free_task)
		td->first_task = td->first_free_task = 0;		// queue empty; reset pointers
	else if (td->first_task >= td->max_tasks/2) {		// half empty; shift data at the beginning
		int ntasks = td->first_free_task-td->first_task;
		memmove (&td->tasks[0], &td->tasks[td->first_task], ntasks*sizeof(gas_task_info));
		td->first_free_task -= td->first_task;
		td->first_task = 0;
	}
	return ci;
}

// return -1 if error or number of tasks enqueued
int
gas_enqueue_task (gas_client_info *ci)
{
	int ntasks;
	GAS_THREADS_INFO *ti = (GAS_THREADS_INFO *)ci->tpi;
	if (!ti->allow_tasks)
		return -1;
	gas_debug_message (GAS_IO, "Task push\n");

	if (gas_get_running_task_threads() < 1)
		gas_start_one_task_thread();
	pthread_mutex_lock(&td->lock);
	ntasks = gas_push_task(ci);
	pthread_mutex_unlock(&td->lock);
	if (ntasks < 0)
		return -1;
	pthread_cond_signal(&td->cond);
	return ntasks;
}

__stdcall gas_callback_t
gas_tasks_thread (void *data)			// data == td
{
	gas_client_info *ci;

	while (!td->must_Exit) {
		pthread_mutex_lock(&td->lock);
		if (td->first_free_task <= 0) {
			while (td->first_free_task==0 && !td->must_Exit)
				pthread_cond_wait(&td->cond, &td->lock);
			if (td->must_Exit) {
				pthread_mutex_unlock(&td->lock);
				break;
			}
		}
		ci = gas_pop_task ();
		if (ci == NULL)					// request to stop thread
			gas_delete_thread (pthread_self());
		pthread_mutex_unlock(&td->lock);
		if (ci == NULL) {				// request to stop thread
			gas_delete_thread (pthread_self());
			pthread_detach(pthread_self());
			break;
		}

		gas_debug_message (GAS_IO, "Task pop\n");
		if (ci->operation == GAS_OP_READ)
			gas_do_callback (ci, GAS_CLIENT_DEFFERED_READ);
		else if (ci->operation == GAS_OP_WRITE)
			gas_do_callback (ci, GAS_CLIENT_DEFFERED_WRITE);
	}
	return (gas_callback_t) 0;
}

// tasks debugging
#if 0

void test_callback (gas_client_info *ci, int can_defer) {
	gas_client_info *ci = (gas_client_info *)ci;
	fprintf (stderr, "client_call_back %d\n", ci->socket);
	sleep (1);
	fprintf (stderr, "client_call_back exit %d\n", ci->socket);
	fprintf (stderr, "gas_get_running_task_threads=%d\n", gas_get_running_task_threads());
}

void gas_test_tasks () {
	static GAS_THREADS_INFO ti;
	static gas_client_info clients[5];
	int it;
	gas_init_tasks ();
	gas_start_one_task_thread();
	gas_start_one_task_thread();
	for (it=0; it<sizeof(clients)/sizeof(gas_client_info); it++) {
		memset (&clients[it], '\0', sizeof(gas_client_info));
		clients[it].tpi = &ti;
		ti.callback = test_callback;
		clients[it].socket = it+1;
		gas_enqueue_task (&clients[it]);
	}
	gas_enqueue_task (NULL);
	for (it=0; it<sizeof(clients)/sizeof(gas_client_info); it++) {
		gas_enqueue_task (&clients[it]);
	}
//	gas_clean_tasks ();		// too early
}

#endif


#else	// GAS_WITH_TASKS


int
gas_enqueue_task (gas_client_info *ci)
{
	return -1;
}


#endif	// GAS_WITH_TASKS

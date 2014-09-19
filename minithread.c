/*
* minithread.c:
*      This file provides a few function headers for the procedures that
*      you are required to implement for the minithread assignment.
*
*      EXCEPT WHERE NOTED YOUR IMPLEMENTATION MUST CONFORM TO THE
*      NAMING AND TYPING OF THESE PROCEDURES.
*
*/
#include <stdlib.h>
#include <stdio.h>
#include "minithread.h"
#include "queue.h"
#include "synch.h"

#include <assert.h>

/*
* A minithread should be defined either in this file or in a private
* header file.  Minithreads have a stack pointer with to make procedure
* calls, a stackbase which points to the bottom of the procedure
* call stack, the ability to be enqueueed and dequeued, and any other state
* that you feel they must have.
*/

typedef enum {READY, WAITING, RUNNING, FINISHED} state_t;


typedef struct minithread {
	int pid;
	stack_pointer_t sp;
	stack_pointer_t stackbase;
	stack_pointer_t stacktop;
	state_t state;
} minithread;

//Current running thread.
minithread_t current_thread = NULL;

/*
	Scheduler definition.
*/
typedef struct scheduler {
	queue_t ready_queue;
	queue_t blocked_queue;
	queue_t finished_queue;
} scheduler;
typedef struct scheduler *scheduler_t;

scheduler_t thread_scheduler;

/*
	Scheduler API.

*/

void scheduler_init(scheduler_t *scheduler_ptr){
	
	scheduler_t s;

	*scheduler_ptr = (scheduler_t) malloc(sizeof(scheduler_t));
	s = *scheduler_ptr;
	s->ready_queue = queue_new();
	s->blocked_queue = queue_new();
	s->finished_queue = queue_new();
}

/*
* Scheduler method that makes the context switch. It adds the current TCB to the appropriate queue,
* depending on its state and then dequeues the next TCB, switching to it. It also busy waits for new threads
* to become ready, releasing them from the blocked queue if necessary.
*/
void scheduler_switch(scheduler_t scheduler){

	minithread_t thread_to_run;

	do{
		int deq_result = queue_dequeue(scheduler->ready_queue, (void **) &thread_to_run);
		if(deq_result == 0){
			stack_pointer_t *oldsp_ptr; 

			//Check if we're scheduling for the first time.
			if(current_thread == NULL){
				//Assign a dummy stack_pointer_t for the first context switch.
				oldsp_ptr = (stack_pointer_t) malloc(sizeof(stack_pointer_t));
			} else {
				oldsp_ptr = &(current_thread->sp);

				if(current_thread->state == FINISHED){
					queue_append(scheduler->finished_queue, current_thread);
				} else if(current_thread->state == WAITING){
					queue_append(scheduler->blocked_queue, current_thread);
				} else{
					current_thread->state = READY;
					queue_append(scheduler->ready_queue, current_thread);
				}
			}

			thread_to_run->state = RUNNING;
			current_thread = thread_to_run;

			minithread_switch(oldsp_ptr, &(current_thread->sp));
		} else {
			//Check if the first blocked thread can be made runnable again.
			minithread_t first_blocked_thread;
			if(queue_dequeue(scheduler->blocked_queue, (void **) &first_blocked_thread) == 0){
				if(first_blocked_thread->state == READY){
					queue_append(scheduler->ready_queue, first_blocked_thread);
				} else {
					queue_prepend(scheduler->blocked_queue, first_blocked_thread);
				}
			}
		}
	} while(1);
	
}

//Thread responsible for freeing up the zombie threads.
int vaccum_cleaner(int *arg){
	int clean_cycle = 100;
	while(1){
		minithread_t zombie_thread;
		if(queue_dequeue(thread_scheduler->finished_queue, (void **) &zombie_thread) == 0){
			int i;
			for(i = 0 ; i < clean_cycle; i++);
			minithread_free(zombie_thread);
		}
		minithread_yield();
	}

}



/* Cleanup function pointer. */
int cleanup_proc(arg_t arg){
	current_thread->state = FINISHED;
	scheduler_switch(thread_scheduler);

	//Shouldn't happen.
	return -1;
}

//Static id counter to number threads.
static int id_counter = 0;

/* minithread functions */

minithread_t
minithread_fork(proc_t proc, arg_t arg) {
	minithread_t forked_thread; 
	forked_thread = minithread_create(proc, arg);
	queue_append(thread_scheduler->ready_queue, forked_thread);

    return forked_thread;
}

minithread_t
minithread_create(proc_t proc, arg_t arg) {
	minithread_t thread = (minithread_t) malloc(sizeof(minithread));
	thread->pid = id_counter++;
	thread->state = READY;

	minithread_allocate_stack(&(thread->stackbase), &(thread->stacktop));
	minithread_initialize_stack(&(thread->stacktop), 
								proc,
								arg,
								&cleanup_proc,
								NULL);

	thread->sp = thread->stacktop;

    return thread;
}

minithread_t
minithread_self() {
    return current_thread;
}

int
minithread_id() {
    return current_thread->pid;
}

void
minithread_stop() {
	current_thread->state = WAITING;
	scheduler_switch(thread_scheduler);
}

void
minithread_start(minithread_t t) {
	if(t->state == READY  || t->state == RUNNING) return;
	t->state = READY;
}

void
minithread_yield() {
	scheduler_switch(thread_scheduler);
}

void minithread_free(minithread_t t){
	minithread_free_stack(t->stackbase);
	free(t);
}

/*
 * Initialization.
 *
 *      minithread_system_initialize:
 *       This procedure should be called from your C main procedure
 *       to turn a single threaded UNIX process into a multithreaded
 *       program.
 *
 *       Initialize any private data structures.
 *       Create the idle thread.
 *       Fork the thread which should call mainproc(mainarg)
 *       Start scheduling.
 *
 */
void
minithread_system_initialize(proc_t mainproc, arg_t mainarg) {

	//Allocate the scheduler's queues.
	scheduler_init(&thread_scheduler);

	//Fork the thread containing the main function.
	minithread_fork(mainproc, mainarg);

	//Fork the vaccum cleaner thread.
	minithread_fork(&vaccum_cleaner, NULL);

	//Start concurrency.
	scheduler_switch(thread_scheduler);

}






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
#include "interrupts.h"
#include "minithread.h"
#include "queue.h"
#include "synch.h"
#include "alarm.h"

#include <assert.h>

static long current_tick = 0;


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

//Semaphore for cleaning up only when needed.
semaphore_t cleanup_sema = NULL;

//Semaphore for thread scheduling.
semaphore_t thread_arrived_sema = NULL;


/*
	Scheduler definition.
*/
typedef struct scheduler {
	queue_t ready_queue;
	queue_t finished_queue;
} scheduler;
typedef struct scheduler *scheduler_t;

scheduler_t thread_scheduler;

/*
	Scheduler API.

*/

void scheduler_init(scheduler_t *scheduler_ptr){
	
	scheduler_t s;

	*scheduler_ptr = (scheduler_t) malloc(sizeof(struct scheduler));
	s = *scheduler_ptr;
	s->ready_queue = queue_new();
	s->finished_queue = queue_new();

	cleanup_sema 		= semaphore_create();
	semaphore_initialize(cleanup_sema, 0);

	thread_arrived_sema = semaphore_create();
	semaphore_initialize(thread_arrived_sema, 0);
}


/*
* Try to context switch only once, return 1 (success) if it found a valid TCB to switch to, otherwise 0 (failure).
*/
int scheduler_switch_try_once(scheduler_t scheduler){
	minithread_t thread_to_run;	

	interrupt_level_t old_level;
	int deq_result;

	//Scheduler cannot be interrupted while it's trying to decide.
	old_level = set_interrupt_level(DISABLED);

	deq_result = queue_dequeue(scheduler->ready_queue, (void **) &thread_to_run);

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
			} else if(current_thread->state == RUNNING || current_thread->state == READY){
				current_thread->state = READY;
				queue_append(scheduler->ready_queue, current_thread);
			}
		}

		thread_to_run->state = RUNNING;
		current_thread = thread_to_run;

		minithread_switch(oldsp_ptr, &(current_thread->sp));
		return 1;
	}

	set_interrupt_level(old_level);
	return 0;
}


/*
* Scheduler method that makes the context switch. It adds the current TCB to the appropriate queue,
* depending on its state and then dequeues the next TCB, switching to it. It also busy waits for new threads
* to become ready, releasing them from the blocked queue if necessary.
*/
void scheduler_switch(scheduler_t scheduler){

	interrupt_level_t old_level;
	int switch_result;

	//Scheduler cannot be interrupted while it's trying to decide.
	old_level = set_interrupt_level(DISABLED);

	switch_result = scheduler_switch_try_once(scheduler);
	if(switch_result){
		set_interrupt_level(old_level);
		return;
	}

	//If the current thread isn't finished yet and has yielded, allow it to proceed.
	if(current_thread->state == RUNNING){
			set_interrupt_level(old_level);
			return;
	}

	//There are no threads to be run and the current_thread cannot continue. Reenable interrupts and busy wait.
	set_interrupt_level(old_level);
	while(current_thread->state != RUNNING && current_thread != READY);
	
}

/*
 * minithread_free()
 *  Frees the resources associated to t (stack and TCB). 
 */
void minithread_free(minithread_t t);


//Thread responsible for freeing up the zombie threads.
int vaccum_cleaner(int *arg){
	while(1){
		interrupt_level_t old_level;
		minithread_t zombie_thread;

		semaphore_P(cleanup_sema);

		old_level = set_interrupt_level(DISABLED);

		if(queue_dequeue(thread_scheduler->finished_queue, (void **) &zombie_thread) == 0){
			minithread_free(zombie_thread);
		}
		set_interrupt_level(old_level);
	}

}


/* Cleanup function pointer. */
int cleanup_proc(arg_t arg){
	interrupt_level_t old_level = set_interrupt_level(DISABLED);	
	current_thread->state = FINISHED;

	//Tell the vaccum_cleaner there is a thread ready to be cleaned up.
	semaphore_V(cleanup_sema);

	set_interrupt_level(old_level);

	scheduler_switch(thread_scheduler);

	//Shouldn't happen.
	return -1;
}

//Static id counter to number threads.
static int id_counter = 0;

/* minithread functions */

minithread_t minithread_fork(proc_t proc, arg_t arg) {
	interrupt_level_t old_level;

	minithread_t forked_thread; 
	forked_thread = minithread_create(proc, arg);

	old_level = set_interrupt_level(DISABLED);
	queue_append(thread_scheduler->ready_queue, forked_thread);
	set_interrupt_level(old_level);


    return forked_thread;
}

minithread_t minithread_create(proc_t proc, arg_t arg) {

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

minithread_t minithread_self() {
    minithread_t self;
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    self = current_thread;
    set_interrupt_level(old_level);
    return self;
}

int minithread_id() {
    int pid;
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    pid = current_thread->pid;
    set_interrupt_level(old_level);
    return pid;
}

void minithread_stop() {
	interrupt_level_t old_level = set_interrupt_level(DISABLED);
	current_thread->state = WAITING;
	set_interrupt_level(old_level);

	scheduler_switch(thread_scheduler);
}

void minithread_start(minithread_t t) {
	interrupt_level_t old_level = set_interrupt_level(DISABLED);

	if(t->state == READY  || t->state == RUNNING) {
            set_interrupt_level(old_level);
            return;
    }
	t->state = READY;

	queue_append(thread_scheduler->ready_queue, t);
	set_interrupt_level(old_level);
}

void minithread_yield() {
	scheduler_switch(thread_scheduler);
}

void minithread_free(minithread_t t){
	minithread_free_stack(t->stackbase);
	free(t);
}

/*
 * This is the clock interrupt handling routine.
 * You have to call minithread_clock_init with this
 * function as parameter in minithread_system_initialize
 */
void 
clock_handler(void* arg)
{
	interrupt_level_t old_level = set_interrupt_level(DISABLED);
	alarm_id alarm = pop_alarm();
	while(alarm != NULL){
		execute_alarm(alarm);
		deregister_alarm(alarm);
		alarm = pop_alarm();
	}
	current_tick++;
	set_interrupt_level(old_level);

	scheduler_switch_try_once(thread_scheduler);
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
void minithread_system_initialize(proc_t mainproc, arg_t mainarg) {

	//Allocate the scheduler's queues.
	scheduler_init(&thread_scheduler);

	//Fork the thread containing the main function.
	minithread_fork(mainproc, mainarg);

	//Fork the vaccum cleaner thread.
	minithread_fork(&vaccum_cleaner, NULL);

	//Initialize alarm system for allowing threads to sleep.
	initialize_alarm_system(MINITHREAD_CLOCK_PERIOD, &current_tick);

	//Initialize clock system for preemption.
	minithread_clock_init(MINITHREAD_CLOCK_PERIOD, clock_handler);
	set_interrupt_level(ENABLED);

	//Start concurrency.
	scheduler_switch(thread_scheduler);
}

/*
 * sleep with timeout in milliseconds
 */

//A wrapper for minithread_start. 
void wrapper_minithread_start(void *arg){
	minithread_t t = (minithread_t) arg;
	minithread_start(t);
}

void semaphore_V_wrapper(void *semaphore_ptr) {
        semaphore_t semaphore = (semaphore_t) semaphore_ptr;
        semaphore_V(semaphore);
}

void 
minithread_sleep_with_timeout(int delay)
{
	semaphore_t sleep_sema = semaphore_create();

    semaphore_initialize(sleep_sema, 0);

	register_alarm(delay, semaphore_V_wrapper, sleep_sema);
	semaphore_P(sleep_sema);

	semaphore_destroy(sleep_sema);
}





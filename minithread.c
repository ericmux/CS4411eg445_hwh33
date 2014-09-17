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

typedef struct minithread {
	int pid;
	stack_pointer_t sp;
	stack_pointer_t stackbase;
	stack_pointer_t stacktop;
} minithread;

/* Cleanup function pointer. */
void cleanup_proc(arg_t arg){}

//Current running thread.
minithread_t current_thread;

//Idle thread.
minithread_t idle_thread;

//Thread control queue.
queue_t thread_queue;

//Static id counter to number threads.
static int id_counter = 0;


int idle_loop(arg_t arg){
	while(1){
		minithread_yield();
	}
}


/* minithread functions */

minithread_t
minithread_fork(proc_t proc, arg_t arg) {

	minithread_t forked_thread = minithread_create(proc, arg);

	queue_append(thread_queue, forked_thread);

    return forked_thread;
}

minithread_t
minithread_create(proc_t proc, arg_t arg) {
	minithread_t thread = (minithread_t) malloc(sizeof(minithread));
	thread->pid = id_counter++;

	minithread_allocate_stack(&(thread->stackbase), &(thread->stacktop));
	minithread_initialize_stack(&(thread->stacktop), 
								proc,
								arg,
								(proc_t) &cleanup_proc,
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
	minithread_t next_thread;

	queue_dequeue(thread_queue, &next_thread);

	minithread_switch(&(current_thread->sp), &(next_thread->sp));
	minithread_free_stack(current_thread->sp);

	current_thread = next_thread;
}

void
minithread_start(minithread_t t) {
	/* nooo idea... */

}

void
minithread_yield() {
	minithread_t next_thread;

	queue_dequeue(thread_queue, &next_thread);

	if(next_thread != NULL){
		minithread_switch(&(current_thread->sp), &(next_thread->sp));

		queue_append(thread_queue, current_thread);

		current_thread = next_thread;
	}
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

	minithread_t first_thread;

	thread_queue = queue_new(); 

	first_thread = minithread_create(mainproc, mainarg);
	queue_append(thread_queue, first_thread);

	idle_thread = minithread_create(&idle_loop, NULL);
	queue_append(thread_queue, idle_thread);

	queue_dequeue(thread_queue, &current_thread);

	minithread_start(current_thread);

}






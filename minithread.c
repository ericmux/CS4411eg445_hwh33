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

void cleanup_proc(arg_t arg){}

queue_t thread_queue;

static int id_counter = 0;


/* minithread functions */

minithread_t
minithread_fork(proc_t proc, arg_t arg) {
    return (minithread_t)0;
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
    return (minithread_t)0;
}

int
minithread_id() {
    return 0;
}

void
minithread_stop() {
}

void
minithread_start(minithread_t t) {
}

void
minithread_yield() {
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
}






#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "defs.h"
#include "synch.h"
#include "queue.h"
#include "minithread.h"

/*
 *      You must implement the procedures and types defined in this interface.
 */


/*
 * Semaphores.
 */
typedef struct semaphore {
    /* This is temporary so that the compiler does not error on an empty struct.
     * You should replace this with your own struct members.
     */
	queue_t queue;
} semaphore;


/*
 * semaphore_t semaphore_create()
 *      Allocate a new semaphore.
 */
semaphore_t semaphore_create() {
    	semaphore_t new_semaphore = (semaphore *)malloc(sizeof(semaphore));
	new_semaphore->queue = queue_new();
	
	return new_semaphore;
}

/*
 * semaphore_destroy(semaphore_t sem);
 *      Deallocate a semaphore.
 */
void semaphore_destroy(semaphore_t sem) {
	queue_free(sem->queue);
	free(sem);
}


/*
 * semaphore_initialize(semaphore_t sem, int cnt)
 *      initialize the semaphore data structure pointed at by
 *      sem with an initial value cnt.
 */
void semaphore_initialize(semaphore_t sem, int cnt) {
	int i;
	// The value of resource is unimportant;
	// it simply holds a place in the queue.
	int resource = 0; 
	for (i = 0; i < cnt; i++) {
		queue_append(sem->queue, &resource);
	}
}


/*
 * semaphore_P(semaphore_t sem)
 *      P on the sempahore.
 *      (a thread calling semaphore_P reflects a request for a resource)
 */
void semaphore_P(semaphore_t sem) {
	void **unused_pointer = (void **)malloc(sizeof(int *));
	// spin until a resource becomes available
	while (queue_dequeue(sem->queue, unused_pointer) == -1) {
		// TODO: need to sleep?	
	}
	free(unused_pointer);
}

/*
 * semaphore_V(semaphore_t sem)
 *      V on the sempahore.
 *      (a thread calling semaphore_V reflects the yield of a resource)
 */
void semaphore_V(semaphore_t sem) {
	// The value of resource is unimportant;
	// it simply holds a place in the queue.
	int resource = 0;
	queue_append(sem->queue, &resource);
}

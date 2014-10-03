#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "defs.h"
#include "synch.h"
#include "queue.h"
#include "minithread.h"
#include "interrupts.h"

/*
 *      You must implement the procedures and types defined in this interface.
 */


/*
 * Semaphores.
 */
typedef struct semaphore {
	queue_t waiting_q;
	tas_lock_t lock;
	// the destruction lock is used to prevent two threads from trying
	// to destroy the same semaphore
	tas_lock_t destruction_lock;
	int count;
} semaphore;


/*
 * semaphore_t semaphore_create()
 *      Allocate a new semaphore.
 */
semaphore_t semaphore_create() {
    semaphore_t new_semaphore = (semaphore *)malloc(sizeof(semaphore));
	new_semaphore->waiting_q = queue_new();
	atomic_clear(&new_semaphore->lock);
	atomic_clear(&new_semaphore->destruction_lock);
	
	return new_semaphore;
}

/*
 * semaphore_destroy(semaphore_t sem);
 *      Deallocate a semaphore.
 */
void semaphore_destroy(semaphore_t sem) {
	// First thing we do is disable interrupts to prevent the OS from
	// context switch while a thread is holding a semaphore lock. We
	// will re-enable at the end of this function.
	interrupt_level_t old_interrupt_level = set_interrupt_level(DISABLED);

	while(atomic_test_and_set(&sem->destruction_lock)) {
		return;
	}
	queue_free(sem->waiting_q);
	free(sem);

	set_interrupt_level(old_interrupt_level);
}


/*
 * semaphore_initialize(semaphore_t sem, int cnt)
 *      initialize the semaphore data structure pointed at by
 *      sem with an initial value cnt.
 */
void semaphore_initialize(semaphore_t sem, int cnt) {
	// First thing we do is disable interrupts to prevent the OS from
	// context switch while a thread is holding a semaphore lock. We
	// will re-enable at the end of this function.
	interrupt_level_t old_interrupt_level = set_interrupt_level(DISABLED);

	while (atomic_test_and_set(&(sem->lock))) {
		minithread_yield();
	}
	sem->count = cnt;
	atomic_clear(&(sem->lock));

	set_interrupt_level(old_interrupt_level);
}


/*
 * semaphore_P(semaphore_t sem)
 *      P on the sempahore.
 *      (a thread calling semaphore_P reflects a request for a resource)
 */
void semaphore_P(semaphore_t sem) {
	// First thing we do is disable interrupts to prevent the OS from
	// context switch while a thread is holding a semaphore lock. We
	// will re-enable at the end of this function.
	interrupt_level_t old_interrupt_level = set_interrupt_level(DISABLED);	

	while (atomic_test_and_set(&sem->lock)) {
		minithread_yield();
	}
	if (--sem->count < 0) {
		// Resources are not available. Thread should be added to
		// the waiting queue.
		queue_append(sem->waiting_q, minithread_self());
		atomic_clear(&sem->lock);
		minithread_stop();
	} else {
		atomic_clear(&sem->lock);
	}

	set_interrupt_level(old_interrupt_level);
}

/*
 * semaphore_V(semaphore_t sem)
 *      V on the sempahore.
 *      (a thread calling semaphore_V reflects the yield of a resource)
 */
void semaphore_V(semaphore_t sem) {
	// First thing we do is disable interrupts to prevent the OS from
	// context switch while a thread is holding a semaphore lock. We
	// will re-enable at the end of this function.
	// TODO: is this necessary?...
	interrupt_level_t old_interrupt_level = set_interrupt_level(DISABLED);

	while (atomic_test_and_set(&(sem->lock))) {
		minithread_yield();
	};
	if (++sem->count <= 0) {
		// Threads are waiting on resources, so pop one off the
		// queue and start it
		minithread_t thread_to_start;
		queue_dequeue(sem->waiting_q, (void **)&thread_to_start);
		minithread_start(thread_to_start);
		atomic_clear(&sem->lock);
	}
	atomic_clear(&sem->lock);

	set_interrupt_level(old_interrupt_level);
}

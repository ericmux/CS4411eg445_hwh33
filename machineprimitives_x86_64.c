/*
 * Minithreads x86_64/OSX Machine Dependent Code
 *
 * You should not need to modify this file.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>     // included for currentTimeMillis
#include <sys/timeb.h>

#include "defs.h"
#include "interrupts.h"
#include "machineprimitives.h"
#include "minithread.h"

uint64_t currentTimeMillis() {
  struct timeb timebuffer;
  uint64_t lt = 0;
  ftime(&timebuffer);
  lt = timebuffer.time;
  lt = lt*1000;
  lt = lt+timebuffer.millitm;
  return lt;
}


extern int atomic_test_and_set(tas_lock_t *l);

/*
 * swap
 *
 * atomically stores newval in *x, returns old value in *x
 */
extern int swap(int* x, int newval);

/*
 * compare and swap
 *
 * compare the value at *x to oldval, swap with
 * newval if successful
 */
extern int compare_and_swap(int* x, int oldval, int newval);

/*
 * atomic_clear
 *
 */

void atomic_clear(tas_lock_t *l) {
	*l = 0;	
}


/*
 * minithread_root
 *
 */
extern int minithread_root();


/*
 * minithread_switch - on the intel x86
 *
 */
extern void minithread_switch(stack_pointer_t *old_thread_sp_ptr,
                      stack_pointer_t *new_thread_sp_ptr);

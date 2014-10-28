/* itest1.c

   Spawn two single thread.
*/

#include "minithread.h"
#include "interrupts.h"

#include <stdio.h>
#include <stdlib.h>

int
scream(int* arg) {
  int my_id = minithread_id();
  printf("My ID is %d and my sleep number is %d\n", my_id, *arg); 
  minithread_sleep_with_timeout(1000*(*arg));
  printf("Woke up with arg: %d\n", *arg);
  fflush(stdout);
  return 0;
}

int
initializer_thread(int *arg) {
  int *p = (int *) malloc(sizeof(int));
  int *q = (int *) malloc(sizeof(int));
  *p = 5;
  *q = 3;
  minithread_fork(scream, p);
  minithread_fork(scream, q);
  return 0;
}

int
main(void) {
  minithread_system_initialize(initializer_thread, NULL);
  return 0;
}

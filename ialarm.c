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
  return 0;
}

int
main(void) {
  int *p = (int *) malloc(sizeof(int));
  *p = 5;
  minithread_system_initialize(scream, p);
  return 0;
}

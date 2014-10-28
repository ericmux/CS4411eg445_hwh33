/* itest1.c

   Spawn two single thread.
*/

#include "minithread.h"
#include "interrupts.h"

#include <stdio.h>
#include <stdlib.h>


static int a[10] = {7,4,9,0,1,3,2,5,6,8}; 

int
sleepsort(int* arg) {
  int my_id = minithread_id();
  //set_interrupt_level(ENABLED);
  printf("My ID is %d and my sleep number is %d\n", my_id, *arg); 
  minithread_sleep_with_timeout(1000*(*arg));
  printf("%d\n", *arg);
  return 0;
}

int initialize_threads(int *arg) {

  int i;
  for(i = 0; i < 10; i++){
	   minithread_fork(sleepsort, &a[i]);
   }

   printf("Threads forked!\n");

	return 0;
}

int
main(void) {
  minithread_system_initialize(initialize_threads, NULL);
  return 0;
}

/* itest1.c

   Spawn two single thread.
*/

#include "minithread.h"

#include <stdio.h>
#include <stdlib.h>


static int a[10] = {7,4,9,0,1,3,2,5,6,8}; 

int
sleepsort(int* arg) {
  minithread_sleep_with_timeout(1000*(*arg));
  printf("%d\n", *arg);
  return 0;
}

int
thread1(int* arg) {
  while(1){
    int i = 0;
    for(; i < 10000; i++);
  }

  return 0;
}

int initialize_threads(int *arg) {

  int i;
  for(i = 0; i < 10; i++){
	   minithread_fork(sleepsort, &a[i]);
   }
   minithread_fork(thread1, NULL);

   printf("Threads forked!");

	return 0;
}

int
main(void) {
  minithread_system_initialize(initialize_threads, NULL);
  return 0;
}
/* itest1.c

   Spawn two single thread.
*/

#include "minithread.h"

#include <stdio.h>
#include <stdlib.h>


int
thread1(int* arg) {
  while(1){
  	int i = 0;
    printf("I'm the 1st thread!\n");
    for(; i < 10000; i++);

  }

  return 0;
}

int
thread2(int* arg) {
  while(1){
    int i = 0;
  	printf("I'm the 2nd thread!\n");
    for(; i < 10000; i++);
  }

  return 0;
}

int initialize_threads(int *arg) {

	minithread_fork(thread1, NULL);
	minithread_fork(thread2, NULL);

	return 0;
}

int
main(void) {
  minithread_system_initialize(initialize_threads, NULL);
  return 0;
}
/* test_1000.c

   Spawns a big ass number of threads.
*/

#include "minithread.h"

#include <stdio.h>
#include <stdlib.h>


int zombie(int* arg) {
  int hp = 5000;
  int i;
  printf("Zombie %d.\n", minithread_id());
  for(i = 0; i < hp ; i++);

  return 0;
}

int necromancer(int* arg) {
  int num_children = 999;
  int i;
  printf("Necromancer %d spawning %d undead threads.\n", minithread_id(), num_children);
  for(i = 0; i < num_children; i++){
    minithread_fork(zombie, NULL);
  }
  printf("Come, my servants!\n");
  minithread_yield();
  printf("Fascinating.\n");

  return 0;
}

int
main(int argc, char * argv[]) {
  minithread_system_initialize(necromancer, NULL);

  //Should not reach this.
  printf("End of concurrency.\n");
  return 0;
}
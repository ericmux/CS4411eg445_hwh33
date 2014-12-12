#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


#include "minithread.h"
#include "synch.h"
#include "mini_mkfs.h"

#define BUFFER_SIZE 256
#define DEFAULT_MINIFILESYSTEM_SIZE 100


int mkfs_thread(int *arg){

	int dsk_siz;
	char cmd[BUFFER_SIZE];

	dsk_siz = *arg;

	kprintf("mkfs with %d blocks.\n", dsk_siz);

	mkfs(dsk_siz);

	kprintf("Good-bye :-)\n");
	return 0;
}


int main(int argc, char** argv) {

	int *dsk_siz_ptr;
	dsk_siz_ptr = (int *) malloc(sizeof(int));

	if(argc > 1){
		sscanf(argv[1], "%d", dsk_siz_ptr);
	} else {
		*dsk_siz_ptr = DEFAULT_MINIFILESYSTEM_SIZE;
	}

    minithread_system_initialize(mkfs_thread, dsk_siz_ptr);
    return -1;
}
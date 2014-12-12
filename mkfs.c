#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


#include "minithread.h"
#include "synch.h"
#include "mini_mkfs.h"

#define BUFFER_SIZE 256


int mkfs_thread(int *arg){

	int dsk_siz;
	char cmd[BUFFER_SIZE];

	gets(cmd);
	sscanf(cmd,"%d",&dsk_siz);

	kprintf("mkfs with %d blocks.\n", dsk_siz);

	mkfs(dsk_siz);

	kprintf("Good-bye :-)\n");
	return 0;
}


int main(int argc, char** argv) {
    minithread_system_initialize(mkfs_thread, NULL);
    return -1;
}
#include <stdlib.h>
#include "mini_mkfs.h"
#include "synch.h"



disk_t *fresh_disk;

semaphore_t disk_request_sema;

void mkfs_disk_handler(void *interrupt_arg){

	disk_interrupt_arg_t* disk_interrupt;
	disk_interrupt = (disk_interrupt_arg_t *) interrupt_arg; 

	kprintf("Request received!\n");

}


void mkfs(int dsk_siz){

	disk_layout_t layout;

	disk_request_sema = semaphore_create();
	semaphore_initialize(disk_request_sema,1);

	// Setting disk's global variables.
	use_existing_disk = 0;
	disk_name = DISK_NAME;
	disk_flags = DISK_READWRITE;
	disk_size = dsk_siz;

	fresh_disk = (disk_t *) malloc(sizeof(disk_t));
	disk_initialize(fresh_disk);

	layout = fresh_disk->layout;
	kprintf("Disk created successfully with %d blocks.\n", layout.size);

	//Install mkfs's disk handler.
	install_disk_handler(mkfs_disk_handler);


	//Create superblock.
	//Create free inode list.
	//Create free datablock list.

	//Request shutdown.
	disk_shutdown(fresh_disk);
	
}
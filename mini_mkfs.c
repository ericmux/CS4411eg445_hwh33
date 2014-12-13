#include <stdlib.h>
#include "mini_mkfs.h"
#include "minifile.h"
#include "synch.h"



disk_t *fresh_disk;
disk_reply_t disk_reply;

semaphore_t disk_request_sema;

void mkfs_disk_handler(void *interrupt_arg){

	disk_interrupt_arg_t* disk_interrupt;
	disk_interrupt = (disk_interrupt_arg_t *) interrupt_arg; 

	disk_reply = disk_interrupt->reply;
	semaphore_V(disk_request_sema);
}


void mkfs(int dsk_siz){

	disk_layout_t layout;
	superblock_t *superblock;
	superblock_t *superblock_in_disk;

	disk_request_sema = semaphore_create();
	semaphore_initialize(disk_request_sema,0);

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
	superblock = minifile_create_superblock(disk_size);
	disk_write_block(fresh_disk,0, (char *) superblock);

	kprintf("Creating superblock... ");
	semaphore_P(disk_request_sema);

	if(disk_reply != DISK_REPLY_OK){
		kprintf("Superblock write din't work. Fail.\n");
		return;
	}
	kprintf("OK.\n");

	//Read from disk just to make sure.
	disk_read_block(fresh_disk,0,(char *) superblock_in_disk);
	semaphore_P(disk_request_sema);

	if(superblock_in_disk->data.magic_number != superblock->data.magic_number){
		kprintf("Disk and memory superblock do not match. Fail.\n");
		return;
	}
	kprintf("Disk and memory superblock match.\n");






	//Create free inode list.
	//Create free datablock list.

	//Request shutdown.
	disk_shutdown(fresh_disk);
	
}
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

	inode_t *root_inode;

	int first_free_inode;
	int first_free_data_block;
	int i;

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

	kprintf("Writing superblock... ");
	semaphore_P(disk_request_sema);
	if(disk_reply != DISK_REPLY_OK){
		kprintf("Superblock write failed. Fail.\n");
		return;
	}
	kprintf("OK.\n");

	//Read from disk just to make sure the superblock in disk matches what we have.
	superblock_in_disk = (superblock_t *) malloc(sizeof(superblock_t));
	disk_read_block(fresh_disk,0,(char *) superblock_in_disk);
	semaphore_P(disk_request_sema);
	if(disk_reply != DISK_REPLY_OK){
		kprintf("Read failed. Fail.\n");
		return;
	}

	if(superblock_in_disk->data.magic_number 				!= superblock->data.magic_number 
		|| superblock_in_disk->data.disk_size				!= superblock->data.disk_size
		|| superblock_in_disk->data.first_free_inode		!= superblock->data.first_free_inode
		|| superblock_in_disk->data.first_free_data_block	!= superblock->data.first_free_data_block ){
		kprintf("Disk and memory superblock do not match. Fail.\n");
		return;
	}
	kprintf("Disk and memory superblock match.\n");
	free(superblock_in_disk);

	//Create root inode.
	root_inode = minifile_create_root_inode();
	disk_write_block(fresh_disk,root_inode->data.idx,(char *) root_inode);

	kprintf("Writing root inode...");
	semaphore_P(disk_request_sema);
	if(disk_reply != DISK_REPLY_OK){
		kprintf("Root inode write failed. Fail.\n");
		return;
	}
	kprintf("OK.\n");	


	first_free_inode = superblock->data.first_free_inode;
	first_free_data_block = superblock->data.first_free_data_block;
	
	//Create free inode list.
	for(i = first_free_inode; i < first_free_data_block; i++){
		free_block_t *free_block;
		int nxt;

		if(i == first_free_data_block - 1) nxt = NULL_PTR;
		else nxt = i+1;

		free_block = minifile_create_free_block(nxt);
		disk_write_block(fresh_disk,i,(char *) free_block);

		kprintf("Writing free inode block at %d...", i);
		semaphore_P(disk_request_sema);
		if(disk_reply != DISK_REPLY_OK){
			kprintf("Root inode write failed. Fail.\n");
			return;
		}
		kprintf("OK.\n");	
	}

	//Create free datablock list.
	for(i = first_free_data_block; i < disk_size; i++){
		free_block_t *free_block;
		int nxt;

		if(i == disk_size - 1) nxt = NULL_PTR;
		else nxt = i+1;

		free_block = minifile_create_free_block(nxt);
		disk_write_block(fresh_disk,i,(char *) free_block);

		kprintf("Writing free data block at %d...", i);
		semaphore_P(disk_request_sema);
		if(disk_reply != DISK_REPLY_OK){
			kprintf("Root inode write failed. Fail.\n");
			return;
		}
		kprintf("OK.\n");
	}


	//Request shutdown.
	kprintf("Transactions completed. Shutting down disk...\n");
	disk_shutdown(fresh_disk);
	
}
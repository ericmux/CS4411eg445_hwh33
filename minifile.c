#include <stdlib.h>

#include "hashtable.h"
#include "minifile.h"
#include "disk.h"
#include "synch.h"
#include "interrupts.h"

#define MAX_NUM_THREADS  0xffff
#define ROOT_INODE_NUM 1

// Return values from disk requests.
typedef enum {
	DISK_REQUEST_SUCCESS = 0,
	DISK_REQUEST_ERROR = -1,
	DISK_OVERLOADED = -2
} disk_request_return_val;

typedef enum {
	FILE_INODE 		= 0,
	DIRECTORY_INODE = 1
} inode_type_t;

/*
 * struct minifile:
 *     This is the structure that keeps the information about 
 *     the opened file like the position of the cursor, etc.
 */
typedef struct minifile {
	int inode_number;
	int cursor_position;
	int current_num_rws;
} minifile;

//Filesystem active.
int filesystem_active = 0;

// Version of superblock in memory.
superblock_t *superblock;

// Each block gets a lock to protect its metadata.
semaphore_t *block_locks;

//Every thread, after getting a hold of a block's lock, should wait until the disk has finished the handling the request
//and only then should it release the lock. This should guarantee the FS is always in a consistent state.
semaphore_t *block_op_finished_semas;

// Maps process IDs to current working directories represented by dir_data structures.
dir_data_t thread_cd_map[MAX_NUM_THREADS];


//Include util functions.
#include "minifile_utils.c"


void minifile_disk_handler(void *interrupt_arg){

	int blocknum;
	disk_interrupt_arg_t* disk_interrupt;
	disk_request_t disk_request;
	disk_interrupt = (disk_interrupt_arg_t *) interrupt_arg; 

	if(!filesystem_active) return;

	disk_request = disk_interrupt->request;
	blocknum = disk_request.blocknum;
	semaphore_V(block_op_finished_semas[blocknum]);
}



int minifile_init(disk_t *input_disk) {

	int i;
	int request_result;
	int result;

	//Initialize disk, using the existing one provided by mkfs.
	use_existing_disk = 1;
	disk_name = DISK_NAME;

	result = disk_initialize(input_disk);
	if(result == -1){
		return -1;
	}

	//Install our minifile disk handler.
	install_disk_handler(minifile_disk_handler);

	//Initialize synchronization structures.
	block_locks = (semaphore_t *) malloc(disk_size*sizeof(semaphore_t));
	block_op_finished_semas = (semaphore_t *) malloc(disk_size*sizeof(semaphore_t));
	for(i = 0; i < disk_size; i++){
		block_locks[i] = semaphore_create();
		semaphore_initialize(block_locks[i],1);

		block_op_finished_semas[i] = semaphore_create();
		semaphore_initialize(block_op_finished_semas[i], 0);
	}

	//Initialize thread directory map.
	for(i = 0; i < MAX_NUM_THREADS; i++){
		strcpy(thread_cd_map[i].absolute_path,"/");
		thread_cd_map[i].inode_number = 1;
	}

	// Initialize the superblock in memory. Check the magic number before proceeding. 
	// Not grabbing locks since this is called before concurrency begins in minithread.
	superblock = (superblock_t *) malloc(sizeof(superblock_t));

	request_result = disk_read_block(input_disk, 0, (char *)superblock);
	if (request_result == DISK_REQUEST_ERROR 
		|| superblock->data.magic_number != SUPERBLOCK_MAGIC_NUM) {
		return -1;
	}

	//Keeps the semaphore count for the superblock's lock consistent. We don't want to P
	//because all this is happening before concurrency begins.
	filesystem_active = 1;
	return 0;
}

minifile_t minifile_creat(char *filename){
	char *parent_path;
	inode_t *new_inode;
	inode_t *old_inode;
	int file_inode_number;
	minifile_t new_minifile;

	int i;
	int request_result;

	// Get the file's parent directory.
	parent_path = get_parent_path(filename);

	// Create the file's inode.
	new_inode = (inode_t *)malloc(sizeof(struct inode));
	new_inode->data.type = FILE_INODE;
	new_inode->data.size = 0;
	for (i = 0; i < DIRECT_PTRS_PER_INODE; i++) {
		new_inode->data.direct_ptrs[i] = NULL_PTR;
	}
	new_inode->data.indirect_ptr = NULL_PTR;

	// Check to see if this file exists in the current directory. If so, we'll overwrite 
	// that inode block with our new one. If not, get the number of the first free inode.
	request_result = traverse_to_inode(&old_inode, filename);
	if (request_result == -1) {
		file_inode_number = get_free_inode();
	} else {
		file_inode_number = old_inode->data.idx;
	}

	// Write the inode and, when finished, free it.
	disk_write_block(minifile_disk, file_inode_number, (char *)new_inode);
	// XXX: wait for confirmation of write?
	free(new_inode);

	// Create and return the new minifile pointer.
	new_minifile = (minifile_t) malloc(sizeof(struct minifile));
	new_minifile->inode_number = file_inode_number;
	new_minifile->cursor_position = 0;
	new_minifile->current_num_rws = 0;

	return new_minifile;
}

minifile_t minifile_open(char *filename, char *mode){
	return NULL;
}

int minifile_read(minifile_t file, char *data, int maxlen){

	return -1;
}

int minifile_write(minifile_t file, char *data, int len){

	return -1;
}

int minifile_close(minifile_t file){

	return -1;
}

int minifile_unlink(char *filename){

	return -1;
}

int minifile_mkdir(char *dirname){

	return -1;
}

int minifile_rmdir(char *dirname){

	return -1;
}

int minifile_stat(char *path){

	return -1;
} 

int minifile_cd(char *path){

	int old_level;
	inode_t *target_inode;

	if(path == NULL) return -1;
	if(strcmp(path,"/") == 0){
		old_level = set_interrupt_level(DISABLED);
		
		strcpy(thread_cd_map[minithread_id()].absolute_path,"/");
		thread_cd_map[minithread_id()].inode_number = 1;
		
		set_interrupt_level(old_level);
		return 0;
	}

	if(traverse_to_inode(&target_inode, path) != 0){
		kprintf("inode traversal failed.\n");
		return -1;
	}

	old_level = set_interrupt_level(DISABLED);
		
	strcpy(thread_cd_map[minithread_id()].absolute_path, get_absolute_path(path));
	thread_cd_map[minithread_id()].inode_number = target_inode->data.idx;
		
	set_interrupt_level(old_level);	

	return -1;
}

char **minifile_ls(char *path){

	return NULL;
}

char* minifile_pwd(void){
	char *pwd;
	pwd = (char *) malloc(MAX_CHARS_IN_FNAME);

	strcpy(pwd,thread_cd_map[minithread_id()].absolute_path);
	return pwd;
}

int minifile_initialize_disk(){
	minifile_disk = (disk_t *) malloc(sizeof(disk_t));
	if(minifile_disk == NULL){
		return -1;
	}

	return 0;
}


superblock_t *minifile_create_superblock(int dsk_siz){

	superblock_t *superblock;
	int first_free_data_block; 

	if(dsk_siz < MINIMUM_DISK_SIZE){
		kprintf("Chosen disk size less than minimum of %d. Fail.\n ", MINIMUM_DISK_SIZE);
		return NULL;
	}

	superblock = (superblock_t *) malloc(sizeof(superblock_t));
	superblock->data.magic_number = SUPERBLOCK_MAGIC_NUM;
	superblock->data.disk_size = dsk_siz;
	superblock->data.root_inode = 1;
	superblock->data.first_free_inode = 2;

	first_free_data_block = (dsk_siz/INODE_RATIO_IN_DISK) + 1;

	superblock->data.first_free_data_block = first_free_data_block;

	return superblock;

}

inode_t *minifile_create_root_inode(){
	inode_t *root_inode;

	root_inode = (inode_t *) malloc(sizeof(inode_t));
	root_inode->data.idx = 1;
	root_inode->data.type = DIRECTORY_INODE;
	root_inode->data.size = 0;
	root_inode->data.parent_inode = NULL_PTR;
	memset(root_inode->data.direct_ptrs, NULL_PTR, DIRECT_PTRS_PER_INODE);
	root_inode->data.indirect_ptr = NULL_PTR;

	return root_inode;
}


free_block_t *minifile_create_free_block(int next_free_block){
	free_block_t *free_block;

	free_block = (free_block_t *) malloc(sizeof(free_block_t));
	free_block->next_free_block = next_free_block;

	return free_block;
}

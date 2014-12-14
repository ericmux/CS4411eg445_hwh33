#include <stdlib.h>

#include "hashtable.h"
#include "minifile.h"
#include "disk.h"
#include "synch.h"
#include "interrupts.h"

#define MAX_NUM_THREADS  0xffff
#define SUPERBLOCK_NUM 0
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

	disk_request = disk_interrupt->request;
	blocknum = disk_request.blocknum;
	semaphore_V(block_op_finished_semas[blocknum]);
}



int minifile_init(disk_t *input_disk) {
	int i;
	int request_result;
	int result;
	int old_level;

	old_level = set_interrupt_level(DISABLED);

	//Initialize disk, using the existing one provided by mkfs.
	use_existing_disk = 1;
	disk_name = DISK_NAME;

	result = disk_initialize(input_disk);
	if(result == -1){
		set_interrupt_level(old_level);
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


	set_interrupt_level(old_level);

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
	return 0;
}

minifile_t minifile_creat(char *filename){
	char *parent_path;
	inode_t *new_inode;
	inode_t *old_inode;
	inode_t *parent_inode;
	int file_inode_number;
	minifile_t new_minifile;
	inode_mapping_t *new_mapping;

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
	request_result = reliable_write_block(
		minifile_disk, file_inode_number, (char *)new_inode);
	if (request_result == -1) return NULL;
	free(new_inode);

	// Create a new mapping for this inode and add it to the parent inode.
	new_mapping = (inode_mapping_t *)malloc(sizeof(struct inode_mapping));
	new_mapping->filename = get_local_filename(filename);
	new_mapping->inode_number = file_inode_number;
	parent_inode = (inode_t *)malloc(sizeof(struct inode));
	request_result = traverse_to_inode(&parent_inode, parent_path);
	add_mapping(parent_inode, new_mapping);
	free(new_mapping);
	free(parent_inode);

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

	int i;
	int read_result;
	char *parent_path;
	inode_t *target_folder;
	inode_t *dir_inode;

	int dir_inode_number;

	inode_mapping_t new_mapping;

	parent_path = get_parent_path(dirname);

	if(traverse_to_inode(&target_folder, parent_path) != 0){
		kprintf("mkdir path not valid.\n");
		return -1;
	}

	dir_inode = (inode_t *) malloc(sizeof(inode_t));

	//Grab a free inode block.
	semaphore_P(block_locks[0]);

	read_result = disk_read_block(minifile_disk, 0, (char *)superblock);
	semaphore_P(block_op_finished_semas[0]);
	if (read_result != DISK_REQUEST_SUCCESS) {
		semaphore_V(block_locks[0]);			
		return -1;
	}

	dir_inode_number = superblock->data.first_free_inode;
	if(dir_inode_number == NULL_PTR){
		kprintf("Disk if full. No more free inode blocks available.\n");
		semaphore_V(block_locks[0]);
		return -1;		
	}

	//Grab the free inode block.
	semaphore_P(block_locks[dir_inode_number]);

	read_result = disk_read_block(minifile_disk, dir_inode_number, (char *)dir_inode);
	semaphore_P(block_op_finished_semas[dir_inode_number]);
	if (read_result != DISK_REQUEST_SUCCESS) {
		semaphore_V(block_locks[dir_inode_number]);			
		return -1;
	}	
	semaphore_V(block_locks[dir_inode_number]);

	//remove dir_inode from free block list and update superblock.
	superblock->data.first_free_inode = ((free_block_t *) dir_inode)->next_free_block;
	read_result = disk_write_block(minifile_disk, 0, (char *)superblock);
	semaphore_P(block_op_finished_semas[0]);
	if (read_result != DISK_REQUEST_SUCCESS) {
		semaphore_V(block_locks[0]);			
		return -1;
	}

	//Update dir_inode to have actual directory inode valid data.
	dir_inode->data.idx = dir_inode_number;
	dir_inode->data.type = DIRECTORY_INODE;
	dir_inode->data.size = 0;
	for(i = 0; i < DIRECT_PTRS_PER_INODE; i++){
		dir_inode->data.direct_ptrs[i] = NULL_PTR;
	}
	dir_inode->data.indirect_ptr = NULL_PTR;

	semaphore_P(block_op_finished_semas[dir_inode_number]);

	read_result = disk_write_block(minifile_disk, dir_inode_number, (char *)dir_inode);
	semaphore_P(block_op_finished_semas[dir_inode_number]);
	if (read_result != DISK_REQUEST_SUCCESS) {
		semaphore_V(block_locks[dir_inode_number]);			
		return -1;
	}
	semaphore_V(block_locks[dir_inode_number]);

	//Update target_folder to have a mapping to this new folder.
	strcpy(new_mapping.filename, get_local_filename(dirname));
	new_mapping.inode_number = dir_inode_number;
	add_mapping(target_folder, &new_mapping);

	semaphore_V(block_locks[0]);

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
	int i;
	inode_t *root_inode;

	root_inode = (inode_t *) malloc(sizeof(inode_t));
	root_inode->data.idx = 1;
	root_inode->data.type = DIRECTORY_INODE;
	root_inode->data.size = 0;

	for(i = 0; i < DIRECT_PTRS_PER_INODE; i++){
		root_inode->data.direct_ptrs[i] = NULL_PTR;
	}
	root_inode->data.indirect_ptr = NULL_PTR;

	return root_inode;
}


free_block_t *minifile_create_free_block(int next_free_block){
	free_block_t *free_block;

	free_block = (free_block_t *) malloc(sizeof(free_block_t));
	free_block->next_free_block = next_free_block;

	return free_block;
}

/*
 * Utility functions for minifiles.
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "minithread.h"
#include "alarm.h"

#define DISK_OP_TIMEOUT 500*MILLISECOND

/* Returns the absolute path for the given filename. If the given filename
 * is already an absolute path, it is returned as is.
 */
char* get_absolute_path(char *filename) {

	char *abs_path;
	char *path_separator;
	char current_directory[MAX_CHARS_IN_FNAME];

	abs_path = (char *) malloc(MAX_CHARS_IN_FNAME);

	if(filename == NULL) return NULL;
	if(filename[0] == '/'){
		strcpy(abs_path, filename);
		return abs_path;
	}

	strcpy(current_directory, thread_cd_map[0].absolute_path);

	path_separator = "/";
	strcpy(abs_path,"");

	if(strcmp(current_directory,"/") != 0){
		strcat(abs_path, current_directory);
	}
	strcat(abs_path, path_separator);
	strcat(abs_path, filename);

	return abs_path;
}

/* Returns the path to the file's parent. filename can be a local or
 * absolute path.
 */

char* get_parent_path(char *filename) {
	int last_separator_index;
	int i;
	// dir_data *parent_dir_data;
	char *parent_path;

	if (filename[0] == '/') {
		last_separator_index = 0;
		for (i = 1; i < strlen(filename); i++) {
			if (filename[i] == '/') {
				last_separator_index = i;
			}
		}
		parent_path = (char *)malloc(last_separator_index);
		strncpy(parent_path, filename, last_separator_index);
		return parent_path;
	} else {
		return thread_cd_map[minithread_id()].absolute_path;
	}
}

/* Returns a list of substrings split by the given delimiter. Stores the
 * number of substrings returned in num_substrings. 
 */
char** str_split(char *input_string, char delimiter, int *num_substrings) {
	int i;
	int curr_word_start;
	int curr_word_size;
	int curr_substring;
	char **return_array;

	*num_substrings = 1;
	for (i = 0; i < strlen(input_string); i++) {
		if (input_string[i] == delimiter) {
			(*num_substrings)++;
		}
	}

	return_array = (char **)malloc(*num_substrings * sizeof(char *));

	curr_word_size = 0;
	curr_substring = 0;
	curr_word_start = 0;
	for (i = 0; i < strlen(input_string); i++) {
		if (input_string[i] == delimiter) {
			return_array[curr_substring] = (char *)malloc(curr_word_size + 1);
			strncpy(return_array[curr_substring], &input_string[curr_word_start], curr_word_size);
			curr_word_start = curr_word_start + curr_word_size + 1;
			curr_word_size = 0;
			curr_substring++;
		} else {
			curr_word_size++;
		}
	}
	// Add the last word to return_array.
	return_array[curr_substring] = (char *)malloc(curr_word_size + 1);
	strncpy(return_array[curr_substring], &input_string[curr_word_start], curr_word_size);

	return return_array;
}


void disk_op_alarm_handler(void *sema){
	semaphore_t s = (semaphore_t) sema;
	semaphore_V(s);
}

/*
* Tries to read a block or fails if the timeout expires.
*/
int reliable_read_block(disk_t *disk, int blocknum, char *buffer){

	int read_result;
	alarm_id timeout_alarm;

	read_result = DISK_REQUEST_ERROR;

	semaphore_P(block_locks[blocknum]);

	timeout_alarm = register_alarm(DISK_OP_TIMEOUT, disk_op_alarm_handler, block_op_finished_semas[blocknum]);

	read_result = disk_read_block(disk, blocknum, buffer);
	semaphore_P(block_op_finished_semas[blocknum]);
	if (read_result != DISK_REQUEST_SUCCESS) {
		semaphore_V(block_locks[blocknum]);			
		return -1;
	}

	deregister_alarm(timeout_alarm);

	semaphore_V(block_locks[blocknum]);
}



/* Returns the inode at the given path in found_inode. The return value
 * is 0 on success and -1 on error.
 */
int traverse_to_inode(inode_t **found_inode, char *path) {
	int len;
	char **dirs;
	int old_level;
	int read_result;
	int i;
	int j;
	int k;

	int base_inode;

	inode_t *cd;

	*found_inode = NULL;

	dirs = str_split(path, '/', &len);

	if(path == NULL) return -1;
	if(len == 0) return -1;

	//Check if absolute path.
	if(strcmp(dirs[0], "") == 0){
		base_inode = 1;
	} else {
		old_level = set_interrupt_level(DISABLED);		
		base_inode = thread_cd_map[minithread_id()].inode_number;
		set_interrupt_level(old_level);
	}

	//current directory inode buffer.
	cd = (inode_t *) malloc(sizeof(inode_t));

	for(i = 0; i < len; i++){

		if(strcmp(dirs[i],"") == 0) continue;

		semaphore_P(block_locks[base_inode]);

		read_result = disk_read_block(minifile_disk, base_inode, (char *)cd);
		semaphore_P(block_op_finished_semas[base_inode]);
		if (read_result != DISK_REQUEST_SUCCESS) {
			semaphore_V(block_locks[base_inode]);			
			return -1;
		}

		semaphore_V(block_locks[base_inode]);

		if(cd->data.type != DIRECTORY_INODE) return -1;

		// Empty/uninitialized directory.
		if(cd->data.size < 1) return -1;

		//Read direct/indirect ptrs and look for mappings.
		for(j = 0; j < cd->data.size; j++){
			int directory_data_blocknum;
			directory_data_block_t *dir_data_block;

			//shouldn't happen if the filesytem is consistent (size would be wrong). We're still only going for directs.
			if(cd->data.direct_ptrs[j] == NULL_PTR) {
				kprintf("The size of directory inode %d is incorrect.\n", cd->data.size);
				return -1;
			}

			directory_data_blocknum = cd->data.direct_ptrs[j];

			dir_data_block = (directory_data_block_t *) malloc(sizeof(directory_data_block_t));
			
			semaphore_P(block_locks[directory_data_blocknum]);

			read_result = disk_read_block(minifile_disk,directory_data_blocknum,(char *) dir_data_block);
			semaphore_P(block_op_finished_semas[directory_data_blocknum]);
			if (read_result != DISK_REQUEST_SUCCESS) {
				semaphore_V(block_locks[directory_data_blocknum]);
				return -1;
			}

			semaphore_V(block_locks[directory_data_blocknum]);

			//search for the mapping matching dirs[i].
			for(k = 0; k < dir_data_block->data.num_maps; k++){
				inode_mapping_t mapping = dir_data_block->data.inode_map[k];

				if(strcmp(dirs[i], mapping.filename) == 0){
					base_inode = mapping.inode_number;
				}
			}

		}
	}

	semaphore_P(block_locks[base_inode]);

	read_result = disk_read_block(minifile_disk, base_inode, (char *)cd);
	semaphore_P(block_op_finished_semas[base_inode]);
	if (read_result != DISK_REQUEST_SUCCESS) {
		semaphore_V(block_locks[base_inode]);
		return -1;
	}

	semaphore_V(block_locks[base_inode]);

	//basenode points to target node, read and return.
	*found_inode = cd;
	return 0;
}

/* Returns the inode number for the next free inode. Handles management of
 * free inode list. If no free inodes are available, -1 is returned.
 */
int get_free_inode() {
	int free_inode_num;
	free_block_t *free_inode_block;

	semaphore_P(block_locks[0]);
	
	free_inode_num = superblock->data.first_free_inode;
	if (free_inode_num == NULL_PTR) {
		return NULL_PTR;
	}
	free_inode_block = (free_block_t *)malloc(sizeof(struct free_block));
	reliable_read_block(minifile_disk, free_inode_num, (char *)free_inode_block);
	superblock->data.first_free_inode = free_inode_block->next_free_block;
	
	semaphore_V(block_locks[0]);

	return free_inode_num;
}

/* Returns the block number for the next free datablock. Handles management of
 * free datablock list. If no free inodes are available, -1 is returned.
 */
int get_free_datablock() {
	int free_datablock_num;
	free_block_t *free_datablock;

	semaphore_P(block_locks[0]);
	
	free_datablock_num = superblock->data.first_free_data_block;
	if (free_datablock_num == NULL_PTR) {
		return NULL_PTR;
	}
	free_datablock = (free_block_t *)malloc(sizeof(struct free_block));
	reliable_read_block(minifile_disk, free_datablock_num, (char *)free_datablock);
	superblock->data.first_free_data_block = free_datablock->next_free_block;
	
	semaphore_V(block_locks[0]);

	return free_datablock_num;
}
/*
 * Utility functions for minifiles.
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "minithread.h"

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
// XXX: find way to avoid passing in hashtable? 
char* get_parent_path(char *filename, hashtable_t current_directory_map) {
	// int last_separator_index;
	// int i;
	// dir_data *parent_dir_data;
	// char *parent_path;

	// if (filename[0] == '/') {
	// 	last_separator_index = 0;
	// 	for (i = 1; i < strlen(filename); i++) {
	// 		if (filename[i] == '/') {
	// 			last_separator_index = i;
	// 		}
	// 	}
	// 	return filename[last_separator_index + 1];
	// } else {
	// 	parent_dir_data = (dir_data *)malloc(sizeof(struct dir_data));
	// 	hashtable_get(current_directory_map, minithread_id(), &parent_dir_data);
	// 	parent_path = parent_dir_data->absolute_path;
	// 	free(parent_dir_data);
	// 	return parent_path;
	// }

	return NULL;
}

/* Attempts to read a block until successful. Stops on DISK_REQUEST_ERROR.
 * Returns 0 on success and -1 on failure.
 */
int reliable_read_block(disk_t *disk, int blocknum, char* buffer) {
	int request_result = DISK_OVERLOADED;

	while (request_result == DISK_OVERLOADED) {
		request_result = disk_read_block(disk, blocknum, buffer);
	}

	return request_result;
}


/* Increments op_counter and handles rollover. */
unsigned int increment_op_counter(unsigned int op_count) {
	if (op_count < UINT_MAX) {
		return ++op_count;
	} else {
		// Wait for operation UINT_MAX to be written.
		// XXX: how to wait?
		return 0;
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
	i = (input_string[0] == delimiter) ? 1 : 0;
	for (; i < strlen(input_string); i++) {
		if (input_string[i] == delimiter) {
			(*num_substrings)++;
		}
	}

	return_array = (char **)malloc(*num_substrings * sizeof(char *));

	curr_word_size = 0;
	curr_substring = 0;
	curr_word_start = (input_string[0] == delimiter) ? 1 : 0;
	for (i = curr_word_start; i < strlen(input_string); i++) {
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

/* Returns the inode number for the file/directory with the given name.
 * Must be a direct child of the given parent.
 */
int traverse_to_inode(inode_t **found_inode, char *path) {
	int len;
	char **dirs;
	int old_level;
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
		int read_result;

		if(strcmp(dirs[i],"") == 0) continue;

		semaphore_P(block_locks[base_inode]);

		read_result = disk_read_block(minifile_disk, base_inode, (char *)cd);
		semaphore_P(block_op_finished_semas[base_inode]);
		if (read_result != DISK_REQUEST_SUCCESS) {
			return -1;
		}

		semaphore_V(block_locks[base_inode]);

		//We just read the target inode, break.
		if(i == len-1){
			*found_inode = cd;
			return 0;
		}

		if(cd->data.type != DIRECTORY_INODE) return -1;

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

	return -1;
}

/* Searches for name in the given parent directory. If found, the inode 
 * number is returned. Else, returns -1. 
 * 'name' can refer to a file or directory and it can be local or absolute, 
 * but it must be a direct child of the given parent directory.
 */
int get_inode_num(char *absolute_path) {
	// int num_substrings;
	// char **splits;
	// char *filename;
	// int i;
	// inode *current_inode;
	// int current_inode_number;
	// int request_result;

	// // The last split is the file/directory whose inode number we are looking for.
	// splits = str_split(absolute_path, '/', &num_substrings);

	// // The first inode will be the root inode, at inode number 1.
	// current_inode = (inode *)malloc(sizeof(struct inode));
	// current_inode_number = 1;

	// // We iterate through the path as defined by splits. For each member of splits,
	// // we find its inode number, then use that to get the inode of the next member
	// // down the path. The final inode number is the one to return. If we ever get
	// // -1 from get_inode_num_in_parent or reliable_read_block, we must return that.
	// for (i = 0; i < num_substrings; i++) {
		
	// 	// Load the current inode.
	// 	request_result = reliable_read_block(
	// 		disk, current_inode_number, (char *)current_inode);
	// 	if (request_result == -1) {
	// 		return -1;
	// 	}

	// 	// Get the inode number for splits[i].
	// 	current_inode_number = get_inode_num_in_parent(current_inode, splits[i]);
	// }

	// // The last inode number obtained is the one we want.
	// return current_inode_number;

	return -1;
}

// // XXX: necessary?
//  Returns the inode corresponding to the given inode number. 
// inode* get_inode(int inode_number) {
// 	// Implement me
// 	return -1;
// }

/* Returns the inode number for the next free inode. Handles management of
 * free inode list. If no free inodes are available, -1 is returned.
 */
int get_free_inode() {
	// int free_inode_num;
	// free_block *fb;

	// semaphore_P(metadatalock[0]);
	
	// free_inode_num = superblock->data->first_free_inode;
	// if (free_inode_num == NULL_PTR) {
	// 	return NULL_PTR;
	// }
	// fb = (free_block *)malloc(sizeof(struct free_block));
	// reliable_read_block(disk, free_inode_num, (char *)free_block);
	// superblock->first_free_inode = fb->next_free_block;
	
	// semaphore_V(metadatalock[0]);

	// return free_inode_num;

	return -1;
}
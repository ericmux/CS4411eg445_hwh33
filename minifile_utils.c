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


/* Returns the inode number for the file/directory with the given name.
 * Must be a direct child of the given parent.
 */
int get_inode_num_in_parent(inode_t *parent_inode, char *name_to_find) {
	indirect_data_block_t *current_indirect_block;
	directory_data_block_t *current_dir_block;
	char *current_inode_name;
	
	int request_result;
	int total_mappings;
	int i;
	int j;
	int i_stop;
	int j_stop;

	current_indirect_block = (indirect_data_block_t *)malloc(sizeof(struct indirect_data_block));
	current_dir_block = (directory_data_block_t *)malloc(sizeof(struct directory_data_block));

	// Load the first indirect block.
	request_result = reliable_read_block(
		minifile_disk, parent_inode->data.indirect_ptr, (char *)current_indirect_block);
	if (request_result == -1) {
		// If there was an error loading that block, we have to quit.
		return -1;
	}

	// Outer loop iterates through all indirect blocks. 
	// Middle loop iterates through all direct pointers.
	// Innermost loop iterates through all mappings.
	total_mappings = 0;
	while (total_mappings < parent_inode->data.size) {

		// Determine how many direct pointers we can safely loop over.
		if ((parent_inode->data.size - total_mappings) < DIRECT_BLOCKS_PER_INDIRECT) { 
			i_stop = parent_inode->data.size - total_mappings;
		} else {
			i_stop = DIRECT_BLOCKS_PER_INDIRECT;
		}

		for (i = 0; i < i_stop; i++) {

			// Load the current directory block.
			request_result = reliable_read_block(
				minifile_disk, current_indirect_block->direct_ptrs[i], (char *)current_dir_block);
			if (request_result == -1) {
				// If there was an error loading that block, we have to quit.
				return -1;
			}

			// Determine how many mappings we can safely loop over.
			if ((parent_inode->data.size - total_mappings) < INODE_MAPS_PER_BLOCK) { 
				j_stop = parent_inode->data.size - total_mappings;
			} else {
				j_stop = INODE_MAPS_PER_BLOCK;
			}

			for (j = 0; j < j_stop; j++, total_mappings++) {
				current_inode_name = current_dir_block->data.inode_map[j].filename;
				if strcmp(name_to_find, current_inode_name) {
					// We found the inode we were looking for.
					inode_number = current_dir_block->data.inode_map[j].inode_number;
					free(current_indirect_block);
					free(current_dir_block);
					return inode_number;
				}
			}
		}

		// Load the next indirect block.
		request_result = reliable_read_block(
			minifile_disk, current_indirect_block->data.indirect_ptr, (char *)current_indirect_block);
		if (request_result == -1) {
			// If there was an error loading that block, we have to quit.
			free(current_indirect_block);
			free(current_dir_block);
			return -1;
		}
	}

	// If this point is reached, then the inode was not found.
	free(current_indirect_block);
	free(current_dir_block);
	return -1;
}

/* Searches for name in the given parent directory. If found, the inode 
 * number is returned. Else, returns -1.
 */
int get_inode_num(char *absolute_path) {
	int num_substrings;
	char **splits;
	char *filename;
	int i;
	inode_t *current_inode;
	int current_inode_number;
	int request_result;

	// The last split is the file/directory whose inode number we are looking for.
	splits = str_split(absolute_path, '/', &num_substrings);

	// The first inode will be the root inode.
	current_inode = (inode *)malloc(sizeof(struct inode));
	current_inode_number = ROOT_INODE_NUM;

	// We iterate through the path as defined by splits. For each member of splits,
	// we find its inode number, then use that to get the inode of the next member
	// down the path. The final inode number is the one to return. If we ever get
	// -1 from get_inode_num_in_parent or reliable_read_block, we must return that.
	for (i = 1; i < num_substrings; i++) {
		
		// Load the current inode.
		request_result = reliable_read_block(
			minifile_disk, current_inode_number, (char *)current_inode);
		if (request_result == -1) {
			return -1;
		}

		// Get the inode number for splits[i].
		current_inode_number = get_inode_num_in_parent(current_inode, splits[i]);
	}

	// The last inode number obtained is the one we want.
	return current_inode_number;

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
	// reliable_read_block(minifile_disk, free_inode_num, (char *)free_block);
	// superblock->first_free_inode = fb->next_free_block;
	
	// semaphore_V(metadatalock[0]);

	// return free_inode_num;

	return -1;
}
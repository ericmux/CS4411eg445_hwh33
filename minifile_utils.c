/*
 * Utility functions for minifiles.
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>

// Forward declaration of types from minifile.c for the compiler.
typedef enum disk_request_return_val;
typedef struct superblock;
typedef struct inode;
typedef struct inode_mapping;
typedef struct directory_data_block;
typedef struct free_block;
typedef struct indirect_data_block;


/* Returns the absolute path for the given filename. If the given filename
 * is already an absolute path, it is returned as is.
 */
char* get_absolute_path(char *filename, char *parent_path) {
	if (filename[0] != '/') {
		char *path_separator = "/";
		char *abs_path = (char *)malloc(
			strlen(current_directory) + strlen(path_separator) + strlen(filename));
		strcat(abs_path, current_directory);
		strcat(abs_path, path_separator);
		strcat(abs_path, filename);
		return abs_path;
	} else {
		return filename;
	}
}

/* Returns the path to the file's parent. */
char* get_parent_path(char *filename) {

}

/* Increments op_counter and handles rollover. */
unsigned int increment_op_counter(unsigned int op_count) {
	// Implement me
	return -1;
}

/* Searches for name in the given parent directory. If found, the inode 
 * number is returned. Else, returns -1. 
 * 'name' can refer to a file or directory and it can be local or absolute, 
 * but it must be a direct child of the given parent directory.
 */
int get_inode_num(char *parent_path, char *name) {
	// Implement me
	return -1;
}

// XXX: necessary?
/* Returns the inode corresponding to the given inode number. */
inode* get_inode(int inode_number) {
	// Implement me
	return -1;
}

/* Returns the inode number for the next free inode. Handles management of
 * free inode list. If no free inodes are available, -1 is returned.
 */
int get_free_inode() {
	// Implement me
	// NOTE: need to lock superblock before manipulation.
	return -1;
}
/*
 * Utility functions for minifiles.
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>

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

/* Strips off all but the local filename. */
char* get_local_name(char *filename) {
	// Implement me
	// NOTE: maybe this should take in an absolute path?
	return NULL;
}

/* Returns the path to the file's parent. */
char* get_parent_path(char *filename) {

}

/* Increments op_counter and handles rollover. */
unsigned int increment_op_counter(unsigned int op_count) {
	// Implement me
	return -1;
}

/* Searches for name in dir_inode. If found, the inode number is returned.
 * Else, returns -1. 
 * 'name' can refer to a file or directory, but it must
 * be a direct child of the directory to which dir_inode corresponds.
 */
int get_inode_num(char *parent_path, char *name) {
	// Implement me
	return -1;
}

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
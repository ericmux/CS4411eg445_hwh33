/*
 * Utility functions for minifiles.
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "disk.h"

char* get_absolute_path(char *filename, char *current_directory) {
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

char* get_local_name(char *filename) {
	// Implement me
	// NOTE: maybe this should take in an absolute path?
	return 'implement me'
}

unsigned int increment_op_counter(unsigned int op_count) {
	// Implement me
	return -1;
}

int get_inode_num(inode *dir_inode, char *name) {
	// Implement me
	return -1;
}
/*
 * Utility functions for minifiles
 */

/* Returns the absolute path for the given filename. If the given filename
 * is already an absolute path, it is returned as is.
 */
char* get_absolute_path(char *filename, char *current_directory);

/* Strips off all but the local filename. */
char* get_local_name(char *filename);

/* Increments op_counter and handles rollover. */
unsigned int increment_op_counter(unsigned int op_count);

/* Searches for name in dir_inode. If found, the inode number is returned.
 * Else, returns -1. 
 * 'name' can refer to a file or directory, but it must
 * be a direct child of the directory to which dir_inode corresponds.
 */
int get_inode_num(inode *dir_inode, char *name);
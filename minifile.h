#ifndef __MINIFILE_H__
#define __MINIFILE_H__

#include "disk.h"
#include "defs.h"

#define SUPERBLOCK_MAGIC_NUM 8675309
#define INODE_RATIO_IN_DISK 10
#define DIRECT_PTRS_PER_INODE 11 // should be able to have more
#define MAX_CHARS_IN_FNAME 256
#define INODE_MAPS_PER_BLOCK 15
#define DIRECT_BLOCKS_PER_INDIRECT 1023
#define NULL_PTR -1
#define MINIMUM_DISK_SIZE 20

/*
 * Definitions for minifiles.
 *
 * You have to implement the fiunctions defined by this file that 
 * provide a filesystem interface for the minithread package.
 * You have to provide the implementation of these functions in
 * the file minifile.c
 */

typedef struct minifile* minifile_t;

//Block definitions.

//Superblock: Starting point in disk for the FS.
typedef struct superblock {
	union {

		struct {
			int magic_number;
			int disk_size;

			int root_inode;

			int first_free_inode;
			int first_free_data_block;
		} data;

		char padding[DISK_BLOCK_SIZE];

	};
} superblock_t;

//A block representing either a file or a directory.
typedef struct inode {
	union {

		struct {
			int idx;
			int type;
			int size;
			int parent_inode;
		
			int direct_ptrs[DIRECT_PTRS_PER_INODE];
			int indirect_ptr;
		} data;

		char padding[DISK_BLOCK_SIZE];

	};
} inode_t;

// A mapping of a single file or directory to an inode number.
typedef struct inode_mapping {
	char filename[MAX_CHARS_IN_FNAME];
	int inode_number;
} inode_mapping_t;

//A data block containing a mapping between name and child inodes in the parent directory.
typedef struct directory_data_block {
	union {

		struct {
			inode_mapping_t inode_map[INODE_MAPS_PER_BLOCK];
			int num_maps;
		} data;

		char padding[DISK_BLOCK_SIZE];

	};
} directory_data_block_t;

//A data block containing a mapping to the rest of the file and another indirect block.
typedef struct indirect_data_block {
	union {

		struct {
			int direct_ptrs[DIRECT_BLOCKS_PER_INDIRECT];
			int indirect_ptr;
		} data;

		char padding[DISK_BLOCK_SIZE];

	};
} indirect_data_block_t;

//A block interepreted as free by the FS to overwrite with either data or make an inode out of it.
typedef struct free_block {
	union {

		int next_free_block;

		char padding [DISK_BLOCK_SIZE];

	};
} free_block_t;


// Represents important data about the directory a process is currently in.
typedef struct dir_data {
	char *absolute_path;
	int inode_number;
} dir_data_t;

// Pointer to disk.
disk_t *minifile_disk;

/* 
 * General requiremens:
 *     If filenames and/or dirnames begin with a "/" they are absolute
 *     (the full path is specified). If they start with anything else
 *     they are relative (the full path is specified by the current 
 *     directory+filename).
 *
 *     All functions should return NULL or -1 to signal an error.
 */

/* Initializes the minifile system from input_disk. Returns 0 on 
 * success and -1 on error.
 */
int minifile_init(disk_t *input_disk);

/* 
 * Create a file. If the file exists its contents are truncated.
 * If the file doesn't exist it is created in the current directory.
 *
 * Returns a pointer to a structure minifile that can be used in
 *    subsequent calls to filesystem functions.
 */
minifile_t minifile_creat(char *filename);

/* 
 * Opens a file. If the file doesn't exist return NULL, otherwise
 * return a pointer to a minifile structure.
 *
 * Mode should be interpreted exactly in the manner fopen interprets
 * this argument (see fopen documentation).
 */
minifile_t minifile_open(char *filename, char *mode);

/* 
 * Reads at most maxlen bytes into buffer data from the current
 * cursor position. If there are no maxlen bytes until the end
 * of the file, it should read less (up to the end of file).
 *
 * Return: the number of bytes actually read or -1 if error.
 */
int minifile_read(minifile_t file, char *data, int maxlen);

/*
 * Writes len bytes to the current position of the cursor.
 * If necessary the file is lengthened (the new length should
 * be reflected in the inode).
 *
 * Return: 0 if everything is fine or -1 if error.
 */
int minifile_write(minifile_t file, char *data, int len);

/*
 * Closes the file. Should free the space occupied by the minifile
 * structure and propagate the changes to the file (both inode and 
 * data) to the disk.
 */
int minifile_close(minifile_t file);

/*
 * Deletes the file. The entry in the directory
 * where the file is listed is removed, the inode and the data
 * blocks are freed
 */
int minifile_unlink(char *filename);

/*
 * Creates a directory with the name dirname. 
 */
int minifile_mkdir(char *dirname);

/*
 * Removes an empty directory. Should return -1 if the directory is
 * not empty.
 */
int minifile_rmdir(char *dirname);

/* 
 * Returns information about the status of a file.
 * Returns the size of the file (possibly 0) if it is a regular file,
 * -1 if the file doesn't exist and -2 if it is a directory.
 */
int minifile_stat(char *path); 

/* Changes the current directory to path. The current directory is 
 * maintained individually for every thread and is the directory
 * to which paths not starting with "/" are relative to.
 */
int minifile_cd(char *path);

/*
 * Lists the contents of the directory path. The last pointer in the returned
 * array is set to NULL to indicate termination. This array and all its
 * constituent entries should be dynamically allocated and must be freed by 
 * the user.
 */
char **minifile_ls(char *path);

/*
 * Returns the current directory of the calling thread. This buffer should
 * be a dynamically allocated, null-terminated string that contains the current
 * directory. The caller has the responsibility to free up this buffer when done.
 */
char* minifile_pwd(void);


//Block API.

/*
* Creates a superblock with the proper magic number.
*/
superblock_t *minifile_create_superblock(int dsk_siz);

/*
* Creates the root inode with no data blocks.
*/
inode_t *minifile_create_root_inode();

/*
* Creates a free block (inode or data block) set with next_free_block.
*/
free_block_t *minifile_create_free_block(int next_free_block);




#endif /* __MINIFILE_H__ */

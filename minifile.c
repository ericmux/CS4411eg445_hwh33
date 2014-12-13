#include <stdlib.h>

#include "hashtable.h"
#include "minifile.h"
#include "disk.h"
#include "synch.h"

#define SUPERBLOCK_MAGIC_NUM 8675309
#define INODE_FRACTION_OF_DISK 0.1
#define DIRECT_PTRS_PER_INODE 11 // should be able to have more
#define MAX_CHARS_IN_FNAME 256
#define INODE_MAPS_PER_BLOCK 15
#define DIRECT_BLOCKS_PER_INDIRECT 1022
#define NULL_PTR -1

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
struct minifile {
	int inode_number;
	int cursor_position;
	int current_num_rws;
};

typedef struct superblock {
	union {

		struct {
			int magic_number;
			int disk_size;
			int last_op_written;

			int root_inode;

			int first_free_inode;
			int first_free_datablock;
		} data;

		char padding[DISK_BLOCK_SIZE];

	};
} superblock_t;

typedef struct inode {
	union {

		struct {
			int inode_type;
			int size;
			
			int direct_ptrs[DIRECT_PTRS_PER_INODE];
			int indirect_ptr;
		} data;

		char padding[DISK_BLOCK_SIZE];

	};
} inode_t;


typedef struct free_inode {
	union {

		int next_free_inode;

		char padding[DISK_BLOCK_SIZE];

	};
} free_inode_t;


// A mapping of a single file or directory to an inode number.
typedef struct inode_mapping {
	char filename[MAX_CHARS_IN_FNAME];
	int inode_number;
} inode_mapping_t;

typedef struct directory_data_block {
	union {

		struct {
			inode_mapping_t inode_map[INODE_MAPS_PER_BLOCK];
			int num_maps;
		} data;

		char padding[DISK_BLOCK_SIZE];

	};
} directory_data_block_t;

typedef struct free_data_block {
	union {

		int next_free_block;

		char padding [DISK_BLOCK_SIZE];

	};
} free_data_block_t;

typedef struct indirect_data_block {
	union {

		struct {
			int direct_ptrs[DIRECT_BLOCKS_PER_INDIRECT];
			int indirect_ptr;
			char num_direct_ptrs[4];
		} data;

		char padding[DISK_BLOCK_SIZE];

	};
} indirect_data_block_t;

// Represents important data about the directory a process is currently in.
struct dir_data {
	char *absolute_path;
	int inode_number;
};

// Pointer to disk.
disk_t *disk;

// Version of superblock in memory.
superblock_t superblock;

// Current operation number; rolls back over to zero when UINT_MAX is reached.
unsigned int current_op;

// Each block gets a lock to protect its metadata.
semaphore_t *metadata_locks;

// Maps process IDs to current working directories represented by dir_data structures.
hashtable_t thread_cd_map;


//Include util functions.
#include "minifile_utils.c"

int minifile_init(disk_t *input_disk) {
	int INIT_NUM_BUCKETS = 10;

	int i;
	int num_blocks;
	int request_result;

	disk = input_disk;

	// Initialize the superblock in memory. Check the magic number before proceeding.
	// Also initialize the current_op counter.
	request_result = disk_read_block(disk, 0, (char *)superblock);
	if (request_result == DISK_REQUEST_ERROR 
		|| superblock->data->magic_number != SUPERBLOCK_MAGIC_NUM) {
		return -1;
	}
	current_op = increment_op_counter(superblock->data->last_op_written);

	// Initialize the metadata locks.
	num_blocks = superblock->data->disk_size;
	metadata_locks = (semaphore_t *)malloc(num_blocks * sizeof(semaphore_t));
	for (i = 0; i < num_blocks; i++) {
		metadata_locks[i] = sempahore_create();
		semaphore_initialize(metadata_locks[i], 1);
	}

	thread_cd_map = hashtable_create_specify_buckets(INIT_NUM_BUCKETS);

	return -1;
}

minifile_t minifile_creat(char *filename){
	char *parent_path;
	char *abs_filename;
	dir_data *parent_dir_data;
	inode *new_inode;
	inode *parent_inode;
	int file_inode_number;
	minifile_t new_minifile;

	int i;
	int request_result;

	// Get the file's parent directory and the file's absolute path (if
	// the path is already absolute, abs_filename will equal filename).
	parent_path = get_parent_path(filename);
	abs_filename = get_absolute_path(filename, parent_path);

	// Create the file's inode.
	new_inode = (inode *)malloc(sizeof(struct inode));
	new_inode->data->inode_type = FILE;
	new_inode->data->size = 0;
	for (i = 0; i < DIRECT_PTRS_PER_INODE; i++) {
		new_inode->data->direct_ptrs[i] = NULL_PTR;
	}
	new_inode->indirect_ptr = NULL_PTR;

	// Check to see if this file exists in the current directory. If so, we'll overwrite 
	// that inode block with our new one. If not, get the number of the first free inode.
	file_inode_number = get_inode_num(parent_path, filename);
	if (file_inode_number == -1) {
		file_inode_number = get_free_inode();
	}

	// Write the inode and, when finished, free it.
	disk_write_block(disk, file_inode_number, (char *)new_inode);
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

	return -1;
}

char **minifile_ls(char *path){

	return NULL;
}

char* minifile_pwd(void){

	return NULL;
}

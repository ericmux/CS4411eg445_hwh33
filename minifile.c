#include <stdlib.h>

#include "minifile.h"
#include "minifile_utils.h"
#include "disk.h"
#include "synch.h"

#define SUPERBLOCK_MAGIC_NUM 8675309
#define INODE_FRACTION_OF_DISK 0.1
#define DIRECT_PTRS_PER_INODE 11
#define MAX_CHARS_IN_FNAME 256
#define INODE_MAPS_PER_BLOCK 15
#define DIRECT_BLOCKS_PER_INDIRECT 1022
#define NULL_PTR -1

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
  int cursor_position;
  int current_num_rws;
};

typedef struct superblock {
	union {

		struct {
			int magic_number;
			int disk_size;

			int root_inode;

			int first_free_inode;
			int first_free_datablock;
		} data;

		char padding[DISK_BLOCK_SIZE]

	};
} superblock_t;

struct inode {
	union {

		struct {
			int inode_type;
			int size;
			
			int direct_ptrs[DIRECT_PTRS_PER_INODE];
			int indirect_ptr;
		} data;

		char padding[DISK_BLOCK_SIZE];

	};
};

struct free_inode {
	union {

		int next_free_inode;

		char padding[DISK_BLOCK_SIZE];

	};
};

// A mapping of a single file or directory to an inode number.
struct inode_mapping {
	char filename[MAX_CHARS_IN_FNAME];
	int inode_number;
};

struct directory_data_block {
	union {

		struct {
			inode_mapping inode_map[INODE_MAPS_PER_BLOCK];
			int num_maps;
		} data;

		char padding[DISK_BLOCK_SIZE];

	};
};

struct free_data_block {
	union {

		int next_free_block;

		char padding [DISK_BLOCK_SIZE];

	};
};

struct indirect_data_block {
	union {

		struct {
			int direct_blocks[DIRECT_BLOCKS_PER_INDIRECT];
			int indirect_ptr;
			char num_direct_blocks[4];
		} data;

		char padding[DISK_BLOCK_SIZE];

	};
};

disk_t *disk;

superblock_t superblock;

// These locks ensure that only one thread can edit metadata on a given 
// block at any time.
semaphore_t *metadata_locks;

hashtable_t thread_cd_map;

void minifile_init(disk_t *input_disk) {
	int INIT_NUM_BUCKETS = 10;

	int i;
	int num_blocks;

	disk = input_disk;

	// Initialize the superblock in memory.
	disk_read_block(disk, 0, (char *)superblock);
	num_blocks = superblock->disk_size;

	// Initialize the metadata locks.
	metadata_locks = (semaphore_t *)malloc(num_blocks * sizeof(semaphore_t));
	for (i = 0; i < num_blocks; i++) {
		metadata_locks[i] = sempahore_create();
		semaphore_initialize(metadata_locks[i], 1);
	}

	thread_cd_map = hashtable_create_specify_buckets(INIT_NUM_BUCKETS);
}

minifile_t minifile_creat(char *filename){
	superblock_t superblock;
	char *current_directory;

	// Get the process's current working directory.
	hashtable_get(thread_cd_map, minithread_id(), &current_directory);

	// If the file name is local, adjust it to be absolute.
	if (filename[0] != '/') {
		// filename = 
	}



}

minifile_t minifile_open(char *filename, char *mode){

}

int minifile_read(minifile_t file, char *data, int maxlen){

}

int minifile_write(minifile_t file, char *data, int len){

}

int minifile_close(minifile_t file){

}

int minifile_unlink(char *filename){

}

int minifile_mkdir(char *dirname){

}

int minifile_rmdir(char *dirname){

}

int minifile_stat(char *path){

} 

int minifile_cd(char *path){

}

char **minifile_ls(char *path){

}

char* minifile_pwd(void){

}

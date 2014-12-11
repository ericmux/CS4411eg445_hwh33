#include <stdlib.h>

#include "minifile.h"
#include "linked_list.h"
#include "minifile_utils.h"
#include "disk.h"

#define INODE_FRACTION_OF_DISK 0.1
#define DIRECT_PTRS_PER_INODE 11
#define MAX_CHARS_IN_FNAME 256
#define INODE_MAPS_PER_BLOCK 15
#define DIRECT_BLOCKS_PER_INDIRECT 1022

typedef enum {
	FILE = 0,
	DIRECTORY = 1
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
			char magic_number[4];
			char disk_size[4];

			char root_inode[4];

			char first_free_inode[4];
			char first_free_datablock[4];
		} data;

		char padding[DISK_BLOCK_SIZE]

	}
} superblock_t;

struct inode {
	union {

		struct {
			char inode_type;
			char size[4];
			
			char direct_ptrs[DIRECT_PTRS_PER_INODE][4];
			char indirect_ptr[4];
		} data;

		char padding[DISK_BLOCK_SIZE];

	}
};

struct free_inode {
	union {

		int next_free_inode;

		char padding[DISK_BLOCK_SIZE];

	}
}

// A mapping of a single file or directory to an inode number.
struct inode_mapping {
	char filename[MAX_CHARS_IN_FNAME];
	char inode_number[4];
};

struct directory_data_block {
	union {

		struct {
			// There are MAX_CHARS_IN_FNAME + 4 bytes in an inode mapping.
			char inode_map[INODE_MAPS_PER_BLOCK][MAX_CHARS_IN_FNAME + 4];
			char num_maps[4];
		} data;

		char padding[DISK_BLOCK_SIZE];

	}
};

struct free_data_block {
	union {

		char next_free_block[4];

		char padding [DISK_BLOCK_SIZE];

	}
};

struct indirect_data_block {
	union {

		struct {
			char direct_blocks[DIRECT_BLOCKS_PER_INDIRECT][4];
			char indirect_ptr[4];
			char num_direct_blocks[4];
		} data;

		char padding[DISK_BLOCK_SIZE];
		
	}
};

disk_t *disk;

// The elements of inode_map are inode_mappings.
linked_list_t inode_map;

linked_list_t inodes;

superblock_t superblock;

inode *current_inode;

int current_inode_number;

int current_datablock_number;

hashtable_t thread_cd_map

// called by mkfs?
void minifile_init() {
	inode_map = linked_list_create();
	inodes = linked_list_create();

	// initialize disk?

	// Create the root inode and write it to datablock 1.
	inode *root_inode = (inode *)malloc(sizeof(struct inode));
	root_inode->inode_type = DIRECTORY;
	root_inode->inode_number = 0;
	root_inode->block_number = 1;
	disk_write_block(disk, 1, (char *)root_inode);

	// Create the super block and write it to datablock 0.
	superblock = (superblock_t) malloc(sizeof(struct superblock));
	superblock->first_free_inode_datablock = //??
	superblock->first_free_datablock = //??
	superblock->root_inode_datablock = 1;
	disk_write_block(disk, 0, (char *)superblock);

	current_inode_number = 1;
	current_inode = (inode *)malloc(sizeof(struct inode));
}

minifile_t minifile_creat(char *filename){
	// If the file name is local, adjust it to be absolute.
	if (filename[0] != '/') {
		filename = get_absolute_path(filename)
	}

	// Create a new inode for the file.
	next_inode = current_inode->next_free_inode;
	next_inode->inode_type = FILE;
	next_inode->next_free_inode = //??

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

#include <stdlib.h>

#include "minifile.h"
#include "linked_list.h"
#include "minifile_utils.h"
#include "disk.h"

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

struct inode {
	int inode_type;
	int inode_number;
	int block_number;
	inode *next_free_inode
	int blocknums[12]; // the last element is the blocknum of the indirect block
};

typedef struct superblock {
	int first_free_inode_datablock;
	int first_free_datablock;
	int root_inode_datablock;
} superblock_t;

// A mapping of a single file or directory to an inode number.
struct inode_mapping {
	char filename[256];
	int inode_number;
};

disk_t *disk;

// The elements of inode_map are inode_mappings.
linked_list_t inode_map;

linked_list_t inodes;

superblock_t superblock;

inode *current_inode;

int current_inode_number;

int current_datablock_number;

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

/* File: ext2.c
 * By: Andrew Holbrook
 *
 * custom ext2 filesystem reader
 */

// standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

// linux headers
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

// custom headers
#include "ext2.h"

// static function prototypes
static inline int is_delim(char *str, char delim);

// is_delim(str, delim) - check if string contains only delim.
static inline int
is_delim(char *str, char delim)
{
	unsigned int i;

	for (i = 0; i < strlen(str); ++i) {
		if (str[i] != delim) {
			return 0;
		}
	}

	return 1;
}

// open_ext2fs(ext2fs, fs_info) - open ext2fs for reading
// fs_info structure is filled in with important filesystem information
int
open_ext2fs(char *ext2fs, ext2_info *fs_info)
{
	int fd;
	void *ptr;
	super_block *sb;

	// attempt to open ext2fs -- this may fail if file is too large (>2GB)
	fd = open(ext2fs, O_RDONLY);
	if (fd == -1) {
		return -1;
	}

	fs_info->fd = fd;

	ptr = mmap(NULL, 2048, PROT_READ, MAP_PRIVATE, fd, 0);
	if (!ptr) {
		close(fd);
		return -1;
	}

	sb = (super_block *)(ptr + 1024);

	if (sb->magic != EXT2_SUPER_MAGIC) {
		close(fd);
		return -1;
	}

	fs_info->block_size = 1024 << sb->log_block_size;
	fs_info->size = sb->blocks_count * fs_info->block_size;

	munmap(ptr, 2048);

	// remap with size of filesystem
	ptr = mmap(NULL, fs_info->size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (!ptr) {
		close(fd);
		return -1;
	}

	fs_info->ptr = ptr;
	fs_info->sb = (super_block *)(ptr + 1024);

	fs_info->gd = (group_desc *)(ptr + (fs_info->sb->first_data_block + 1) *
		fs_info->block_size);

	return 0;
}

// close_ext2fs(fs_info) - cleanup
void
close_ext2fs(ext2_info *fs_info)
{
	munmap(fs_info->ptr, fs_info->size);
	close(fs_info->fd);
}

// get_inode(fs_info, inode_num) - locate inode based on inode number
inode_table *
get_inode(ext2_info *fs_info, unsigned int inode_num)
{
	unsigned int blk_grp;
	unsigned int idx;
	group_desc *gd;
	inode_table *itbl;

	if (!inode_num) {
		return NULL;
	}

	blk_grp = (inode_num - 1) / fs_info->sb->inodes_per_group;
	idx = (inode_num - 1) % fs_info->sb->inodes_per_group;

	gd = fs_info->gd + blk_grp;

	itbl = (inode_table *)(fs_info->ptr + gd->inode_table * fs_info->block_size);
	itbl = ((void *)itbl) + idx * fs_info->sb->inode_size;

	return itbl;
}

// find_inode_number(fs_info, path) - find inode associated with last token in
// path
unsigned int
find_inode_number(ext2_info *fs_info, char *path)
{
	inode_table *root;
	inode_table *cur_inode;
	dir_entry *dent;
	char *path_copy;
	char *blocks;
	char *tmp;
	uint32_t inode_num = 0;

	// return root inode number if only delim character is found
	if (is_delim(path, '/')) {
		return EXT2_ROOT_INO;
	}

	root = get_inode(fs_info, EXT2_ROOT_INO);

	// strtok requires a non-const char string
	path_copy = (char *)malloc(strlen(path) + 1);

	strcpy(path_copy, path);

	// loop through tokens ("/home/wowzer" ; tokens = ["home", "wowzer])
	// find inode for each token until the end is reached -- return the inode
	// number.
	cur_inode = root;
	tmp = strtok(path_copy, "/");
	while (tmp) {
		blocks = (char *)malloc(cur_inode->blocks * 512);
		get_data(fs_info, cur_inode, blocks);

		inode_num = 0;
		dent = (dir_entry *)blocks;
		while (dent->file_type != EXT2_FT_UNKNOWN) {
			char *tmp_c = (char *)calloc(dent->name_len + 1, 1);
			memcpy(tmp_c, dent->name, dent->name_len);

			if (strcmp(tmp_c, tmp) == 0) {
				inode_num = dent->inode;
				cur_inode = get_inode(fs_info, dent->inode);
				break;
			}

			free(tmp_c);
			dent = ((void *)dent) + dent->rec_len;
		}

		free(blocks);
		tmp = strtok(NULL, "/");
	}

	free(path_copy);

	return inode_num;
}

// get_data(fs_info, inode, data) - copy data stored in inode to data.
void
get_data(ext2_info *fs_info, inode_table *inode, void *data)
{
	unsigned int i;
	uint32_t *block;

	for (i = 0; i < inode->blocks / (fs_info->block_size >> 9); ++i) {
		if (i < 12) { // direct blocks
			memcpy(data + fs_info->block_size * i,
				   fs_info->ptr + fs_info->block_size * inode->block[i],
				   fs_info->block_size);
		} else if (i < 268) { // indirect blocks
			block = fs_info->ptr + fs_info->block_size * inode->block[12];
			memcpy(data + fs_info->block_size * i,
				   fs_info->ptr + fs_info->block_size * block[i - 12],
				   fs_info->block_size);
		} else { // doubly-indirect blocks
			block = fs_info->ptr + fs_info->block_size * inode->block[13];
			block = fs_info->ptr + fs_info->block_size * block[(i - 268) >> 8];
			memcpy(data + fs_info->block_size * i,
				   fs_info->ptr + fs_info->block_size * block[(i - 268) & 255],
				   fs_info->block_size);
		}
	}
}

// get_path_from_link(fs_info, inode, p) - copy path from symbolic link inode
// into p.
void
get_path_from_link(ext2_info *fs_info, inode_table *inode, char *p)
{
	unsigned int i;

	if (!ISLINK(inode->mode)) {
		return;
	}

	// symlinks <= 60 are stored where blocks would be.
	if (inode->size <= 60) {
		for (i = 0; i < inode->size; i += 4) {
			memcpy(p + i, &inode->block[i >> 2], 4);
		}
	} else {
		get_data(fs_info, inode, p);
	}
}

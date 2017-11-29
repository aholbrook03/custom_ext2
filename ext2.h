/* File: ext2.h
 * By: Andrew Holbrook
 *
 * custom ext2 filesystem reader
 */

#pragma once
#ifndef __EXT2_H__
#define __EXT2_H__

#include <inttypes.h>

#define EXT2_SUPER_MAGIC	0xEF53

#define EXT2_GOOD_OLD_REV	0
#define EXT2_DYNAMIC_REV	1

#define EXT2_FT_UNKNOWN		0
#define EXT2_FT_REG_FILE	1
#define EXT2_FT_DIR			2
#define EXT2_FT_CHRDEV		3
#define EXT2_FT_BLKDEV		4
#define EXT2_FT_SYMLINK		7

#define EXT2_ROOT_INO		2

#define EXT2_S_IFREG		0x8000
#define EXT2_S_IFDIR		0x4000
#define EXT2_S_IFLNK		0xA000

#define EXT2_S_IRUSR	0x0100
#define EXT2_S_IWUSR	0x0080
#define EXT2_S_IXUSR	0x0040
#define EXT2_S_IRGRP	0x0020
#define EXT2_S_IWGRP	0x0010
#define EXT2_S_IXGRP	0x0008
#define EXT2_S_IROTH	0x0004
#define EXT2_S_IWOTH	0x0002
#define EXT2_S_IXOTH	0x0001

// macros for checking filetype
#define ISDIR(mode) ((mode & EXT2_S_IFDIR) == EXT2_S_IFDIR)
#define ISFILE(mode) ((mode & EXT2_S_IFREG) == EXT2_S_IFREG)
#define ISLINK(mode) ((mode & EXT2_S_IFLNK) == EXT2_S_IFLNK)

// macros for checking permissions
#define ISIRUSR(mode)	(mode & EXT2_S_IRUSR)
#define ISIWUSR(mode)	(mode & EXT2_S_IWUSR)
#define ISIXUSR(mode)	(mode & EXT2_S_IXUSR)
#define ISIRGRP(mode)	(mode & EXT2_S_IRGRP)
#define ISIWGRP(mode)	(mode & EXT2_S_IWGRP)
#define ISIXGRP(mode)	(mode & EXT2_S_IXGRP)
#define ISIROTH(mode)	(mode & EXT2_S_IROTH)
#define ISIWOTH(mode)	(mode & EXT2_S_IWOTH)
#define ISIXOTH(mode)	(mode & EXT2_S_IXOTH)

typedef struct _super_block
{
	uint32_t inodes_count;
	uint32_t blocks_count;
	uint32_t r_blocks_count;
	uint32_t free_blocks_count;
	uint32_t free_inodes_count;
	uint32_t first_data_block;
	uint32_t log_block_size;
	uint32_t log_frag_size;
	uint32_t blocks_per_group;
	uint32_t frags_per_group;
	uint32_t inodes_per_group;
	uint32_t mtime;
	uint32_t wtime;
	uint16_t mnt_count;
	uint16_t max_mnt_count;
	uint16_t magic;
	uint16_t state;
	uint16_t errors;
	uint16_t minor_rev_level;
	uint32_t lastcheck;
	uint32_t checkinterval;
	uint32_t creator_os;
	uint32_t rev_level;
	uint16_t def_resuid;
	uint16_t def_resgid;
	uint32_t first_ino;
	uint16_t inode_size;
	uint16_t pad[467];
} super_block;

typedef struct _group_desc
{
	uint32_t block_bitmap;
	uint32_t inode_bitmap;
	uint32_t inode_table;
	uint16_t free_blocks_count;
	uint16_t free_inodes_count;
	uint16_t used_dirs_count;
	uint16_t pad;
	uint32_t reserved[3];
} group_desc;

typedef struct _inode_table
{
	uint16_t mode;
	uint16_t uid;
	uint32_t size;
	uint32_t atime;
	uint32_t ctime;
	uint32_t mtime;
	uint32_t dtime;
	uint16_t gid;
	uint16_t links_count;
	uint32_t blocks;
	uint32_t flags;
	uint32_t osd1;
	uint32_t block[15];
	uint32_t pad[7];
} inode_table;

typedef struct _dir_entry
{
	uint32_t inode;
	uint16_t rec_len;
	uint8_t name_len;
	uint8_t file_type;
	char name[255];
} dir_entry;

typedef struct _ext2_info
{
	int32_t fd;
	uint32_t block_size;
	size_t size;
	super_block *sb;
	group_desc *gd;
	void *ptr;
} ext2_info;

// function prototypes
int open_ext2fs(char *ext2fs, ext2_info *fs_info);
void close_ext2fs(ext2_info *fs_info);
inode_table * get_inode(ext2_info *fs_info, unsigned int inode_num);
unsigned int find_inode_number(ext2_info *fs_info, char *path);
void get_data(ext2_info *fs_info, inode_table *inode, void *data);
void get_path_from_link(ext2_info *fs_info, inode_table *inode, char *p);

#endif // __EXT2_H__

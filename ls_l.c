/* File: ls_l.c
 * By: Andrew Holbrook
 *
 * Implementation of "ls" shell command
 */

#include <stdio.h>
#include <malloc.h>
#include <time.h>
#include <string.h>

#include <unistd.h>

#include "ext2.h"

static void print_perm(uint16_t mode);
int listdir(ext2_info *fs_info, char *path);

int
main(int argc, char **argv)
{
	ext2_info fs_info;

	if (argc != 3) {
		fprintf(stderr, "Usage: ls_l extfs path\n");
		return -1;
	}

	if (open_ext2fs(argv[1], &fs_info)) {
		fprintf(stderr, "Error opening filesystem!\n");
		fprintf(stderr, "ext2 file may be too large (>2GB)\n");
		return -1;
	}

	if (listdir(&fs_info, argv[2])) {
		fprintf(stderr, "directory not found!\n");
		return -1;
	}

	close_ext2fs(&fs_info);

	return 0;
}

static void
print_perm(uint16_t mode)
{
	size_t rv;

	if (ISIRUSR(mode)) {
		rv = write(STDOUT_FILENO, "r", 1);
	} else {
		rv = write(STDOUT_FILENO, "-", 1);
	}

	if (ISIWUSR(mode)) {
		rv = write(STDOUT_FILENO, "w", 1);
	} else {
		rv = write(STDOUT_FILENO, "-", 1);
	}

	if (ISIXUSR(mode)) {
		rv = write(STDOUT_FILENO, "x", 1);
	} else {
		rv = write(STDOUT_FILENO, "-", 1);
	}

	if (ISIRGRP(mode)) {
		rv = write(STDOUT_FILENO, "r", 1);
	} else {
		rv = write(STDOUT_FILENO, "-", 1);
	}

	if (ISIWGRP(mode)) {
		rv = write(STDOUT_FILENO, "w", 1);
	} else {
		rv = write(STDOUT_FILENO, "-", 1);
	}

	if (ISIXGRP(mode)) {
		rv = write(STDOUT_FILENO, "x", 1);
	} else {
		rv = write(STDOUT_FILENO, "-", 1);
	}

	if (ISIROTH(mode)) {
		rv = write(STDOUT_FILENO, "r", 1);
	} else {
		rv = write(STDOUT_FILENO, "-", 1);
	}

	if (ISIWOTH(mode)) {
		rv = write(STDOUT_FILENO, "w", 1);
	} else {
		rv = write(STDOUT_FILENO, "-", 1);
	}

	if (ISIXOTH(mode)) {
		rv = write(STDOUT_FILENO, "x ", 2);
	} else {
		rv = write(STDOUT_FILENO, "- ", 2);
	}
}

int
listdir(ext2_info *fs_info, char *path)
{
	inode_table *inode;
	inode_table *tmp;
	dir_entry *dent;
	void *data;
	size_t rv;

	inode = get_inode(fs_info, find_inode_number(fs_info, path));
	if (!inode) {
		return -1;
	}

	if (ISDIR(inode->mode)) {
		data = malloc(inode->blocks * 512);
		get_data(fs_info, inode, data);

		dent = (dir_entry *)data;
		while ((void *)dent <= (data + inode->blocks * 512)) {
			if (dent->file_type != EXT2_FT_UNKNOWN) {
				if (dent->file_type == EXT2_FT_REG_FILE) {
					rv = write(STDOUT_FILENO, "-", 1);
				} else if (dent->file_type == EXT2_FT_DIR) {
					rv = write(STDOUT_FILENO, "d", 1);
				} else if (dent->file_type == EXT2_FT_SYMLINK) {
					rv = write(STDOUT_FILENO, "l", 1);
				} else if (dent->file_type == EXT2_FT_CHRDEV) {
					rv = write(STDOUT_FILENO, "c", 1);
				} else if (dent->file_type == EXT2_FT_BLKDEV) {
					rv = write(STDOUT_FILENO, "b", 1);
				}

				tmp = get_inode(fs_info, dent->inode);

				print_perm(tmp->mode);

				printf("%u\t", tmp->links_count);

				printf("%u\t%u\t", tmp->uid, tmp->gid);
				printf("%u\t", tmp->blocks);
				fflush(stdout);

				rv = write(STDOUT_FILENO, ctime((time_t *)&tmp->mtime),
					strlen(ctime((time_t *)&tmp->mtime)) - 1);

				rv = write(STDOUT_FILENO, " ", 1);

				rv = write(STDOUT_FILENO, dent->name, dent->name_len);

				if (dent->file_type == EXT2_FT_SYMLINK) {
					char *tmp_c;
					if (tmp->size <= 60) {
						unsigned int i;

						tmp_c = (char *)malloc(tmp->size);

						for (i = 0; i < tmp->size; i += 4) {
							memcpy(tmp_c + i, &tmp->block[i / 4], 4);
						}
					} else {
						tmp_c= (char *)malloc(tmp->blocks * 512);
						get_data(fs_info, tmp, tmp_c);
					}

					rv = write(STDOUT_FILENO, " -> ", 4);
					rv = write(STDOUT_FILENO, tmp_c, tmp->size);

					free(tmp_c);
				}

				rv = write(STDOUT_FILENO, "\n", 1);
			}

			dent = ((void *)dent) + dent->rec_len;
		}

		free(data);
	} else {
		return -1;
	}

	return 0;
}

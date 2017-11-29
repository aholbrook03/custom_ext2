/* File: cat.c
 * By: Andrew Holbrook
 *
 * custom "cat" shell command
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ext2.h"

int
main (int argc, char **argv)
{
	ext2_info fs_info;
	inode_table *inode;
	void *data;
	int fd = -1;

	if (argc != 3 && argc != 4) {
		fprintf(stderr, "Usage: cat extfs file [output file]\n");
		return -1;
	}

	if (argc == 4) {
		fd = open(argv[3], O_RDWR | O_CREAT, EXT2_S_IRUSR | EXT2_S_IRGRP |
			EXT2_S_IWUSR);

		if (fd == -1) {
			fprintf(stderr, "Can't create file: %s\n", argv[3]);
			return -1;
		}

		fd = dup2(fd, STDOUT_FILENO);
		if (fd == -1) {
			fprintf(stderr, "Can't create file: %s\n", argv[3]);
			return -1;
		}
	}

	if (open_ext2fs(argv[1], &fs_info)) {
		fprintf(stderr, "Error opening filesystem!\n");
		fprintf(stderr, "ext2 file may be too large (>2GB)\n");
		return -1;
	}

	inode = get_inode(&fs_info, find_inode_number(&fs_info, argv[2]));
	if (!inode) {
		fprintf(stderr, "file not found!\n");
		return -1;
	}

	while (ISLINK(inode->mode)) {
		char *tmp_c;

		if (inode->size <= 60) {
			tmp_c = (char *)malloc(inode->size + 1);
			memset(tmp_c, 0, inode->size + 1);
		} else {
			tmp_c = (char *)malloc(inode->blocks * 512);
			memset(tmp_c, 0, inode->blocks * 512);
		}

		get_path_from_link(&fs_info, inode, tmp_c);

		inode = get_inode(&fs_info, find_inode_number(&fs_info, tmp_c));
		if (!inode) {
			fprintf(stderr, "file not found!\n");
			return -1;
		}
	}

	if (ISFILE(inode->mode)) {
		size_t rv;

		data = malloc(inode->blocks * 512);

		get_data(&fs_info, inode, data);

		rv = write(STDOUT_FILENO, data, inode->size);

		free(data);
	} else {
		fprintf(stderr, "not a file\n");
		return -1;
	}

	close_ext2fs(&fs_info);

	if (fd != -1) {
		close(fd);
	}

	return 0;
}

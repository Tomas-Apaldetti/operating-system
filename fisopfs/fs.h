#ifndef _FISOP_FS_
#define _FISOP_FS_

#include <time.h>
#include <stdint.h>
#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>


#define KiB 1024
#define MiB (1024 * KiB)

#define BLOCK_SIZE (32 * KiB)

#define MAX_FILE_NAME 256
#define MAX_PATH_LEN 2048

#define MAX_INODE_BLOCK_PTR 13
#define MAX_DIRECT_BLOCK_COUNT 12
#define MAX_INDIRECT_BLOCK_COUNT BLOCK_SIZE / sizeof(int)
#define MAX_BLOCK_COUNT MAX_DIRECT_BLOCK_COUNT + MAX_INDIRECT_BLOCK_COUNT

#define INODE_ONE_LI_COUNT 13
#define INODE_INDIRECT_BLOCK_IDX 12

#define DENTRY_EMPTY 0

#define DIR_TYPE_MODE                                                          \
	(__S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#define REG_TYPE_MODE                                                          \
	(__S_IFREG | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)


#define P_OX 0x1
#define P_OW 0x2
#define P_OR 0x4

#define P_GX 0x1 << 3
#define P_GW 0x2 << 3
#define P_GR 0x4 << 3

#define P_UX 0x1 << 6
#define P_UW 0x2 << 6
#define P_UR 0x4 << 6

#define FS_FILE 0x1 << 9
#define FS_DIR 0x2 << 9
#define FS_LINK 0x4 << 9

typedef char byte;
typedef struct superblock {
	// Struct amount
	int inode_amount;
	int data_blocks_amount;
	int block_size;

	// Block idx
	int inode_start;
	int data_start;

	// Inode idx
	ino_t root_inode;
} superblock_t;

typedef struct inode {
	mode_t type_mode;
	uid_t user_id;
	gid_t group_id;

	// Time info
	time_t last_access;
	time_t last_modification;
	time_t created_date;

	// Inode data
	size_t size;
	nlink_t link_count;
	int blocks[MAX_INODE_BLOCK_PTR];  // 12 directos, 1 indirecto
	blkcnt_t block_amount;
} inode_t;

typedef struct dentry {
	ino_t inode_number;
	char file_name[MAX_FILE_NAME];
} dentry_t;

typedef int (*dentry_iterator)(dentry_t *, void *);

void init_fs(void);

int new_inode(const char *path, mode_t mode, inode_t **out);

int search_inode(const char *path, inode_t **out);

int iterate_over_dir(const inode_t *inode, dentry_iterator func, void *param);

bool dir_is_empty(inode_t *inode);

int truncate_inode(inode_t *inode, off_t size);

int fiuba_write(inode_t *inode, const char *buf, size_t size, off_t offset);

int fiuba_read(inode_t *inode, char *buffer, size_t size, off_t offset);

int fiuba_readdir(inode_t *inode, void *buffer, fuse_fill_dir_t filler);

int fiuba_rmv_inode(const char *path, inode_t *inode_to_rmv, ino_t inode_to_rmv_n);

int fiuba_access(inode_t *inode, int mask);

int deserialize(int fd);

int serialize(int fd);

#endif  // _FISOP_FS_

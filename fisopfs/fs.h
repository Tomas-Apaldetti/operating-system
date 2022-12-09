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
#define MiB 1024 * KiB

#define BLOCK_SIZE 32 * KiB

#define MAX_FILE_NAME 256

#define MAX_INODE_BLOCK_PTR 13
#define MAX_DIRECT_BLOCK_COUNT 12
#define INODE_ONE_LI_COUNT 13
#define INODE_ONE_LI_INDEX 12

#define DENTRY_EMPTY 0

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
	int related_block[MAX_INODE_BLOCK_PTR];  // 12 directos, 1 indirecto
	blkcnt_t block_count;

	// Optimization
	ino_t next_free;
} inode_t;

typedef struct dentry {
	ino_t inode_number;
	char file_name[MAX_FILE_NAME];
} dentry_t;

typedef bool (*dentry_iterator)(dentry_t *, const char *);

void init_fs(void);

inode_t *get_inode(ino_t);

int search_inode(const char *path, inode_t **out);

int destroy_inode(const char *path);

int dir_is_empty(inode_t *inode);

ino_t
iterate_over_dir(const inode_t *inode, const char *dir_name, dentry_iterator f);

long fiuba_write(inode_t *inode, const char *buf, size_t size, off_t offset);

long fiuba_read(const inode_t *inode, char *buffer, size_t size, off_t offset);

int deserialize(int fd);

int serialize(int fd);


// void set_type(inode_t *inode, mode_t type);

#endif  // _FISOP_FS_

#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "fs.h"

#define RWRWR (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)

#define FILE_MODE RWRWR
#define DIR_MODE (__S_IFDIR | RWRWR)

#define PERSIST_FILE_NAME 3

int persist();


static int fisopfs_getattr(const char *path, struct stat *st);

static int fisopfs_mknod(const char *path, mode_t mode, dev_t rdev);

static int fisopfs_mkdir(const char *path, mode_t mode);

static int fisopfs_unlink(const char *path);

static int fisopfs_rmdir(const char *path);

static int fisopfs_rename(const char *from, const char *to);

static int fisopfs_truncate(const char *path, off_t size);

static int fisopfs_open(const char *path, struct fuse_file_info *fi);

static int fisopfs_read(const char *path,
                        char *buffer,
                        size_t size,
                        off_t offset,
                        struct fuse_file_info *fi);

static int fisopfs_write(const char *path,
                         const char *buf,
                         size_t size,
                         off_t offset,
                         struct fuse_file_info *fi);

static int fisopfs_flush(const char *path, struct fuse_file_info *fi);

static int fisopfs_opendir(const char *path, struct fuse_file_info *fi);

static int fisopfs_readdir(const char *path,
                           void *buffer,
                           fuse_fill_dir_t filler,
                           off_t offset,
                           struct fuse_file_info *fi);

static void *fisopfs_init(struct fuse_conn_info *conn);

static void fisopfs_destroy(void *private_data);

static int fisopfs_access(const char *path, int mask);

static int fisopfs_create(const char *path, mode_t mode, struct fuse_file_info *fi);

static int
fisopfs_ftruncate(const char *path, off_t size, struct fuse_file_info *fi);

static int fisopfs_fgetattr(const char *path,
                            struct stat *stbuf,
                            struct fuse_file_info *fi);

static int fisopfs_utimens(const char *path, const struct timespec tv[2]);

int load_persist();

//========

static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf("[debug] fisopfs_getattr with: %s\n", path);

	inode_t *inode;
	int result = search_inode(path, &inode);
	if (result < 0)
		return result;

	st->st_mode = inode->type_mode;
	st->st_uid = inode->user_id;
	st->st_gid = inode->group_id;

	st->st_size = inode->size;
	st->st_blocks = inode->block_amount;

	st->st_atime = inode->last_access;
	st->st_mtime = inode->last_modification;
	st->st_ctime = inode->last_modification;

	return 0;
}

static int
fisopfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
	printf("[debug] fisopfs_mknod with: %s \n", path);

	inode_t *inode;
	int result;

	result = search_inode(path, &inode);
	if (result > 0)
		return -EEXIST;

	result = new_inode(path, mode, &inode);
	if (result < 0)
		return result;
	return EXIT_SUCCESS;
}

static int
fisopfs_mkdir(const char *path, mode_t mode)
{
	printf("[debug] fisopfs_mkdir \n");
	return fisopfs_mknod(path, DIR_TYPE_MODE | mode, 0);
}

static int
fisopfs_unlink(const char *path)
{
	printf("[debug] fisopfs_unlink \n");

	inode_t *inode_to_rmv;
	ino_t inode_to_rmv_n;

	int result = search_inode(path, &inode_to_rmv);
	if (result == 0)
		return -ENOENT;
	else if (result < 0)
		return result;
	inode_to_rmv_n = result;

	if (S_ISDIR(inode_to_rmv->type_mode))
		return -EISDIR;

	return fiuba_rmv_inode(path, inode_to_rmv, inode_to_rmv_n);
}

static int
fisopfs_rmdir(const char *path)
{
	printf("[debug] fisopfs_rmdir \n");

	inode_t *inode_to_rmv;
	ino_t inode_to_rmv_n;

	int result = search_inode(path, &inode_to_rmv);
	if (result == 0)
		return -ENOENT;
	else if (result < 0)
		return result;
	inode_to_rmv_n = result;

	if (!S_ISDIR(inode_to_rmv->type_mode))
		return -ENOTDIR;
	if (!dir_is_empty(inode_to_rmv))
		return -ENOTEMPTY;

	return fiuba_rmv_inode(path, inode_to_rmv, inode_to_rmv_n);
}

static int
fisopfs_truncate(const char *path, off_t size)
{
	printf("[debug] fisopfs_truncate \n");

	inode_t *inode;
	int result = search_inode(path, &inode);
	if (result < 0)
		return result;
	return truncate_inode(inode, size);
}

static int
fisopfs_open(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_open \n");

	if (fi->flags & O_CREAT)
		return fisopfs_create(path, REG_TYPE_MODE, fi);

	return fisopfs_access(path, F_OK);
}

static int
fisopfs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_read \n");

	inode_t *inode;
	int result = search_inode(path, &inode);
	if (result < 0)
		return result;
	return fiuba_read(inode, buffer, size, offset);
}

static int
fisopfs_write(const char *path,
              const char *buf,
              size_t size,
              off_t offset,
              struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_write with: %s \n", path);

	inode_t *inode;
	int result = search_inode(path, &inode);
	if (result < 0)
		return result;
	if (S_ISDIR(inode->type_mode))
		return -EINVAL;
	return fiuba_write(inode, buf, size, offset);
}

static int
fisopfs_flush(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_flush \n");

	int result = persist();
	if (result) {
		printf("Can't persist the FS");
	}
	return 0;
}

static int
fisopfs_opendir(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_opendir with: %s \n", path);

	if (fi->flags & O_CREAT)
		return fisopfs_create(path, DIR_TYPE_MODE, fi);

	return fisopfs_access(path, F_OK);
}

static int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_readdir with: %s \n", path);

	inode_t *inode;

	int result = search_inode(path, &inode);
	if (result < 0)
		return result;

	return fiuba_readdir(inode, buffer, filler);
}

static void *
fisopfs_init(struct fuse_conn_info *conn)
{
	printf("[debug] fisopfs_init \n");

	int result = load_persist();
	if (result < 0)
		init_fs();

	return (void *) 0;
}

static void
fisopfs_destroy(void *private_data)
{
	printf("[debug] fisopfs_destroy \n");

	int result = persist();
	if (result) {
		printf("Can't persist the FS\n");
	}
}

static int
fisopfs_access(const char *path, int mask)
{
	printf("[debug] fisopfs_access with: %s\n", path);

	inode_t *inode;
	int result;

	result = search_inode(path, &inode);
	if (result < 0)
		return -ENOENT;

	return fiuba_access(inode, mask);
}

static int
fisopfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_create\n");

	int result = fisopfs_access(path, F_OK);
	if (result == EXIT_SUCCESS)
		return -EEXIST;

	return fisopfs_mknod(path, mode, 0);
}

static int
fisopfs_ftruncate(const char *path, off_t size, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_ftruncate \n");
	return fisopfs_truncate(path, size);
}

static int
fisopfs_fgetattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_fgetattr \n");
	return fisopfs_getattr(path, stbuf);
}

static int
fisopfs_utimens(const char *path, const struct timespec tv[2])
{
	printf("[debug] fisopfs_utimens \n");

	inode_t *inode;
	int result;

	result = search_inode(path, &inode);
	if (result < 0)
		return -ENOENT;

	inode->last_access = tv[0].tv_sec;
	inode->last_modification = tv[1].tv_sec;

	return EXIT_SUCCESS;
}


static struct fuse_operations operations = {
	// Nota: no todos son necesarios. Checkear con test de uso comunes
	// Otra cosita es que por ahora solo hice que todo retornara -ENOENT pero
	// desp hay que ver de devolver el error que corresultponda cuando toque solamente.

	// NOTA IMPORTANTE: para debuggear en otra terminal correr con ./fisopfs -f ./dir_de_prueba/

	// NOTA IMPORATANTE 2: Abajo comenté open, opendir, init, destroy, access,
	// create y read_buf para que al menos permita realizar un cd (en debug) como el
	// visto de la clase y un cat al pseudo-archivo hardcodeado que tiene adentro
	// el root del filesystem. Son las funciones que deberian ser implementadas
	// de primero porque sino no deja hacer absolutamente nada con el dir root de el filesystem montado al ejecutar.
	// Lo mejor para ir viendo cuales implementar es ir probando con modo
	// debug, ahi se registran todas las llamadas.

	.init = fisopfs_init,

	.getattr = fisopfs_getattr,   .access = fisopfs_access,
	.utimens = fisopfs_utimens,

	.mknod = fisopfs_mknod,       .unlink = fisopfs_unlink,
	.truncate = fisopfs_truncate, .open = fisopfs_open,
	.read = fisopfs_read,         .write = fisopfs_write,

	.mkdir = fisopfs_mkdir,       .rmdir = fisopfs_rmdir,
	.opendir = fisopfs_opendir,   .readdir = fisopfs_readdir,

	.flush = fisopfs_flush,

	.destroy = fisopfs_destroy,
};

int
load_persist()
{
	int result = open("fs_state.fisopfs", O_RDONLY);
	if (result < 0)
		return -EIO;

	result = deserialize(result);
	if (result < 0) {
		close(result);
		return -EIO;
	};

	close(result);
	return 0;
}

int
persist()
{
	int result = open("fs_state.fisopfs", O_WRONLY | O_CREAT, REG_TYPE_MODE);
	if (result < 0)
		return -EIO;

	result = serialize(result);
	if (result < 0) {
		close(result);
		return -EIO;
	};

	close(result);
	return 0;
}

int
main(int argc, char *argv[])
{
	for (int i = 0; i < argc; i++) {
		printf("%s\n", argv[i]);
	}

	return fuse_main(argc, argv, &operations, NULL);
}

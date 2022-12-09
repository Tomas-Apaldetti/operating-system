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

static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf("[debug] fisopfs_getattr with: %s\n", path);

	inode_t *inode;
	int response = search_inode(path, &inode);
	if (response != 0)
		return response;

	st->st_mode = inode->type_mode;
	st->st_uid = inode->user_id;
	st->st_gid = inode->group_id;

	st->st_size = inode->size;
	st->st_blocks = inode->block_count;

	st->st_atime = inode->last_access;
	st->st_mtime = inode->last_modification;
	st->st_ctime = inode->last_modification;

	return 0;
}

static int
fisopfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
	inode_t* inode;
	int result = new_inode(path, mode, &inode);
	if (result < 0) return result;
	return 0;
}

static int
fisopfs_mkdir(const char *path, mode_t mode)
{
	inode_t* inode;
	int result = new_inode(path, mode, &inode);
	if (result < 0) return result;
	return 0;
}

static int
fisopfs_unlink(const char *path)
{
	return fiuba_unlink(path);
}

static int
fisopfs_rmdir(const char *path)
{
	inode_t* inode;
	int res = search_inode(res, &inode);
	if (res < 0) return res;
	int is_empty = dir_is_empty(inode);
	if (is_empty < 0) return is_empty;
	if (is_empty) return fiuba_unlink(path);
	return -EINVAL;
}

static int
fisopfs_rename(const char *from, const char *to)
{
	printf("[debug] fisopfs_rename \n");
	return -ENOENT;
}

static int
fisopfs_truncate(const char *path, off_t size)
{
	inode_t* inode;
	int res = search_inode(path, &inode);
	if (res < 0) return res;
	return truncate_inode(inode, size);
}

static int
fisopfs_open(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_open \n");
	return -ENOENT;
}

static int
fisopfs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	// TODO: check permissions
	inode_t* inode;
	int res = search_inode(path, &inode);
	if (res < 0) return res;
	return fiuba_read(inode, buffer, size, offset);
}

static int
fisopfs_write(const char *path,
              const char *buf,
              size_t size,
              off_t offset,
              struct fuse_file_info *fi)
{
	// TODO: check permissions
	inode_t* inode;
	int res = search_inode(path, &inode);
	if (res < 0) return res;
	if (S_ISDIR(inode->type_mode))
        return -EINVAL; 
	return fiuba_write(inode, buf, size, offset);
}

static int
fisopfs_statfs(const char *path, struct statvfs *stbuf)
{
	printf("[debug] fisopfs_statfs \n");
	return -ENOENT;
}

static int
fisopfs_flush(const char *path, struct fuse_file_info *fi)
{
	// I don't think this have to be done here, but oh well.
	int res = persist();
	if (res){
		printf("Can't persist the FS");
	}
	return res;
}

static int
fisopfs_release(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_release \n");
	return -ENOENT;
}

static int
fisopfs_fsync(const char *path, int isdatasync, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_fsync \n");
	return -ENOENT;
}

static int
fisopfs_opendir(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_opendir with: %s \n", path);
	printf("[debug] fisopfs_opendir flags: %i \n", fi->flags);

	inode_t *inode;
	return search_inode(path, &inode);  // ver de devolver el numero de inodo
}

static int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	// printf("[debug] fisopfs_readdir(%s) \n", path);

	// inode_t *inode;

	// int response = search_inode(path, &inode);
	// if (response != 0)
	// 	return response;

	// return fiuba_read_dir(inode, buffer, filler);
	return 0;
}


static int
fisopfs_releasedir(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_releasedir with: %s\n", path);
	printf("[debug] fisopfs_releasedir flags: %i \n", fi->flags);
	return 0;
}

static int
fisopfs_fsyncdir(const char *path, int isdatasync, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_fsyncdir \n");
	return -ENOENT;
}

static void *
fisopfs_init(struct fuse_conn_info *conn)
{
	return (void *) 0;
}

static void
fisopfs_destroy(void *private_data)
{
	int res = persist();
	if (res){
		printf("Can't persist the FS");
	}
}

static int
fisopfs_access(const char *path,
               int mask)  // en la docu de hmc aparece mode como mask, aunque no
                          // se corresponde con la syscall normal (donde figura mode)
{
	printf("[debug] fisopfs_access with: %s\n", path);
	return 0;
}

static int
fisopfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_create \n");
	return -ENOENT;
}

static int
fisopfs_ftruncate(const char *path, off_t size, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_ftruncate \n");
	return -ENOENT;
}

static int
fisopfs_fgetattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_fgetattr \n");


	return -ENOENT;
}

static int
fisopfs_lock(const char *path, struct fuse_file_info *fi, int cmd, struct flock *locks)
{
	printf("[debug] fisopfs_lock \n");
	return -ENOENT;
}

static int
fisopfs_utimens(const char *path, const struct timespec tv[2])
{
	printf("[debug] fisopfs_utimens \n");
	return -ENOENT;
}

static int
fisopfs_bmap(const char *path,
             size_t blocksize,
             uint64_t *idx)  // idx es el blockno (docu hmc)
{
	printf("[debug] fisopfs_bmap \n");
	return -ENOENT;
}

static int
fisopfs_ioctl(const char *path,
              int cmd,
              void *arg,
              struct fuse_file_info *fi,
              unsigned int flags,
              void *data)
{
	printf("[debug] fisopfs_ioctl \n");
	return -ENOENT;
}

static int
fisopfs_poll(const char *path,
             struct fuse_file_info *fi,
             struct fuse_pollhandle *ph,
             unsigned *reventsp)
{
	printf("[debug] fisopfs_poll \n");
	return -ENOENT;
}

static int
fisopfs_write_buf(const char *path,
                  struct fuse_bufvec *buf,
                  off_t off,
                  struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_write_buf \n");
	return -ENOENT;
}

static int
fisopfs_read_buf(const char *path,
                 struct fuse_bufvec **bufp,
                 size_t size,
                 off_t off,
                 struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_read_buf \n");
	return -ENOENT;
}

static int
fisopfs_flock(const char *path, struct fuse_file_info *fi, int op)
{
	printf("[debug] fisopfs_flock \n");
	return -ENOENT;
}

static int
fisopfs_fallocate(const char *path,
                  int mode,
                  off_t offset,
                  off_t len,
                  struct fuse_file_info *fi)  // args basados en man pq no hay mas en la docu
{
	printf("[debug] fisopfs_fallocate \n");
	return -ENOENT;
}


static struct fuse_operations operations = {
	// Nota: no todos son necesarios. Checkear con test de uso comunes
	// Otra cosita es que por ahora solo hice que todo retornara -ENOENT pero
	// desp hay que ver de devolver el error que corresponda cuando toque solamente.

	// NOTA IMPORTANTE: para debuggear en otra terminal correr con ./fisopfs -f ./dir_de_prueba/

	// NOTA IMPORATANTE 2: Abajo comenté open, opendir, init, destroy, access,
	// create y read_buf para que al menos permita realizar un cd (en debug) como el
	// visto de la clase y un cat al pseudo-archivo hardcodeado que tiene adentro
	// el root del filesystem. Son las funciones que deberian ser implementadas
	// de primero porque sino no deja hacer absolutamente nada con el dir root de el filesystem montado al ejecutar.
	// Lo mejor para ir viendo cuales implementar es ir probando con modo
	// debug, ahi se registran todas las llamadas.

	.init = fisopfs_init,

	.getattr = fisopfs_getattr,
	.access = fisopfs_access,
	.utimens = fisopfs_utimens,

	.mknod = fisopfs_mknod,
	.unlink = fisopfs_unlink,
	.rename = fisopfs_rename,
	.truncate = fisopfs_truncate,
	.open = fisopfs_open,
	.read = fisopfs_read,
	.write = fisopfs_write,
	.release = fisopfs_release,

	.mkdir = fisopfs_mkdir,
	.rmdir = fisopfs_rmdir,
	.opendir = fisopfs_opendir,
	.readdir = fisopfs_readdir,
	.releasedir = fisopfs_releasedir,

	.statfs = fisopfs_statfs,
	.flush = fisopfs_flush,
	.fsync = fisopfs_fsync,

	.lock = fisopfs_lock,  // Prolly not

	.write_buf = fisopfs_write_buf,  // What is the difference
	.read_buf = fisopfs_read_buf,

	.fallocate = fisopfs_fallocate,  // Probably not, but not too difficult

	.destroy = fisopfs_destroy,
};

int
load_persist(const char* file_name){
	int res = open(file_name, O_RDONLY);
	if (res < 0) return -EIO;
	res = deserialize(res);
	if (res < 0){
		close(res);
		return -EIO;
	};
	close(res);
	return 0;
}

int
persist()
{
	// TODO: save it in argv?
	int res = open("~/.fisopfs/fisopfs", O_WRONLY | O_CREAT, 664);
	if (res < 0) return -EIO;
	res = serialize(res);
	if (res < 0){
		close(res);
		return -EIO;
	};
	close(res);
	return 0;
}

int
main(int argc, char *argv[])
{
	for (int i = 0; i < argc; i++) {
		printf("%s\n", argv[i]);
	}
	init_fs();
	if(argc >= PERSIST_FILE_NAME){
		if (load_persist < 0) {
			perror("Error while trying to load the persisted information");
			return -EIO;
		}
	}
	return fuse_main(argc, argv, &operations, NULL);
}

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

static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf("[debug] fisopfs_getattr(%s)\n", path);

	if (strcmp(path, "/") == 0) {
		st->st_uid = 1717;
		st->st_mode = __S_IFDIR | 0755;
		st->st_nlink = 2;
	} else if (strcmp(path, "/fisop") == 0) {
		st->st_uid = 1818;
		st->st_mode = __S_IFREG | 0644;
		st->st_size = 2048;
		st->st_nlink = 1;
	} else {
		return -ENOENT;
	}

	return 0;
}


static int
fisopfs_readlink(const char *path, char *buf, size_t size)
{
	printf("[debug] fisopfs_readlink \n");
	return -ENOENT;
}

static int
fisopfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
	printf("[debug] fisopfs_mknod \n");
	return -ENOENT;
}

static int
fisopfs_mkdir(const char *path, mode_t mode)
{
	printf("[debug] fisopfs_mkdir \n");
	return -ENOENT;
}

static int
fisopfs_unlink(const char *path)
{
	printf("[debug] fisopfs_unlink \n");
	return -ENOENT;
}

static int
fisopfs_rmdir(const char *path)
{
	printf("[debug] fisopfs_rmdir \n");
	return -ENOENT;
}

static int
fisopfs_symlink(const char *to, const char *from)
{
	printf("[debug] fisopfs_symlink \n");
	return -ENOENT;
}

// los parametros de symlink estan invertidos con respecto a los demas pero es asi segun la docu

static int
fisopfs_rename(const char *from, const char *to)
{
	printf("[debug] fisopfs_rename \n");
	return -ENOENT;
}

static int
fisopfs_link(const char *from, const char *to)
{
	printf("[debug] fisopfs_link \n");
	return -ENOENT;
}

static int
fisopfs_chmod(const char *path, mode_t mode)
{
	printf("[debug] fisopfs_chmod \n");
	return -ENOENT;
}

static int
fisopfs_chown(const char *path, uid_t uid, gid_t gid)
{
	printf("[debug] fisopfs_chown \n");
	return -ENOENT;
}

static int
fisopfs_truncate(const char *path, off_t size)
{
	printf("[debug] fisopfs_truncate \n");
	return -ENOENT;
}

static int
fisopfs_open(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_open \n");
	return -ENOENT;
}

#define MAX_CONTENIDO
static char fisop_file_contenidos[MAX_CONTENIDO] = "hola fisopfs!\n";

static int
fisopfs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_read(%s, %lu, %lu)\n", path, offset, size);

	// Solo tenemos un archivo hardcodeado!
	if (strcmp(path, "/fisop") != 0)
		return -ENOENT;


	if (offset + size > strlen(fisop_file_contenidos))
		size = strlen(fisop_file_contenidos) - offset;

	size = size > 0 ? size : 0;

	strncpy(buffer, fisop_file_contenidos + offset, size);

	return size;
}

static int
fisopfs_write(const char *path,
              const char *buf,
              size_t size,
              off_t offset,
              struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_write \n");
	return -ENOENT;
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
	printf("[debug] fisopfs_flush \n");
	return -ENOENT;
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

// static int
// fisopfs_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
// {
// 	printf("[debug] fisopfs_setxattr \n");
//  return -ENOENT;
// }

// static int
// fisopfs_getxattr(const char *path, const char *name, char *value, size_t size)
// {
// 	printf("[debug] fisopfs_getxattr \n");
//  return -ENOENT;
// }

// static int
// fisopfs_listxattr(const char *path, char *list, size_t size)
// {
// 	printf("[debug] fisopfs_listxattr \n");
//  return -ENOENT;
// }

// static int
// fisopfs_removexattr(const char *path, const char *name)
// {
// 	printf("[debug] fisopfs_removexattr \n");
//  return -ENOENT;
// }

static int
fisopfs_opendir(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_opendir \n");
	return -ENOENT;
}

static int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_readdir(%s) \n", path);

	// Los directorios '.' y '..'
	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);

	// Si nos preguntan por el directorio raiz, solo tenemos un archivo
	if (strcmp(path, "/") == 0) {
		filler(buffer, "fisop", NULL, 0);
		return 0;
	}

	return -ENOENT;
}


static int
fisopfs_releasedir(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_releasedir \n");
	return -ENOENT;
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
	printf("[debug] fisopfs_init \n");
	return (void *) -ENOENT;
}

static void
fisopfs_destroy(void *private_data)
{
	printf("[debug] fisopfs_destroy \n");
}

static int
fisopfs_access(const char *path,
               int mask)  // en la docu de hmc aparece mode como mask, aunque no
                          // se corresponde con la syscall normal (donde figura mode)
{
	printf("[debug] fisopfs_access \n");
	return -ENOENT;
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

	.getattr = fisopfs_getattr,  // <--- traida en el esqueleto

	.readlink = fisopfs_readlink,
	.mknod = fisopfs_mknod,
	.mkdir = fisopfs_mkdir,
	.unlink = fisopfs_unlink,
	.rmdir = fisopfs_rmdir,
	// .symlink = fisopfs_symlink, // [no obligatorio]
	.rename = fisopfs_rename,
	// .link = fisopfs_link, // [no obligatorio]
	// .chmod = fisopfs_chmod, // [no obligatorio]
	// .chown = fisopfs_chown, // [no obligatorio]
	.truncate = fisopfs_truncate,
	//.open = fisopfs_open,

	.read = fisopfs_read,  // <--- traida en el esqueleto

	.write = fisopfs_write,
	.statfs = fisopfs_statfs,
	.flush = fisopfs_flush,
	.release = fisopfs_release,
	.fsync = fisopfs_fsync,

	//.setxattr = fisopfs_setxattr, // no creo que se necesiten estos 4
	//.getxattr = fisopfs_getxattr,
	//.listxattr = fisopfs_listxattr,
	//.removexattr = fisopfs_removexattr,

	//.opendir = fisopfs_opendir,

	.readdir = fisopfs_readdir,  // <--- traida en el esqueleto

	.releasedir = fisopfs_releasedir,
	.fsyncdir = fisopfs_fsyncdir,
	// .init = fisopfs_init,
	// .destroy = fisopfs_destroy,
	// .access = fisopfs_access,
	// .create = fisopfs_create,
	.ftruncate = fisopfs_ftruncate,
	.fgetattr = fisopfs_fgetattr,
	.lock = fisopfs_lock,
	.utimens = fisopfs_utimens,
	.bmap = fisopfs_bmap,
	.ioctl = fisopfs_ioctl,
	.poll = fisopfs_poll,
	.write_buf = fisopfs_write_buf,
	//.read_buf = fisopfs_read_buf,
	.flock = fisopfs_flock,
	.fallocate = fisopfs_fallocate,
};

int
main(int argc, char *argv[])
{
	for (int i = 0; i < argc; i++) {
		printf("%s\n", argv[i]);
	}
	return fuse_main(argc, argv, &operations, NULL);
}

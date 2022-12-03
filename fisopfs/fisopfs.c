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
fisopfs_readlink(const char *, char *, size_t)
{
	printf("[debug] fisopfs_readlink");
}

static int
fisopfs_mknod(const char *, mode_t, dev_t)
{
	printf("[debug] fisopfs_mknod");
}

static int
fisopfs_mkdir(const char *, mode_t)
{
	printf("[debug] fisopfs_mkdir");
}

static int
fisopfs_unlink(const char *)
{
	printf("[debug] fisopfs_unlink");
}

static int
fisopfs_rmdir(const char *)
{
	printf("[debug] fisopfs_rmdir");
}

static int
fisopfs_symlink(const char *, const char *)
{
	printf("[debug] fisopfs_symlink");
}

static int
fisopfs_rename(const char *, const char *, unsigned int flags)
{
	printf("[debug] fisopfs_rename");
}

static int
fisopfs_link(const char *, const char *)
{
	printf("[debug] fisopfs_link");
}

static int
fisopfs_chmod(const char *, mode_t, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_chmod");
}

static int
fisopfs_chown(const char *, uid_t, gid_t, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_chown");
}

static int
fisopfs_truncate(const char *, off_t, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_truncate");
}

static int
fisopfs_open(const char *, struct fuse_file_info *)
{
	printf("[debug] fisopfs_open");
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
fisopfs_write(const char *, const char *, size_t, off_t, struct fuse_file_info *)
{
	printf("[debug] fisopfs_write");
}

static int
fisopfs_statfs(const char *, struct statvfs *)
{
	printf("[debug] fisopfs_statfs");
}

static int
fisopfs_flush(const char *, struct fuse_file_info *)
{
	printf("[debug] fisopfs_flush");
}

static int
fisopfs_release(const char *, struct fuse_file_info *)
{
	printf("[debug] fisopfs_release");
}

static int
fisopfs_fsync(const char *, int, struct fuse_file_info *)
{
	printf("[debug] fisopfs_fsync");
}

static int
fisopfs_setxattr(const char *, const char *, const char *, size_t, int)
{
	printf("[debug] fisopfs_setxattr");
}

static int
fisopfs_getxattr(const char *, const char *, char *, size_t)
{
	printf("[debug] fisopfs_getxattr");
}

static int
fisopfs_listxattr(const char *, char *, size_t)
{
	printf("[debug] fisopfs_listxattr");
}

static int
fisopfs_removexattr(const char *, const char *)
{
	printf("[debug] fisopfs_removexattr");
}

static int
fisopfs_opendir(const char *, struct fuse_file_info *)
{
	printf("[debug] fisopfs_opendir");
}

static int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_readdir(%s)", path);

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
fisopfs_releasedir(const char *, struct fuse_file_info *)
{
	printf("[debug] fisopfs_releasedir");
}

static int
fisopfs_fsyncdir(const char *, int, struct fuse_file_info *)
{
	printf("[debug] fisopfs_fsyncdir");
}

static void *
fisopfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
	printf("[debug] fisopfs_init");
}

static void
fisopfs_destroy(void *private_data)
{
	printf("[debug] fisopfs_destroy");
}

static int
fisopfs_access(const char *, int)
{
	printf("[debug] fisopfs_access");
}

static int
fisopfs_create(const char *, mode_t, struct fuse_file_info *)
{
	printf("[debug] fisopfs_create");
}

static int
fisopfs_lock(const char *, struct fuse_file_info *, int cmd, struct flock *)
{
	printf("[debug] fisopfs_lock");
}

static int
fisopfs_utimens(const char *, const struct timespec tv[2], struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_utimens");
}

static int
fisopfs_bmap(const char *, size_t blocksize, uint64_t *idx)
{
	printf("[debug] fisopfs_bmap");
}

static int
fisopfs_ioctl(const char *,
              unsigned int cmd,
              void *arg,
              struct fuse_file_info *,
              unsigned int flags,
              void *data)
{
	printf("[debug] fisopfs_ioctl");
}

static int
fisopfs_poll(const char *,
             struct fuse_file_info *,
             struct fuse_pollhandle *ph,
             unsigned *reventsp)
{
	printf("[debug] fisopfs_poll");
}

static int
fisopfs_write_buf(const char *,
                  struct fuse_bufvec *buf,
                  off_t off,
                  struct fuse_file_info *)
{
	printf("[debug] fisopfs_write_buf");
}

static int
fisopfs_read_buf(const char *,
                 struct fuse_bufvec **bufp,
                 size_t size,
                 off_t off,
                 struct fuse_file_info *)
{
	printf("[debug] fisopfs_read_buf");
}

static int
fisopfs_flock(const char *, struct fuse_file_info *, int op)
{
	printf("[debug] fisopfs_flock");
}

static int
fisopfs_fallocate(const char *, int, off_t, off_t, struct fuse_file_info *)
{
	printf("[debug] fisopfs_fallocate");
}


static struct fuse_operations operations = {
	// nota: no todos son necesarios. Checkear con test de uso comunes
	.getattr = fisopfs_getattr,  // <--- traida en el esqueleto
	.readlink = fisopfs_readlink,
	.mknod = fisopfs_mknod,
	.mkdir = fisopfs_mkdir,
	.unlink = fisopfs_unlink,
	.rmdir = fisopfs_rmdir,
	.symlink = fisopfs_symlink,
	.rename = fisopfs_rename,
	.link = fisopfs_link,
	.chmod = fisopfs_chmod,
	.chown = fisopfs_chown,
	.truncate = fisopfs_truncate,
	.open = fisopfs_open,
	.read = fisopfs_read,  // <--- traida en el esqueleto
	.write = fisopfs_write,
	.statfs = fisopfs_statfs,
	.flush = fisopfs_flush,
	.release = fisopfs_release,
	.fsync = fisopfs_fsync,
	//.setxattr = fisopfs_setxattr,
	//.getxattr = fisopfs_getxattr,
	//.listxattr = fisopfs_listxattr,
	//.removexattr = fisopfs_removexattr,
	.opendir = fisopfs_opendir,
	.readdir = fisopfs_readdir,  // <--- traida en el esqueleto
	.releasedir = fisopfs_releasedir,
	.fsyncdir = fisopfs_fsyncdir,
	.init = fisopfs_init,
	.destroy = fisopfs_destroy,
	.access = fisopfs_access,
	.create = fisopfs_create,
	.lock = fisopfs_lock,
	.utimens = fisopfs_utimens,
	.bmap = fisopfs_bmap,
	.ioctl = fisopfs_ioctl,
	.poll = fisopfs_poll,
	.write_buf = fisopfs_write_buf,
	.read_buf = fisopfs_read_buf,
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

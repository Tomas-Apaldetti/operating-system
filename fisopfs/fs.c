#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "fs.h"

#define SUPERBLOCK ((superblock_t *) blocks)
#define BLOCK(n) (void *) ((byte *) blocks + BLOCK_SIZE * (n))
#define DATA_BITMAP ((bool *) (SUPERBLOCK + 1))
#define INODE_BITMAP                                                           \
	((bool *) (DATA_BITMAP + ((SUPERBLOCK)->data_blocks_amount)))
#define INODE_BLOCKS BLOCK(SUPERBLOCK->inode_start)
#define DATA_BLOCKS BLOCK(SUPERBLOCK->data_start)

#define EMPTY_DIRENTRY 0
#define ROOT_INODE 1

#define BLOCK_AMOUNT 6300
// INODE_SIZE 256
#define INODE_AMOUNT 6300
#define INODE_BLOCK_N 50
// Assuming 256b inode size, each blocks holds 128 inodes.
// Therefore, to completely map each block with at least one inode we need ~49,2
// blocks of inodes; rounding it to 50. Due that we have more inodes that we
// need, we can use the first one to mark that a dentry is empty (inode 0)
#define FS_SIZE                                                                \
	(1 * BLOCK_SIZE + INODE_BLOCK_N * BLOCK_SIZE + BLOCK_AMOUNT * BLOCK_SIZE)

#define MAX_BLOCK_WRITE 20


void init_inode(inode_t *inode, mode_t mode);

inode_t *get_inode_n(int inode_nmb);

void init_data_blocks_dir(inode_t *directory, const ino_t dot, const ino_t dotdot);

void *get_inode_block(const inode_t *inode, int block_number);

int insert_direntry_if_free(dentry_t *curr, void *_new_entry);

int search_parent_directory(const char *path, inode_t **out);

ino_t search_dir_inode_n(const inode_t *directory, const char *inode_name);

int search_free_inode(mode_t mode, inode_t **out);

void deallocate_block(int block_n);

void *get_block_n(int block_n);

ino_t unlink_inode(const inode_t *inode, const char *dir_name);

bool is_free_dentry(dentry_t *dentry);

void notify_access(inode_t *inode);

void notify_modif(inode_t *inode);

int remove_dentry(dentry_t *entry, void *_dir_name);

int is_dentry_searched(dentry_t *dentry, void *_dir_name);

int search_free_block();

byte blocks[FS_SIZE] = { 0 };  // ~200 MiB

int last_used_block = 0;


/*  =============================================================
 *  ========================== INIT =============================
 *  =============================================================
 */
void
init_superblock()
{
	superblock_t *superblock = SUPERBLOCK;

	superblock->inode_amount = INODE_AMOUNT;
	superblock->data_blocks_amount = BLOCK_AMOUNT;
	superblock->block_size = BLOCK_SIZE;

	superblock->inode_start = 1;
	superblock->data_start = INODE_BLOCK_N + 1;

	superblock->root_inode = ROOT_INODE;
}

void
init_bitmaps()
{
	superblock_t *superblock = SUPERBLOCK;
	bool *data_bitmap = DATA_BITMAP;
	memset(data_bitmap, false, superblock->data_blocks_amount);
	bool *inode_bitmap = INODE_BITMAP;
	memset(inode_bitmap, false, superblock->inode_amount);
	inode_bitmap[0] = true;
}

void
init_root_inode()
{
	inode_t *root_inode = get_inode_n(SUPERBLOCK->root_inode);

	root_inode->link_count = 1;

	init_inode(root_inode, DIR_TYPE_MODE);
	init_data_blocks_dir(root_inode, ROOT_INODE, ROOT_INODE);

	INODE_BITMAP[ROOT_INODE] = true;
}

void
init_fs()
{
	init_superblock();
	init_bitmaps();
	init_root_inode();
}

/*  =============================================================
 *  ========================= INODES ============================
 *  =============================================================
 */

void
init_inode(inode_t *inode, mode_t mode)
{
	time_t curr_time = time(NULL);

	inode->type_mode = mode;
	inode->user_id = getuid();
	inode->group_id = getgid();

	inode->last_access = curr_time;
	inode->last_modification = curr_time;
	inode->created_date = curr_time;

	inode->size = 0;
	inode->link_count = 1;
	inode->block_amount = 0;
}

int
add_inode_to_directory(inode_t *parent_inode,
                       ino_t parent_inode_n,
                       inode_t *new_inode,
                       ino_t new_inode_n,
                       char *new_inode_name)
{
	dentry_t new_dentry = { .inode_number = new_inode_n };
	strcpy(new_dentry.file_name, new_inode_name);

	int could_insert_in_the_middle = iterate_over_dir(
	        parent_inode, insert_direntry_if_free, (void *) &new_dentry);
	if (!could_insert_in_the_middle) {
		int write_result = fiuba_write(parent_inode,
		                               (char *) &new_dentry,
		                               sizeof(dentry_t),
		                               parent_inode->size);
		if (write_result < 0) {
			return write_result;
		}
	}

	if (S_ISDIR(new_inode->type_mode))
		init_data_blocks_dir(new_inode, new_inode_n, parent_inode_n);

	return EXIT_SUCCESS;
}

int
extract_last_name_offset(const char *path)
{
	int pathlen = strlen(path);
	int i = pathlen - 1;
	while ((i > 0) && (path[i] != '/')) {
		i--;
	}
	return i + 1;
}

int
new_inode(const char *path, mode_t mode, inode_t **out)
{
	inode_t *parent_inode;
	ino_t parent_inode_n;

	inode_t *new_inode;
	ino_t new_inode_n;

	int new_inode_path_offset;

	int result;

	size_t pathlen = strlen(path);
	if (pathlen > MAX_PATH_LEN)
		return -ENAMETOOLONG;

	result = search_parent_directory(path, &parent_inode);
	if (result < 0)
		return result;
	parent_inode_n = result;

	result = search_free_inode(mode, &new_inode);
	if (result < 0)
		return result;
	new_inode_n = result;


	new_inode_path_offset = extract_last_name_offset(path);
	result = add_inode_to_directory(parent_inode,
	                                parent_inode_n,
	                                new_inode,
	                                new_inode_n,
	                                (char *) path + new_inode_path_offset);
	if (result < 0) {
		INODE_BITMAP[new_inode_n] = false;
		return result;
	}

	(*out) = new_inode;
	return EXIT_SUCCESS;
}

inode_t *
get_inode_n(int inode_nmb)
{
	inode_t *inode = BLOCK(SUPERBLOCK->inode_start);
	return inode + inode_nmb;
}

void *
get_inode_indirect_block(const inode_t *inode, int block_n)
{
	int *indirect_block =
	        get_block_n(inode->blocks[INODE_INDIRECT_BLOCK_IDX]);
	int new_index = block_n - INODE_INDIRECT_BLOCK_IDX;
	return get_block_n(indirect_block[new_index]);
}

void *
get_inode_block(const inode_t *inode, int block_n)
{
	if (inode->block_amount <= block_n)
		return NULL;

	if (block_n >= INODE_INDIRECT_BLOCK_IDX) {
		return get_inode_indirect_block(inode, block_n);
	}

	return get_block_n(inode->blocks[block_n]);
}

int
get_inode_indirect_block_n(const inode_t *inode, int block_n)
{
	int *indirect_block =
	        get_block_n(inode->blocks[INODE_INDIRECT_BLOCK_IDX]);
	int new_index = block_n - INODE_INDIRECT_BLOCK_IDX;
	return indirect_block[new_index];
}

int
get_inode_block_n(const inode_t *inode, int block_n)
{
	if (inode->block_amount <= block_n)
		return -EINVAL;

	if (block_n >= INODE_INDIRECT_BLOCK_IDX) {
		return get_inode_indirect_block_n(inode, block_n);
	}

	return inode->blocks[block_n];
}

int
search_free_inode(mode_t mode, inode_t **out)
{
	superblock_t *superblock = SUPERBLOCK;
	bool *inode_bitmap = INODE_BITMAP;
	for (int i = ROOT_INODE + 1; i < superblock->inode_amount; i++) {
		if (!inode_bitmap[i]) {
			inode_bitmap[i] = true;
			(*out) = get_inode_n(i);
			init_inode((*out), mode);
			return i;
		}
	}
	return -ENOMEM;
}

void
deallocate_blocks_from_inode(inode_t *inode, int new_block_amount)
{
	int block_n;

	for (; inode->block_amount > new_block_amount; inode->block_amount--) {
		block_n = get_inode_block_n(inode, inode->block_amount - 1);
		deallocate_block(block_n);
	}
}

void
deallocate_inode(inode_t *inode, ino_t inode_n)
{
	deallocate_blocks_from_inode(inode, 0);
	INODE_BITMAP[inode_n] = false;
}

void
substract_link(inode_t *inode_to_rmv, ino_t inode_to_rmv_n)
{
	inode_to_rmv->link_count--;
	if (inode_to_rmv->link_count == 0)
		deallocate_inode(inode_to_rmv, inode_to_rmv_n);
}

int
fill_with_null_bytes(inode_t *inode, size_t size)
{
	char buffer[KiB] = { 0 };
	size_t size_to_fill = KiB;

	off_t offset = size - inode->size;

	int result;

	if (offset < size_to_fill)
		size_to_fill = offset;

	while (offset > 0) {
		result = fiuba_write(inode, buffer, offset, inode->size);
		if (result < 0)
			return result;

		offset -= size_to_fill;

		if (offset < size_to_fill)
			size_to_fill = offset;
	}

	return EXIT_SUCCESS;
}

int
truncate_inode(inode_t *inode, off_t size)
{
	if (inode->size < size)
		return fill_with_null_bytes(inode, size);

	int block_n = size / BLOCK_SIZE;
	block_n += size % BLOCK_SIZE != 0 ? 1 : 0;

	deallocate_blocks_from_inode(inode, block_n);

	inode->size = size;
	return EXIT_SUCCESS;
}

int
remove_from_parent(const char *path)
{
	inode_t *parent_inode;

	int name_offset;

	int result;

	result = search_parent_directory(path, &parent_inode);
	if (result == 0)
		return -ENOENT;
	if (result < 0)
		return result;

	name_offset = extract_last_name_offset(path);

	result = unlink_inode(parent_inode, path + name_offset);
	if (result == 0)
		return -ENOENT;

	return result;
}

int
fiuba_rmv_inode(const char *path, inode_t *inode_to_rmv, ino_t inode_to_rmv_n)
{
	int result = remove_from_parent(path);
	if (result <= 0)
		return result;

	substract_link(inode_to_rmv, inode_to_rmv_n);
	return EXIT_SUCCESS;
}

int
deserialize_inode(int fd, ino_t inode_n)
{
	inode_t *inode = get_inode_n(inode_n);

	size_t read_size = sizeof(inode_t);
	ssize_t result = read(fd, inode, read_size);
	if (result < read_size)
		return -EIO;

	size_t data_left_to_read = inode->size;

	size_t curr_offset = 0;
	while (data_left_to_read) {
		char buffer[BLOCK_SIZE];
		size_t this_read_size = data_left_to_read >= BLOCK_SIZE
		                                ? BLOCK_SIZE
		                                : data_left_to_read;
		ssize_t result = read(fd, buffer, this_read_size);
		if (result < this_read_size)
			return -EIO;

		fiuba_write(inode, buffer, this_read_size, curr_offset);

		data_left_to_read -= this_read_size;
		curr_offset += this_read_size;
	}


	return 0;
}

int
serialize_inode(int fd, ino_t inode_n)
{
	inode_t *inode = get_inode_n(inode_n);

	size_t write_size = sizeof(inode_t);
	ssize_t result = write(fd, inode, write_size);
	if (result < write_size)
		return -EIO;

	if (!INODE_BITMAP[inode_n])
		return EXIT_SUCCESS;

	size_t data_written = 0;
	int curr_block = 0;
	while (data_written < inode->size) {
		char *data = get_inode_block(inode, curr_block);
		size_t this_write_size = inode->size - data_written >= BLOCK_SIZE
		                                 ? BLOCK_SIZE
		                                 : inode->size - data_written;


		size_t result = write(fd, data, this_write_size);
		if (result < this_write_size)
			return -EIO;

		data_written += this_write_size;
		curr_block++;
	}
	return EXIT_SUCCESS;
}

ino_t
unlink_inode(const inode_t *inode, const char *dir_name)
{
	return iterate_over_dir(inode, remove_dentry, (void *) dir_name);
}

int
iterate_over_one_block_dir(const inode_t *inode,
                           dentry_iterator func,
                           void *param,
                           void *block,
                           int *curr_iter_size)
{
	dentry_t *curr_dentry = (dentry_t *) block;
	dentry_t *last_dentry = curr_dentry + (BLOCK_SIZE / sizeof(dentry_t)) - 1;

	int response;

	while ((inode->size > (*curr_iter_size)) && curr_dentry <= last_dentry) {
		response = func(curr_dentry, param);
		if (response != 0)
			return response;
		(*curr_iter_size) += sizeof(dentry_t);
		curr_dentry++;
	}

	return 0;
}

/// @brief Iterator over a Inode that is a directory.
/// For each entry in the directory calls the function with the dentry and the
/// param passed by parameter. Iterates until the function return true or until
/// the directory is fully consumed.
/// @param inode Directory inode
/// @param param Param passed to the function. Responsability of the caller to
/// know what the pointer represents.
/// @param f Function called for each dentry in the directory
/// @return -ENOTDIR should the inode not be a directory.
/// -ENOENT should the directory be fully consumed in the iteration
/// Number greater than 0 should, indicating the inode number of the dentry that
/// made the function return true.
int
iterate_over_dir(const inode_t *inode, dentry_iterator func, void *param)
{
	if (!(S_ISDIR(inode->type_mode)))
		return -ENOTDIR;

	int curr_inode_block = 0;  // block idx
	int curr_iter_size = 0;

	dentry_t *curr_dentry = get_inode_block(inode, curr_inode_block);
	while (curr_dentry && curr_iter_size < inode->size) {
		// cero -> sigo iterando
		// > cero -> freno iteracion forzado
		// < cero -> freno iteracion debido a error
		int response = iterate_over_one_block_dir(
		        inode, func, param, curr_dentry, &curr_iter_size);
		if (response != 0)
			return response;

		curr_inode_block++;
		curr_dentry = get_inode_block(inode, curr_inode_block);
	}
	return 0;
}

ino_t
search_dir_inode_n(const inode_t *inode, const char *dir_name)
{
	return iterate_over_dir(inode, is_dentry_searched, (void *) dir_name);
}

/*  =============================================================
 *  ========================= NOTIFY ============================
 *  =============================================================
 */

void
notify_access(inode_t *inode)
{
	time_t new_time = time(NULL);
	if (!inode)
		return;
	inode->last_access = new_time;
}

void
notify_modif(inode_t *inode)
{
	time_t new_time = time(NULL);
	if (!inode)
		return;
	inode->last_modification = new_time;
}


/*  =============================================================
 *  ========================= BLOCKS ============================
 *  =============================================================
 */

void *
get_block_n(int block_n)
{
	return BLOCK((SUPERBLOCK->data_start + block_n));
}

void *
allocate_direct_block(inode_t *inode, int new_free_block)
{
	inode->blocks[inode->block_amount] = new_free_block;
	inode->block_amount++;

	return get_block_n(new_free_block);
}

void *
allocate_indirect_block(inode_t *inode, int new_free_block)
{
	int new_indirect_block;

	int *indirect_block_ptr;
	int indirect_block_idx = inode->block_amount - INODE_INDIRECT_BLOCK_IDX;

	if (inode->block_amount == MAX_DIRECT_BLOCK_COUNT) {
		new_indirect_block = search_free_block();
		if (new_indirect_block < 0) {
			DATA_BITMAP[new_free_block] = false;
			return NULL;
		}
		inode->blocks[INODE_INDIRECT_BLOCK_IDX] = new_indirect_block;
	}

	indirect_block_ptr = get_block_n(inode->blocks[INODE_INDIRECT_BLOCK_IDX]);
	indirect_block_ptr[indirect_block_idx] = new_free_block;

	inode->block_amount++;

	return get_block_n(new_free_block);
}

void *
allocate_new_block(inode_t *inode)
{
	if (inode->block_amount >= MAX_BLOCK_COUNT)
		return NULL;

	int new_free_block = search_free_block();
	if (new_free_block < 0)
		return NULL;

	if (inode->block_amount < MAX_DIRECT_BLOCK_COUNT) {
		return allocate_direct_block(inode, new_free_block);
	}

	return allocate_indirect_block(inode, new_free_block);
}

void
deallocate_block(int block_n)
{
	DATA_BITMAP[block_n] = false;
}

/*  =============================================================
 *  ========================== DIR ==============================
 *  =============================================================
 */

int
fill_buff_readdir(dentry_t *dentry, void *param)
{
	void **special_param = param;
	void *buffer = special_param[0];
	fuse_fill_dir_t filler = special_param[1];

	if (!is_free_dentry(dentry))
		filler(buffer, dentry->file_name, NULL, 0);

	return 0;
}

int
fiuba_readdir(inode_t *inode, void *buffer, fuse_fill_dir_t filler)
{
	void *special_param[2];
	special_param[0] = (void *) buffer;
	special_param[1] = (void *) filler;

	iterate_over_dir(inode, fill_buff_readdir, special_param);

	return 0;
}

void
init_data_blocks_dir(inode_t *dir_inode, const ino_t dot, const ino_t dotdot)
{
	dentry_t dentries[] = { { .inode_number = dot, .file_name = "." },
		                { .inode_number = dotdot, .file_name = ".." } };
	fiuba_write(dir_inode, (char *) &dentries, sizeof(dentries), dir_inode->size);
}

bool
is_free_dentry(dentry_t *dentry)
{
	return (dentry->inode_number == EMPTY_DIRENTRY);
}

int
is_dentry_searched(dentry_t *dentry, void *_dir_name)
{
	char *dir_name = (char *) _dir_name;

	if (!is_free_dentry(dentry) && (strcmp(dentry->file_name, dir_name) == 0)) {
		return dentry->inode_number;
	}

	return 0;
}

int
search_first_file(dentry_t *dentry, void *_)
{
	if (is_free_dentry(dentry) || (strcmp(dentry->file_name, ".") == 0) ||
	    (strcmp(dentry->file_name, "..") == 0)) {
		return 0;
	}

	return dentry->inode_number;
}

bool
dir_is_empty(inode_t *inode)
{
	if (inode->size == 2 * sizeof(dentry_t))
		return true;

	int result = iterate_over_dir(inode, search_first_file, NULL);
	printf("RESULT %i\n", result);
	printf("Inode size %li\n", inode->size);
	if (result == 0)
		return true;
	return false;
}

int
remove_dentry(dentry_t *entry, void *_dir_name)
{
	char *dir_name = (char *) _dir_name;
	if (is_dentry_searched(entry, dir_name)) {
		entry->inode_number = EMPTY_DIRENTRY;
		return true;
	}
	return false;
}

int
insert_direntry_if_free(dentry_t *curr, void *_new_entry)
{
	dentry_t *new_entry = (dentry_t *) _new_entry;
	if (is_free_dentry(curr)) {
		curr->inode_number = new_entry->inode_number;
		strcpy(curr->file_name, new_entry->file_name);
		return new_entry->inode_number;
	}
	return 0;
}


/*  =============================================================
 *  ========================== ACCESS ===========================
 *  =============================================================
 */

int
fiuba_access(inode_t *inode, int mask)
{
	if ((R_OK & mask) &&
	    !((inode->type_mode & S_IRUSR) && (inode->type_mode & S_IRGRP))) {
		return -EXIT_FAILURE;
	}

	if ((W_OK & mask) &&
	    !((inode->type_mode & S_IWUSR) && (inode->type_mode & S_IWGRP))) {
		return -EXIT_FAILURE;
	}

	if ((X_OK & mask) &&
	    !((inode->type_mode & S_IXUSR) && (inode->type_mode & S_IXGRP))) {
		return -EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

/*  =============================================================
 *  ========================== SEARCH ===========================
 *  =============================================================
 */

int
search_free_block()
{
	bool *block_bitmap = DATA_BITMAP;
	superblock_t *superblock = SUPERBLOCK;

	for (int i = 0; i < superblock->data_blocks_amount; i++) {
		if (!block_bitmap[i]) {
			block_bitmap[i] = true;
			return i;
		}
	}
	return -ENOSPC;
}

int
get_first_file_name(const char *path, char *buffer)
{
	int i = 0;

	while (path[i] != '/' && path[i] != '\0') {
		buffer[i] = path[i];
		i++;
	}
	buffer[i] = '\0';

	return i;
}

int
search_inode_rec(const char *path, inode_t **out, inode_t *curr_inode, ino_t curr_inode_n)
{
	char buffer[MAX_FILE_NAME];
	int buff_len;

	int response;

	if (!(S_ISDIR(curr_inode->type_mode)))
		return -ENOTDIR;

	if ((*path) == '\0') {
		(*out) = curr_inode;
		return curr_inode_n;
	}

	buff_len = get_first_file_name(path, buffer);

	response = search_dir_inode_n(curr_inode, buffer);
	if (response < 0)
		return response;
	else if (response == 0)
		return -ENOENT;
	curr_inode_n = response;

	curr_inode = get_inode_n(curr_inode_n);
	if (path[buff_len] == '\0') {
		(*out) = curr_inode;
		return curr_inode_n;
	}

	return search_inode_rec(path + buff_len + 1, out, curr_inode, curr_inode_n);
}

int
search_inode(const char *path, inode_t **out)
{
	if ((*path) == '\0')
		return -EINVAL;
	if (strlen(path) >= MAX_FILE_NAME)
		return -ENAMETOOLONG;

	char path_copy[MAX_PATH_LEN] = { 0 };
	strcpy(path_copy, path);

	inode_t *root_inode = get_inode_n(ROOT_INODE);

	int path_len = strlen(path_copy);
	if (path_len > 1 && path_copy[path_len - 1] == '/')
		path_copy[path_len - 1] = '\0';

	return search_inode_rec(path_copy + 1, out, root_inode, ROOT_INODE);
}

int
search_parent_directory(const char *path, inode_t **out)
{
	int path_idx = strlen(path) - 1;
	char buffer[MAX_PATH_LEN];

	if (path_idx >= MAX_PATH_LEN)
		return -ENAMETOOLONG;

	if (path[path_idx] == '/' && strlen(path) == 1) {
		return search_inode(path, out);
	}

	if (path[path_idx] == '/') {
		// parece innecesario, pero no lo es si justo se trata de un directorio
		path_idx--;
	}
	while (path[path_idx] != '/') {
		path_idx--;
	}
	strcpy(buffer, path);
	buffer[path_idx + 1] = '\0';

	return search_inode(buffer, out);
}


/*  =============================================================
 *  ========================== READ =============================
 *  =============================================================
 */
int
fiuba_read(inode_t *inode, char *buffer, size_t size, off_t offset)
{
	if (inode->size < offset)
		return -EINVAL;

	int inode_block_idx = offset / BLOCK_SIZE;
	int curr_block_offset = offset % BLOCK_SIZE;

	int read = 0;

	char *data = get_inode_block(inode, inode_block_idx);
	while (data &&                         // I have something to read
	       offset + read < inode->size &&  // What I'm reading is user data
	       read < size  // I'm not exceeding the buffer size
	) {
		buffer[read] = data[curr_block_offset];

		curr_block_offset++;
		read++;

		if (curr_block_offset > BLOCK_SIZE) {
			inode_block_idx++;
			data = get_inode_block(inode, inode_block_idx);
			curr_block_offset = 0;
		}
	}
	notify_access(inode);
	return read;
}

/*  =============================================================
 *  ========================= WRITE =============================
 *  =============================================================
 */

int
calc_last_block_idx(int inode_block_idx, int block_offset, size_t size)
{
	int inode_last_block_idx = inode_block_idx + (size / BLOCK_SIZE);

	if ((size % BLOCK_SIZE) + block_offset >= BLOCK_SIZE)
		inode_last_block_idx += 1;

	return inode_last_block_idx;
}

int
fiuba_write(inode_t *inode, const char *buffer, size_t size, off_t offset)
{
	if (inode->size < offset)
		return -EINVAL;

	// Search where to start writing
	int inode_block_idx = offset / BLOCK_SIZE;
	int curr_block_offset = offset % BLOCK_SIZE;

	int inode_last_block_idx =
	        calc_last_block_idx(inode_block_idx, curr_block_offset, size);
	if (inode_last_block_idx >= MAX_BLOCK_COUNT)
		return -ENOMEM;

	int written = 0;

	byte *data_block = get_inode_block(inode, inode_block_idx);
	if (!data_block) {
		// If offset is exactly divisible by BLOCK_SIZE, the
		// block will be NULL because it points to a new block
		// that has to be allocated
		data_block = allocate_new_block(inode);
		if (!data_block) {
			// If allocation fails, it means that we have no more space
			return -ENOSPC;
		}
	}

	while (written < size && data_block) {
		data_block[curr_block_offset] = buffer[written];

		curr_block_offset++;
		written++;

		if (curr_block_offset >= BLOCK_SIZE) {
			curr_block_offset = 0;
			data_block = allocate_new_block(inode);
		}
	}

	if (!data_block)
		return -ENOSPC;

	inode->size = offset + size;
	notify_access(inode);
	notify_modif(inode);
	return written;
}

/*  =============================================================
 *  ==================== SERIALIZATION ==========================
 *  =============================================================
 */

/// @brief Deserializes a given file that represents the FS.
/// First is Inode Bitmap. Then each inode is directly adjacent to the
/// data that it contained. The inode struct is saved as is, but the
/// data block pointers are simply left for padding. It assumes a good
/// formatting of the file and that the file it's not corrupted, and do,
/// at least, contain all the information of the inodes.
/// @param fd The file descriptor to the file representing the FS.
/// @return -EIO in an error(early termination of the file or IO error
/// on the guest FS). errno is not touched. 0 if it deseriales correctly
int
deserialize(int fd)
{
	ssize_t result = 0;
	size_t size_to_read;

	superblock_t *superblock = SUPERBLOCK;
	bool *inode_bitmap = INODE_BITMAP;

	// Read superblock
	size_to_read = sizeof(superblock_t);
	result = read(fd, superblock, size_to_read);
	if (result < size_to_read)
		return -EIO;

	// Read Inode Bitmap to know the bitmap state
	size_to_read = superblock->inode_amount * sizeof(bool);
	result = read(fd, inode_bitmap, size_to_read);
	if (result < size_to_read)
		return -EIO;

	for (int i = 0; i < superblock->inode_amount; i++) {
		result = deserialize_inode(fd, i);
		if (result < 0) {
			printf("ERROR\n");
			return result;
		}
	}
	return 0;
}

/// @brief Serializes the current state of the FS.
///
/// The order of the final file is the given: INODE BMP,{ INODE i, [data
/// of INODE i] } i between 0 and superblock->inode_amount
/// @param fd File descriptor to where the representation will be saved
/// @return -EIO in an error(IO error on the guest FS). errno is not
/// touched. 0 if it serializes correctly
int
serialize(int fd)
{
	ssize_t result = 0;
	size_t size_to_write;

	superblock_t *superblock = SUPERBLOCK;
	bool *inode_bitmap = INODE_BITMAP;

	// write superblock
	size_to_write = sizeof(superblock_t);
	result = write(fd, superblock, size_to_write);
	if (result < size_to_write)
		return -EIO;

	// write inode bitmap
	size_to_write = superblock->inode_amount * sizeof(bool);
	result = write(fd, inode_bitmap, size_to_write);
	if (result < size_to_write)
		return -EIO;

	// move data block
	for (int i = 0; i < superblock->inode_amount; i++) {
		result = serialize_inode(fd, i);
		if (result < 0)
			return result;
	}

	return EXIT_SUCCESS;
}
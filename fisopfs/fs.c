#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "fs.h"

// Estructura
// Primer Bloque: Superbloque - Bitmap
// A partir del segundo bloque: Inodos

// En total hay 6400 bloques de 32 KiB cada uno, lo cual llega a un total de 200
// MiB. Si cada bloque tuviera un inodo asociado, seria un total de 6400 inodos
// maximo Si cada inodo aproximadamente ocupa 64 bytes, por bloque podrian haber
// 512 inodos, lo cual implica que los inodos ocuparan 13 bloques.
// Esto generaria un total de 6386 bloques para datos

// La conclusion de arriba puede que esté mal porque pesa mas cada inodo ahora.
// Tambien habria que ver los tipos de datos que puse que los saque de
// struct_stat.h asi directamente le podemos pasar la info de los inodos sin hacer calculos extras en el medio

#define SUPERBLOCK ((superblock_t *) blocks)
#define BLOCK(n) (void *) ((byte *) blocks + BLOCK_SIZE * n)
#define DATA_BITMAP ((bool *) (SUPERBLOCK + 1))
#define INODE_BITMAP                                                           \
	((bool *) (DATA_BITMAP + ((SUPERBLOCK)->data_blocks_amount)))
#define INODE_BLOCKS BLOCK(SUPERBLOCK->inode_start)
#define DATA_BLOCKS BLOCK(SUPERBLOCK->data_start)

#define DIR_TYPE_MODE                                                          \
	(__S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)


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


void init_inode(inode_t *inode, mode_t mode);

inode_t *get_inode_n(int inode_nmb);

void init_data_blocks_dir(inode_t *directory, const ino_t dot, const ino_t dotdot);

void *get_inode_block_n(const inode_t *inode, int block_number);

int next_token(const char *path, int offset, char *buffer, int limit, int *rest);

ino_t search_dir(const inode_t *directory, const char *inode_name);

int link_to_inode(ino_t parent_n, ino_t child_n, const char *child_name);

void deallocate_block(int block_n);

void *get_block_n(int block_n);

int search_parent(const char *path, ino_t *parent, int *name_offset_from_path);

ino_t unlink_inode(const inode_t *inode, const char *dir_name);

int chcount(char c, const char *s);


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
	inode->link_count = 0;
	inode->block_amount = 0;
}

int
add_inode_to_directory(inode_t *parent_inode, inode_t *new_inode, char *new_inode_name)
{
	dentry_t new_dentry = { .file_name = { 0 }, .inode_number = child_n };
	strcpy(new_dentry.file_name, new_inode_name);


	// iterar sobre el dir para ver si tenes un direntry vacio, si es asi,
	// usalo sino appendeas atras d todo


	int middle = iterate_over_dir(parent, (void *) &new_dentry, insert_dir);

	if (middle < 0) {
		fiuba_write(parent,
		            (char *) &new_dentry,
		            sizeof(dentry_t),
		            parent->size);
	}
	return 0;
}

int
new_inode(const char *path, mode_t mode, inode_t **out)
{
	if (strlen(path) > MAX_PATH_LEN)
		return -ENAMETOOLONG;

	inode_t *parent_inode;

	int result = search_parent_directory(path, &parent_inode);
	if (result < 0)
		return result;

	int result = search_free_inode(mode, &parent_inodes);
	if (result < 0)
		return -ENOMEM;


	// hasta aca
	int name_offset;
	ino_t parent;


	if (link_to_inode(parent, new_inode_n, path + name_offset) < 0) {
		bool *inode_bitmap = INODE_BITMAP;
		inode_bitmap[new_inode_n] = false;
		*out = NULL;
		return -EINVAL;
	}
	new_inode->link_count = 1;
	*out = new_inode;
	return 0;
}

inode_t *
get_inode_n(int inode_nmb)
{
	inode_t *inode = BLOCK(SUPERBLOCK->inode_start);
	return inode + inode_nmb;
}

void *
get_inode_indirect_block_n(const inode_t *inode, int block_n)
{
	int *indirect_block =
	        get_block_n(inode->blocks[INODE_INDIRECT_BLOCK_IDX]);
	int new_index = block_n - INODE_INDIRECT_BLOCK_IDX;
	return get_block_n(indirect_block[new_index]);
}

void *
get_inode_block_n(const inode_t *inode, int block_n)
{
	if (inode->block_amount <= block_n)
		return NULL;

	if (block_n >= INODE_INDIRECT_BLOCK_IDX) {
		return get_inode_indirect_block_n(inode, block_n);
	}

	return get_block_n(inode->blocks[block_n]);
}


void
notify_access(inode_t *inode, time_t *t)
{
	time_t new_time = time(NULL);
	if (!inode)
		return;
	if (t) {
		new_time = *t;
	}
	inode->last_access = new_time;
}

void
notify_modif(inode_t *inode, time_t *t)
{
	time_t new_time = time(NULL);
	if (!inode)
		return;
	if (t) {
		new_time = *t;
	}
	inode->last_modification = new_time;
}


int
search_free_inode(mode_t mode, inode_t **out)
{
	superblock_t *superblock = SUPERBLOCK;
	bool *inode_bitmap = INODE_BITMAP;
	for (int i = ROOT_INODE + 1; i < superblock->inode_amount; i++) {
		if (inode_bitmap[i]) {
			inode_bitmap[i] = true;
			*out = get_inode_n(i);
			init_inode(*out, mode);
			return i;
		}
	}
	return -ENOMEM;
}


void
deallocate_blocks_from_inode(inode_t *inode, int block_dest)
{
	int to_delete = inode->block_amount - block_dest;
	bool have_indirect = inode->block_amount >= MAX_INODE_BLOCK_PTR;
	if (have_indirect) {
		int *indirect = (int *) get_block_n(
		        inode->blocks[INODE_INDIRECT_BLOCK_IDX]);
		int indirect_amount = inode->block_amount - MAX_DIRECT_BLOCK_COUNT;
		for (int i = indirect_amount; i < 0 || to_delete < 0; i--) {
			deallocate_block(indirect[i]);
			to_delete--;
		}
	}
	if (block_dest <= MAX_DIRECT_BLOCK_COUNT) {
		deallocate_block(inode->blocks[INODE_INDIRECT_BLOCK_IDX]);
		for (int i = MAX_DIRECT_BLOCK_COUNT; i < 0 || to_delete < 0; i--) {
			deallocate_block(inode->blocks[i - 1]);
			to_delete--;
		}
	}
	inode->block_amount = block_dest;
}

void
deallocate_inode(ino_t inode_n, inode_t *inode)
{
	deallocate_blocks_from_inode(inode, 0);
	bool *inode_bitmap = INODE_BITMAP;
	inode_bitmap[inode_n] = false;
}

void
substract_link(ino_t inode_n)
{
	inode_t *inode = get_inode_n(inode_n);
	inode->link_count--;
	if (inode->link_count)
		deallocate_inode(inode_n, inode);
}

int
truncate_inode(inode_t *inode, off_t size)
{
	if (inode->size < size)
		return -EINVAL;

	int block_n = size / BLOCK_SIZE;
	block_n += size % BLOCK_SIZE != 0 ? 1 : 0;
	deallocate_blocks_from_inode(inode, block_n);
	inode->size = size;
	return 0;
}

int
remove_from_parent(const char *path)
{
	int name_offset;
	ino_t parent;
	ino_t result = search_parent(path, &parent, &name_offset);
	if (result < 0)
		return result;
	ino_t to_delete = unlink_inode(get_inode_n(parent), path + name_offset);
	return to_delete;
}

int
insert_into_parent(const char *path, ino_t inode_n)
{
	int name_offset;
	ino_t parent;
	int result = search_parent(path, &parent, &name_offset);
	if (result < 0)
		return result;
	return link_to_inode(parent, inode_n, path + name_offset);
}

int
fiuba_unlink(const char *path)
{
	ino_t to_delete = remove_from_parent(path);
	if (to_delete < 0)
		return to_delete;
	substract_link(to_delete);
	return 0;
}

int
move_inode(const char *from, const char *to)
{
	// Remove from old
	ino_t to_delete = remove_from_parent(from);
	if (to_delete < 0)
		return to_delete;

	// Link to new
	int result = insert_into_parent(to, to_delete);
	if (result < 0) {
		// Guaranteed not to fail, because it was recently removed from it.
		insert_into_parent(from, to_delete);
	}
	return result;
}


int
exchange_inodes(const char *path_one, const char *path_two)
{
	// All the insert operations here are guaranteed not to fail due to name collission.
	ino_t inode_one = remove_from_parent(path_one);
	if (inode_one < 0)
		return inode_one;
	ino_t inode_two = remove_from_parent(path_two);
	if (inode_two < 0) {
		insert_into_parent(path_one, inode_one);
		return inode_two;
	}
	insert_into_parent(path_one, inode_two);
	insert_into_parent(path_two, inode_one);
	return 0;
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

	size_t data_written = 0;
	int curr_block = 0;
	while (data_written < inode->size) {
		char *data = (char *) get_inode_block_n(inode, curr_block);
		size_t this_write_size = inode->size - data_written >= BLOCK_SIZE
		                                 ? BLOCK_SIZE
		                                 : inode->size - data_written;
		size_t result = write(fd, data, this_write_size);
		if (result < this_write_size)
			return -EIO;

		data_written += this_write_size;
		curr_block++;
	}
	return 0;
}

/*  =============================================================
 *  ========================= BLOCKS ============================
 *  =============================================================
 */

void *
get_block_n(int block_n)
{
	return BLOCK(SUPERBLOCK->data_start + block_n);
}

int
search_free_block()
{
	bool *block_bitmap = DATA_BITMAP;
	superblock_t *superblock = SUPERBLOCK;

	for (int i = last_used_block; i < superblock->block_size; i++) {
		if (!block_bitmap[i]) {
			block_bitmap[i] = true;
			last_used_block = i;
			return i;
		}
	}

	for (int i = 0; i < last_used_block; i++) {
		if (!block_bitmap[i]) {
			block_bitmap[i] = true;
			last_used_block = i;
			return i;
		}
	}
	return -ENOSPC;
}

void *
allocate_direct_block(inode_t *inode, int new_free_block)
{
	inode->blocks[inode->block_amount - 1] = new_free_block;
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

	filler(buffer, dentry->file_name, NULL, 0);
	return 0;
}

int
fiuba_readdir(inode_t *inode,
              void *buffer,
              fuse_fill_dir_t filler,
              off_t offset,
              struct fuse_file_info *fi)
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

	fiuba_write(dir_inode, (char *) &dentries, sizeof(dentries), 0);
}

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


bool
is_free_dentry(dentry_t *dentry)
{
	return (dentry->inode_number == EMPTY_DIRENTRY);
}

int
is_not_empty_dentry(dentry_t *dentry, void *_)
{
	return !is_free_dentry(dentry);
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
dir_is_empty(inode_t *inode)
{
	if (!(S_ISDIR(inode->type_mode)))
		return -ENOTDIR;

	if (inode->size == 2 * sizeof(dentry_t))
		return 1;

	int result = iterate_over_dir(inode, is_not_empty_dentry, NULL);
	if (result < 0)
		return 0;
	return 1;
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

	dentry_t *curr_dentry = get_inode_block_n(inode, curr_inode_block);
	while (curr_dentry && curr_iter_size < inode->size) {
		// cero -> sigo iterando
		// > cero -> freno iteracion forzado
		// < cero -> freno iteracion debido a error
		int response = iterate_over_one_block_dir(
		        inode, func, param, curr_dentry, &curr_iter_size);
		if (response != 0)
			return response;

		curr_inode_block++;
		curr_dentry = get_inode_block_n(inode, curr_inode_block);
	}
	return 0;
}

ino_t
search_dir(const inode_t *inode, const char *dir_name)
{
	return iterate_over_dir(inode, is_dentry_searched, (void *) dir_name);
}

bool
insert_dir(dentry_t *curr, void *_new_entry)
{
	dentry_t *new_entry = (dentry_t *) _new_entry;
	if (is_free_dentry(curr)) {
		curr->inode_number = new_entry->inode_number;
		strncpy(curr->file_name, new_entry->file_name, MAX_FILE_NAME);
		return true;
	}
	return false;
}

int
link_to_inode(ino_t parent_n, ino_t child_n, const char *child_name)
{
	if (strlen(child_name) > MAX_FILE_NAME)
		return -EINVAL;

	inode_t *parent = get_inode_n(parent_n);
	// Already exists, fail
	if (search_dir(parent, child_name) > 0)
		return -EINVAL;

	dentry_t new_dentry = { .file_name = { 0 }, .inode_number = child_n };
	strcpy(new_dentry.file_name, child_name, MAX_FILE_NAME);

	int middle = iterate_over_dir(parent, (void *) &new_dentry, insert_dir);

	if (middle < 0) {
		fiuba_write(parent,
		            (char *) &new_dentry,
		            sizeof(dentry_t),
		            parent->size);
	}
	return 0;
}

/*  =============================================================
 *  ========================== SEARCH ===========================
 *  =============================================================
 */

int
chcount(char c, const char *s)
{
	int count = 0;
	for (int i = 0; s[i]; i++)
		if (s[i] == c)
			count++;
	return count;
}


/// @brief Copies into the buffer at most limit characters. Zero terminates the
/// string resultant in the buffer. Is responsability of the caller to ensure
/// that the buffer has enough space for the new string. (Should be at least
/// limit + 1 size)
/// @param path String that will be copied until the next token
/// @param offset offset into the path. Zero index, assumes that always start
/// with '/'
/// @param buffer Buffer where the string will be copied
/// @param limit At most limit bytes copied
/// @param rest Pointer to an int, where, should exist, the amount of '/' rest
/// in the path should be saved.
/// @return -1 in case the token is larger than limit (In other words, if any
/// other char other than '/' or '\0' is in the new offset). An integer greater
/// than 0, showing the offset position into the path of the next separator
int
next_token(const char *path, int offset, char *buffer, int limit, int *rest)
{
	int path_index = offset + 1;  // Skip '/' that will be at the start
	int buffer_idx = 0;

	while (path[path_index] != '/' && path[path_index] != '\0' &&
	       buffer_idx < limit) {
		buffer[buffer_idx] = path[path_index];
		buffer_idx++;
		path_index++;
	}
	buffer[buffer_idx + 1] = '\0';

	bool token_finished = path[path_index] == '/' || path[path_index] == '\0';
	if (buffer_idx == limit && !token_finished)
		return -EINVAL;

	if (rest)
		*rest = chcount('/', path + path_index + 1);
	return path_index;
}


int
get_first_file_name(const char *path, char *buffer)
{
	int i = 0;

	while (path[i] != '/' && path[i] != '\0') {
		buffer[i] = path[i];
		i++;
	}
	buffer[i + 1] = '\0';

	return i;
}

int
search_inode_rec(const char *path, inode_t **out, inode_t *curr_inode)
{
	char buffer[MAX_FILE_NAME];
	int buff_len;

	int response;

	if (!(S_ISDIR(curr_inode->type_mode)))
		return -ENOTDIR;

	if ((*path) == '\0') {
		(*out) = curr_inode;
		return EXIT_SUCCESS;
	}

	buff_len = get_first_file_name(path, buffer);

	response = search_dir(curr_inode, buffer);
	if (response < 0)
		return response;
	else if (response == 0)
		return -ENOENT;

	curr_inode = get_inode_n(response);
	if (path[buff_len] == '\0') {
		(*out) = curr_inode;
		return EXIT_SUCCESS;
	}


	return search_inode_rec(path + buff_len + 1, out, curr_inode);
}

int
search_inode(const char *path, inode_t **out)
{
	if ((*path) == '\0')
		return -EINVAL;
	if (strlen(path) >= MAX_FILE_NAME)
		return -ENAMETOOLONG;

	inode_t *root_inode = get_inode_n(ROOT_INODE);

	int path_len = strlen(path);
	if (path_len > 1 && path[path_len - 1] == '/')
		path[path_len - 1] = '\0';

	return search_inode_rec(path + 1, out, root_inode);
}

int
search_parent_directory(const char *path, inode_t **out)
{
	int path_idx = strlen(path) - 1;
	int result;
	char buffer[MAX_PATH_LEN];

	if (path_idx >= MAX_PATH_LEN)
		return -ENAMETOOLONG;

	if (path[path_idx] == '/' && strlen(path) == 1) {
		return search_inode(path, out)
	}

	if (path[path_idx] == '/') {
		path[path_idx] = '\0';
		path_idx--;
	}

	while (path[path_idx] != '/') {
		path_idx--;
	}
	strcpy(buffer, path);
	buffer[path_idx] = '\0';

	return search_inode(buffer, out);
}


int
search_parent(const char *path, ino_t *parent, int *name_offset_from_path)
{
	inode_t *curr_inode = get_inode_n(ROOT_INODE);

	if (strcmp(path, "/") == 0) {
		return -EINVAL;
	}

	char buffer[MAX_FILE_NAME + 1] = { 0 };
	int curr_offset = 0;
	int rest = 1;
	ino_t parent_inode;
	while (rest) {
		curr_offset =
		        next_token(path, curr_offset, buffer, MAX_FILE_NAME, &rest);
		if (curr_offset < 0)
			return -EINVAL;
		parent_inode = search_dir(curr_inode, buffer);
		if (parent_inode < 0)
			return parent_inode;
		curr_inode = get_inode_n(parent_inode);
	}

	if (!(S_ISDIR(curr_inode->type_mode)))
		return -ENOTDIR;

	*parent = parent_inode;
	*name_offset_from_path = curr_offset + 1;
	return 0;
}


/*  =============================================================
 *  ========================== READ =============================
 *  =============================================================
 */
int
fiuba_read(const inode_t *inode, char *buffer, size_t size, off_t offset)
{
	int inode_block_idx = offset / BLOCK_SIZE;
	int curr_block_offset = offset % BLOCK_SIZE;

	int read = 0;

	char *data = get_inode_block_n(inode, inode_block_idx);
	if (!data)
		return -EINVAL;

	while (data &&                         // I have something to read
	       inode->size > offset + read &&  // What I'm reading is user data
	       read < size  // I'm not exceeding the buffer size
	) {
		buffer[read] = data[curr_block_offset];

		curr_block_offset++;

		if (curr_block_offset > BLOCK_SIZE) {
			inode_block_idx++;
			data = get_inode_block_n(inode, inode_block_idx);
			curr_block_offset = 0;
		}
	}

	return read;
}

/*  =============================================================
 *  ========================= WRITE =============================
 *  =============================================================
 */
int
fiuba_write(inode_t *inode, const char *buffer, size_t size, off_t offset)
{
	if (inode->size < offset)
		return -EINVAL;

	// Search where to start writing
	int inode_block_idx = offset / BLOCK_SIZE;
	int curr_block_offset = offset % BLOCK_SIZE;

	int written = 0;

	byte *data_block = get_inode_block_n(inode, inode_block_idx);
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
		inode->size++;

		if (curr_block_offset >= BLOCK_SIZE) {
			curr_block_offset = 0;
			data_block = allocate_new_block(inode);
		}
	}

	if (!data_block)
		return -ENOSPC;
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
	ssize_t operation_result = 0;
	size_t operation_read_size = 0;
	superblock_t *superblock = SUPERBLOCK;
	bool *inode_bitmap = INODE_BITMAP;

	// Read Inode Bitmap to know the bitmap state
	operation_read_size = superblock->inode_amount * sizeof(bool);
	operation_result = read(fd, inode_bitmap, operation_read_size);
	if (operation_result < operation_read_size)
		return -EIO;

	for (int i = 0; i < superblock->inode_amount; i++) {
		operation_result = deserialize_inode(fd, i);
		if (operation_result < 0)
			return operation_result;
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
	ssize_t operation_result = 0;
	size_t operation_write_size = 0;
	superblock_t *superblock = SUPERBLOCK;
	bool *inode_bitmap = INODE_BITMAP;

	operation_write_size = superblock->inode_amount * sizeof(bool);
	operation_result = write(fd, inode_bitmap, operation_write_size);
	if (operation_result < operation_write_size)
		return -EIO;

	for (int i = 0; i < superblock->inode_amount; i++) {
		operation_result = serialize_inode(fd, i);
		if (operation_result < 0)
			return operation_result;
	}

	return 0;
}
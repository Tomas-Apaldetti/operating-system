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

#define DIR_TYPE_MODE (__S_IFDIR | S_IRWXU | S_IRWXG | S_IROTH | S_IWOTH)

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

void init_dir(inode_t *directory, const ino_t dot, const ino_t dotdot);

inode_t *get_root_inode(void);

int next_token(const char* path, int offset, char* buffer, int limit, int* rest);

ino_t search_dir(const inode_t* directory, const char* inode_name);

void link_to_inode(ino_t parent_n, ino_t child_n, const char* child_name);

int search_parent(const char* path, ino_t *parent, int* name_offset_from_path);

int chcount(char c, const char *s);

inode_t * get_root_inode(void);


byte blocks[FS_SIZE] = { 0 };  // ~200 MiB

int last_used_block = -1;


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
	INODE_BITMAP[ROOT_INODE] = true;

	inode_t *root_inode = get_root_inode();

	init_inode(root_inode, DIR_TYPE_MODE);
	root_inode->link_count = 1;
	init_dir(root_inode, ROOT_INODE, ROOT_INODE);
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
}

inode_t *
get_root_inode()
{
	superblock_t *superblock = SUPERBLOCK;
	inode_t *inode = BLOCK(superblock->inode_start);
	return inode + superblock->root_inode;
}

inode_t *
get_inode_n(int inode_nmb)
{
	superblock_t *superblock = SUPERBLOCK;
	inode_t *inode = BLOCK(superblock->inode_start);
	return inode + inode_nmb;
}


ino_t
get_next_free_inode(mode_t mode, inode_t** out){
    superblock_t* superblock = SUPERBLOCK;
    bool *inode_bitmap = INODE_BITMAP;
    for (int i = 2; i < superblock->inode_amount; i++){
        if (inode_bitmap[i]){
            inode_bitmap[i] = true;
            inode_t* inode = get_inode_n(i);
            init_inode(inode, mode);
            *out = inode;
            return i;
        }
    }
    return -ENOSPC;
}

int
search_parent(const char* path, ino_t *parent, int* name_offset_from_path)
{
    inode_t *curr_inode = get_inode_n(ROOT_INODE);

    if (strcmp(path, "/") == 0){
		return -EINVAL;
	}

    char buffer[MAX_FILE_NAME + 1] = {0};
    int curr_offset = 0;
    int rest = 1;
    ino_t parent_inode;
    while (rest){
        curr_offset = next_token(path, curr_offset, buffer, MAX_FILE_NAME, &rest);
        parent_inode = search_dir(curr_inode, buffer);
        if (parent_inode < 0) return parent_inode;
        curr_inode = get_inode_n(parent_inode);
    }
    
    if (!(S_ISDIR(curr_inode->type_mode)))
        return -ENOTDIR;

    *parent = parent_inode;
    *name_offset_from_path = curr_offset + 1;
    return 0;
}

int
new_inode(const char* path, mode_t mode, inode_t **out)
{
    int name_offset;
    ino_t parent;
    int result = search_parent(path, &parent, &name_offset);
    if (result < 0) {
        *out = NULL;
        return result;
    }
    inode_t* new_inode;
    ino_t new_inode_n = get_next_free_inode(mode, &new_inode);
    if (new_inode_n < 0 ) {
        *out = NULL;
        return -ENOSPC;
    }
    link_to_inode(parent, new_inode_n, path + name_offset);
    new_inode->link_count = 1;
    *out = new_inode;
    return 0;
}

/*  ============================================================= 
 *  ========================= BLOCKS ============================
 *  =============================================================
 */

void *
get_block_n(int block_n)
{
	superblock_t *superblock = SUPERBLOCK;
	return BLOCK(superblock->data_start + block_n);
}

void *
get_block(const inode_t *inode, int block_number)
{
	if (inode->block_count <= block_number)
		return NULL;
	if (block_number >= INODE_ONE_LI_INDEX) {
		int *indirect = (int *) get_block_n(
		        inode->related_block[INODE_ONE_LI_INDEX]);
		int new_index = block_number - INODE_ONE_LI_INDEX;
		return get_block_n(indirect[new_index]);
	}
	return get_block_n(inode->related_block[block_number]);
}

int
search_free_block()
{
	bool *block_bitmap = DATA_BITMAP;
	superblock_t *superblock = SUPERBLOCK;
	for (int i = last_used_block; i < superblock->block_size; i++) {
		if (!block_bitmap[i]) {
			last_used_block = i;
			return i;
		}
	}

	for (int i = 0; i < last_used_block; i++) {
		if (!block_bitmap[i]) {
			last_used_block = i;
			return i;
		}
	}
	return -ENOSPC;
}

void
use_block(int block_n)
{
	bool *block_bitmap = DATA_BITMAP;
	block_bitmap[block_n] = true;
}

void *
allocate_next_block(inode_t *inode)
{
	if (inode->block_count >= MAX_DIRECT_BLOCK_COUNT + BLOCK_SIZE / sizeof(int))
		return NULL;
	int next_block = search_free_block();
	if (next_block < 0)
		return NULL;
	if (inode->block_count <= MAX_DIRECT_BLOCK_COUNT) {
		use_block(next_block);
		inode->related_block[inode->block_count - 1] = next_block;
		inode->block_count++;
		return get_block_n(next_block);
	}

	if (inode->block_count == INODE_ONE_LI_COUNT) {
		int indirect_block = search_free_block();
		if (indirect_block < 0)
			return NULL;
		use_block(indirect_block);
		inode->related_block[INODE_ONE_LI_INDEX] = indirect_block;
	}

	int *indirect =
	        (int *) get_block_n(inode->related_block[INODE_ONE_LI_INDEX]);
	int new_index = inode->block_count - INODE_ONE_LI_COUNT;
	indirect[new_index] = next_block;

	use_block(next_block);
	inode->block_count++;
	return get_block_n(next_block);
}

/*  =============================================================
 *  ========================== DIR ==============================
 *  =============================================================
 */

void
init_dir(inode_t *directory, const ino_t dot, const ino_t dotdot)
{
	dentry_t base[] = { { .inode_number = dot, .file_name = "." },
		            { .inode_number = dotdot, .file_name = ".." } };

	fiuba_write(directory, (char *) &base, sizeof(base), 0);
}

bool
is_empty_dentry(const dentry_t *dentry)
{
	return (dentry->inode_number == EMPTY_DIRENTRY);
}

bool
is_dentry_searched(const dentry_t *dentry, const char *dir_name)
{
	return (!is_empty_dentry(dentry) &&
	        (strcmp(dentry->file_name, dir_name) == 0));
}

ino_t
search_dir(const inode_t *inode, const char *dir_name)
{
	if (!(S_ISDIR(inode->type_mode)))
		return -ENOTDIR;

	int curr_inode_block = 0;  // block idx
	int searched_size = 0;     // esto es dentro del bloque
	int remaining_size = inode->size;

	dentry_t *entry = get_block(inode, curr_inode_block);

	while (entry && remaining_size > sizeof(dentry_t)) {
		if (is_dentry_searched(entry, dir_name))
			return entry->inode_number;

		entry++;

		remaining_size -= sizeof(dentry_t);
		searched_size += sizeof(dentry_t);

		if (searched_size >= BLOCK_SIZE) {
			curr_inode_block++;
			entry = get_block(inode, curr_inode_block);
			searched_size = 0;
		}
	}
	return -ENOENT;
}

void 
link_to_inode(ino_t parent_n, ino_t child_n, const char* child_name)
{
    dentry_t new_entry[1] = {
        {
            .file_name = {0},
            .inode_number = child_n
        }
    };
    strncpy(new_entry[0].file_name, child_name, MAX_FILE_NAME - 1);

    inode_t* parent = get_inode_n(parent_n);
    fiuba_write(parent,(char*)new_entry,MAX_FILE_NAME, parent->size);
}

/*  ============================================================= 
 *  ========================== SEARCH ===========================
 *  =============================================================
 */

int
chcount(char c, const char *s)
{
    int count = 0;
    for (int i = 0; s[i]; i++) if (s[i] == c) count++;
    return count;
}

int
next_token(const char* path, int offset, char* buffer, int limit, int* rest)
{
	int path_index = offset + 1;  // Skip '/' that will be at the start
	int i = 0;
	while (path && *path && path[path_index] != '/' && i <= limit) {
		buffer[i] = path[path_index];
		i++;
		path_index++;
	}
    if (rest) *rest = chcount('/', path + path_index + 1);
	return path_index;
}

int
search_inode(const char *path, inode_t **out)
{
	inode_t *curr_inode = get_inode_n(ROOT_INODE);

	if (strcmp(path, "/") == 0) {
		*out = curr_inode;
		return 0;
	}

	char buffer[MAX_FILE_NAME + 1] = { 0 };
	int curr_offset = 0;
	while (path[curr_offset]) {
		curr_offset =
		        next_token(path, curr_offset, buffer, MAX_FILE_NAME, NULL);
		int next_inode = search_dir(curr_inode, buffer);
		if (next_inode < 0)
			return next_inode;
		curr_inode = get_inode_n(next_inode);
	}
	*out = curr_inode;
	return 0;
}

/*  =============================================================
 *  ========================== READ =============================
 *  =============================================================
 */
long
fiuba_read(const inode_t *inode, char *buffer, size_t size, off_t offset)
{
	int block_n = offset / BLOCK_SIZE;
	int curr_block_offset = offset % BLOCK_SIZE;
	char *data = (char *) get_block(inode, block_n);
	if (!data)
		return -EINVAL;
	long read = 0;
	while (data &&                       // I have something to read
	       inode->size <= offset + read  // What I'm reading is user data
	       && read <= size  // I'm not exceeding the buffer size
	) {
		buffer[read] = data[curr_block_offset];
		curr_block_offset++;
		if (curr_block_offset > BLOCK_SIZE) {
			block_n++;
			data = (char *) get_block(inode, block_n);
			curr_block_offset = 0;
		}
	}

	return read;
}

/*  =============================================================
 *  ========================= WRITE =============================
 *  =============================================================
 */
long
fiuba_write(inode_t *inode, const char *buffer, size_t size, off_t offset)
{
	if (inode->size < offset)
		return -EINVAL;
	int block_n = offset / BLOCK_SIZE;
	int curr_block_offset = offset % BLOCK_SIZE;
	char *data = (char *) get_block(inode, block_n);
	if (!data) {
		// If offset is exactly divisible by BLOCK_SIZE, the block will
		// be NULL because it points to a new block that has to be
		// allocated
		data = (char *) allocate_next_block(inode);
		if (!data) {
			// If allocation fails, it means that we have no more space
			return -ENOSPC;
		}
	}
	long written = 0;
	while (data && written <= size) {
		data[curr_block_offset] = buffer[written];
		curr_block_offset++;
		written++;
		inode->size++;
		if (curr_block_offset > BLOCK_SIZE) {
			curr_block_offset = 0;
			data = (char *) allocate_next_block(inode);
		}
	}
	return written;
}
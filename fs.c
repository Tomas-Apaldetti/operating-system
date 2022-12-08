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

#define SUPERBLOCK (superblock_t *) blocks
#define BLOCK(n) (void *) ((byte *) blocks + BLOCK_SIZE * n)


#define BLOCK_AMOUNT 6300
// INODE_SIZE 256 
// INODE_AMOUNT 6300 
#define INODE_BLOCK_N 50
// Assuming 256b inode size, each blocks holds 128 inodes
// Therefore, to completely map each block with at least one inode
// we need ~49,2 blocks of inodes; rounding it to 50.
// Due that we have more inodes that we need, we can use the first one
// to mark that a dentry is empty (inode 0)

byte blocks[
    1 * BLOCK_SIZE +
    INODE_BLOCK_N * BLOCK_SIZE + 
    BLOCK_AMOUNT * BLOCK_SIZE +
    ] = { 0 };
// ~200 MiB

void
init_superblock()
{
	superblock_t *superblock = SUPERBLOCK;

	superblock->inode_amount = INODE_AMOUNT;
	superblock->data_blocks_amount = BLOCK_AMOUNT;
	superblock->block_size = BLOCK_SIZE;

	superblock->inode_start = 1;
	superblock->data_start = INODE_BLOCK_N + 1;

	superblock->root_inode = 1;
}

inode_t *
get_inode(int inode_nmb)
{
	superblock_t *superblock = SUPERBLOCK;
	inode_t *inode = BLOCK(superblock->inode_start);
	return inode + inode_nmb;
}


void
init_root_inode()
{
	superblock_t *superblock = SUPERBLOCK;
	inode_t *root_inode = get_inode(superblock->root_inode);

	// root_inode->type_mode = 0;
	// root_inode->user_id = 0;
	// root_inode->group_id = 0;
}
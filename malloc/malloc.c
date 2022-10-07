#define _DEFAULT_SOURCE

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>

#include "malloc.h"
#include "printfmt.h"

#define ALIGN4(s) (((((s) -1) >> 2) << 2) + 4)
#define REGION2PTR(r) ((r) + 1)
#define PTR2REGION(ptr) ((region_header_t *) ptr - 1)

#define KiB 1024
#define MiB (1024 * KiB)

#define MIN_REGION_LEN 64

#define BLOCK_SM (16 * KiB)
#define BLOCK_MD (1 * MiB)
#define BLOCK_LG (32 * MiB)

#define EXIT_OK 0
#define EXIT_ERROR -1

#define TOO_LARGE -1

#define NEWREGION(region, size) ()

typedef char *byte;
typedef struct region {
	bool free;
	size_t size;
	struct region *next;
	struct region *prev;
} region_header_t;

typedef struct block_header {
	int size;
	struct block_header *next;
	struct block_header *prev;
} block_header_t;

// 8  -> 7 -> 0111 -> 0001 -> 0100 = 4 -> 8
// 10 -> 9 -> 1001 -> 0010 -> 1000 = 8 -> 12
// 11 -> 10 -> 1010 -> 1000 = 8 	   -> 12

block_header_t *block_header_list = NULL;
block_header_t *block_header_tail = NULL;

int amount_of_mallocs = 0;
int amount_of_frees = 0;
int requested_memory = 0;

static void
print_statistics(void)
{
	printfmt("mallocs:   %d\n", amount_of_mallocs);
	printfmt("frees:     %d\n", amount_of_frees);
	printfmt("requested: %d\n", requested_memory);
}


static region_header_t *
find_region_first_fit(size_t size)
{
	block_header_t *actual_block = block_header_list;

	while (actual_block) {
		region_header_t *actual_region =
		        (region_header_t *) (actual_block + 1);
		while (actual_region) {
			if ((actual_region->size >= size + sizeof(region_header_t)) &&
			    (actual_region->free)) {
				return actual_region;
			}
			actual_region = actual_region->next;
		}
		actual_block = actual_block->next;
	}

	return NULL;
}

// finds the next free region
// that holds the requested size
//
static region_header_t *
find_free_region(size_t size)
{
#ifdef FIRST_FIT
	// Your code here for "first fit"
	return find_region_first_fit(size);
#endif

#ifdef BEST_FIT
	// Your code here for "best fit"
#endif

	return find_region_first_fit(size);
}

// NUEVO

// EXTRA

void
print_region(region_header_t *actual_region)
{
	printfmt("\n");
	int i = 0;
	while (actual_region) {
		printfmt("Region: %d\n", i);
		if (actual_region->free)
			printfmt("Direccion: %p ; Tamaño: %d - Free ; NextReg: "
			         "%p\n",
			         actual_region,
			         actual_region->size,
			         actual_region->next);
		else
			printfmt("Direccion: %p ; Tamaño: %d - Not Free ; "
			         "NextReg: %p\n",
			         actual_region,
			         actual_region->size,
			         actual_region->next);

		i++;
		actual_region = actual_region->next;
	}
	printfmt("\n");
}

void
print_blocks()
{
	block_header_t *actual_block = block_header_list;
	int i = 0;
	while (actual_block) {
		printfmt("\n");
		printfmt("Bloque: %d\n", i);
		printfmt("Direccion: %p - Tamaño: %d\n",
		         actual_block,
		         actual_block->size);
		print_region((region_header_t *) (actual_block + 1));
		i++;
		actual_block = actual_block->next;
	}

	printfmt("_____________________________________________________________"
	         "___________");
	printfmt("\n");
}


// ============================================

// MALLOC

void
append_block(block_header_t *new_block)
{
	new_block->prev = block_header_tail;
	new_block->next = NULL;
	if (!block_header_tail)
		block_header_list = new_block;
	else
		block_header_tail->next = new_block;

	block_header_tail = new_block;
}

int
get_block_size(size_t size)
{
	int block_size = BLOCK_SM;
	if (size > BLOCK_SM - sizeof(region_header_t)) {
		if (size <= BLOCK_MD - sizeof(region_header_t))
			block_size = BLOCK_MD;
		else if (size <= BLOCK_LG - sizeof(region_header_t))
			block_size = BLOCK_LG;
		else
			return TOO_LARGE;
	}

	return block_size;
}


/// @brief Construye un nuevo bloque dentro del Heap.
/// @param size PARA LA PARTE 2
/// @return Puntero al bloque creado o NULL en caso de error.
region_header_t *
new_block(size_t size)
{
	block_header_t *new_block_header = NULL;
	int block_size = get_block_size(size);
	if (block_size == TOO_LARGE)
		return NULL;

	new_block_header = mmap(NULL,
	                        block_size + sizeof(block_header_t),
	                        PROT_READ | PROT_WRITE,
	                        MAP_ANON | MAP_PRIVATE,
	                        -1,
	                        0);
	if (new_block_header == MAP_FAILED)
		return NULL;

	append_block(new_block_header);
	new_block_header->size = block_size;

	region_header_t *region_header =
	        (region_header_t *) (new_block_header + 1);
	region_header->size = block_size - sizeof(region_header_t);
	region_header->next = NULL;
	region_header->prev = NULL;
	region_header->free = true;

	return region_header;
}


/// @brief Realiza un split en una determinada region.
/// @param region Region a la cual se le aplicará el split.
/// @param size Tamaño que debe tener la región que luego se le retornará al usuario (Region 1).
void
try_split_region(region_header_t *region, size_t required_size)
{
	int required_size_to_split =
	        required_size + sizeof(region_header_t) + MIN_REGION_LEN;
	if (region->size < required_size_to_split)
		return;

	region_header_t *region_1 = region;
	region_header_t *region_2;

	region_header_t *no_header = region_1 + 1;  // TODO: Move to macro

	region_2 = (region_header_t *) (((byte) no_header) + required_size);

	region_2->next = region_1->next;
	region_2->prev = region_1;
	region_2->size = region_1->size - required_size - sizeof(region_header_t);
	region_2->free = true;

	region_1->next = region_2;
	region_1->size = required_size;
	region_1->free = true;
}

// ============================================

// FREE

region_header_t *
coalesce_regions(region_header_t *region_static,
                 region_header_t *region_adjacent_right)
{
	region_static->next = region_adjacent_right->next;
	if (region_adjacent_right->next)
		region_adjacent_right->next->prev = region_static;

	region_static->size +=
	        region_adjacent_right->size + sizeof(region_header_t);

	return region_static;
}

region_header_t *
try_coalesce_regions(region_header_t *region)
{
	region_header_t *surviving_region = region;
	if (region->next && region->next->free)
		surviving_region = coalesce_regions(region, region->next);
	if (region->prev && region->prev->free)
		surviving_region = coalesce_regions(region->prev, region);

	return surviving_region;
}

void
return_to_OS(region_header_t *region)
{
	block_header_t *block_header = ((block_header_t *) region) - 1;

	if (block_header->prev)
		block_header->prev->next = block_header->next;
	if (block_header->next)
		block_header->next->prev = block_header->prev;

	int result = munmap(block_header,
	                    block_header->size + sizeof(block_header_t));

	if (result == -1)
		printfmt("La cagamo");
}
// ============================================

void *
malloc(size_t size)
{
	region_header_t *block;
	region_header_t *region;

	// aligns to multiple of 4 bytes
	size = ALIGN4(size);

	if (size < MIN_REGION_LEN) {
		size = MIN_REGION_LEN;
	}

	// updates statistics
	amount_of_mallocs++;
	requested_memory += size;

	region = find_free_region(size);
	if (!region) {
		region = new_block(size);
		if (!region) {
			return NULL;
		}
	}

	try_split_region(region, size);
	region->free = false;

	// // lo hice para ir viendo cómo va todo
	print_blocks();

	return REGION2PTR(region);
}

void
free(void *ptr)
{
	// updates statistics
	amount_of_frees++;

	region_header_t *region_to_free = PTR2REGION(ptr);

	region_to_free->free = true;

	region_header_t *surviving_region = try_coalesce_regions(region_to_free);

	if (surviving_region->prev || surviving_region->next)
		return;

	return_to_OS(surviving_region);
}

void *
calloc(size_t nmemb, size_t size)
{
	// Your code here

	return NULL;
}

void *
realloc(void *ptr, size_t size)
{
	// Your code here

	return NULL;
}

#define _DEFAULT_SOURCE

#define _ALLOW_STATISTICS_

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>

#include "malloc.h"
#include "printfmt.h"

// TODO:
// 		* Test (Agregar Estadisticas para hacer mas facil el test)
// 		* Probar Realloc
//		* Dejar de confiar en el usuario => Agregar magic number
//		* Calloc
// 		* Completar Readme
//		* Usar MACROS
//		* SETEAR ERRNO = ENOMEM

#define MAGIC_32BIT 0xBE5A74CDU

#define ALIGN4(s) (((((s) -1) >> 2) << 2) + 4)
#define REGION2PTR(r) ((r) + 1)
#define PTR2REGION(ptr) ((region_header_t *) ptr - 1)

#define BLOCK2REGION(ptr) ((region_header_t *) (ptr + 1))

#define SPLITREGION(ptr, size) ((region_header_t *) (((byte) ptr + 1) + size))

#define KiB 1024
#define MiB (1024 * KiB)

#define MIN_REGION_LEN 64

#define BLOCK_SM (16 * KiB)
#define BLOCK_MD (1 * MiB)
#define BLOCK_LG (32 * MiB)

#define EXIT_OK 0
#define EXIT_ERROR -1

typedef char *byte;

typedef struct block_header {
	int size;
	// int min_free_region_size;
	struct block_header *next;
	struct block_header *prev;
} block_header_t;

typedef struct region_header {
	block_header_t *block_header;
	struct region_header *next;
	struct region_header *prev;
	size_t size;
	bool free;
	int check_num;
} region_header_t;


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

region_header_t *
loop_for_first_fit_region(region_header_t *region, size_t size)
{
	while (region) {
		if ((region->size >= size) && (region->free))
			return region;
		region = region->next;
	}

	return NULL;
}


static region_header_t *
find_region_first_fit(size_t size)
{
	region_header_t *found_region;

	for (block_header_t *curr_block_header = block_header_list;
	     curr_block_header;
	     curr_block_header = curr_block_header->next) {
		found_region = loop_for_first_fit_region(
		        BLOCK2REGION(curr_block_header), size);
		if (found_region)
			return found_region;
	}

	return NULL;
}

region_header_t *
loop_for_best_fit_region(region_header_t *best_region,
                         region_header_t *start_region,
                         size_t size)
{
	region_header_t *curr_best = best_region;
	region_header_t *curr_region_header = start_region;

	while (curr_region_header) {
		if (curr_region_header->size <= size) {
			if (best_region &&
			    curr_region_header->size < best_region->size) {
				curr_best = curr_region_header;
			} else if (!best_region) {
				curr_best = curr_region_header;
			}
		}
		curr_region_header = curr_region_header->next;
	}

	return curr_best;
}

region_header_t *
find_region_best_fit(size_t size)
{
	region_header_t *best_region;

	for (block_header_t *curr_block_header = block_header_list;
	     curr_block_header;
	     curr_block_header = curr_block_header->next) {
		best_region = loop_for_best_fit_region(
		        best_region, BLOCK2REGION(curr_block_header), size);
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
	return find_region_first_fit(size);
#endif

#ifdef BEST_FIT
	return find_region_best_fit(size);
#endif

	return find_region_first_fit(size);
}

// NUEVO

// EXTRA

void
print_region(region_header_t *curr_region_header)
{
	printfmt("\n");
	int i = 0;
	while (curr_region_header) {
		printfmt("Region: %d\n", i);
		if (curr_region_header->free)
			printfmt("Direccion: %p ; Tamaño: %d - Free ; NextReg: "
			         "%p\n",
			         curr_region_header,
			         curr_region_header->size,
			         curr_region_header->next);
		else
			printfmt("Direccion: %p ; Tamaño: %d - Not Free ; "
			         "NextReg: %p\n",
			         curr_region_header,
			         curr_region_header->size,
			         curr_region_header->next);

		i++;
		curr_region_header = curr_region_header->next;
	}
	printfmt("\n");
}

void
print_blocks()
{
	block_header_t *curr_block_header = block_header_list;
	int i = 0;
	while (curr_block_header) {
		printfmt("\n");
		printfmt("Bloque: %d\n", i);
		printfmt("Direccion: %p - Tamaño: %d\n",
		         curr_block_header,
		         curr_block_header->size);
		print_region((region_header_t *) (curr_block_header + 1));
		i++;
		curr_block_header = curr_block_header->next;
	}
	printfmt("_____________________________________________________________"
	         "___________\n");
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
	if (size + sizeof(region_header_t) <= BLOCK_SM)
		return BLOCK_SM;
	else if (size + sizeof(region_header_t) <= BLOCK_MD)
		return BLOCK_MD;
	else if (size + sizeof(region_header_t) <= BLOCK_LG)
		return BLOCK_LG;

	return EXIT_ERROR;
}

block_header_t *
new_block(size_t size)
{
	block_header_t *new_block_header;
	size_t new_block_size;

	new_block_size = get_block_size(size);
	if (new_block_size == EXIT_ERROR)
		return NULL;

	new_block_header = mmap(NULL,
	                        new_block_size + sizeof(block_header_t),
	                        PROT_READ | PROT_WRITE,
	                        MAP_ANON | MAP_PRIVATE,
	                        -1,
	                        0);
	if (new_block_header == MAP_FAILED)
		return NULL;

	new_block_header->size = new_block_size;
	append_block(new_block_header);

	return new_block_header;
}


/// @brief Construye un nuevo bloque dentro del Heap.
/// @param size PARA LA PARTE 2
/// @return Puntero al bloque creado o NULL en caso de error.
region_header_t *
new_region(size_t size)
{
	block_header_t *new_block_header;
	region_header_t *new_region_header;

	new_block_header = new_block(size);
	if (!new_block_header)
		return NULL;

	new_region_header = BLOCK2REGION(new_block_header);
	new_region_header->size =
	        new_block_header->size - sizeof(region_header_t);
	new_region_header->block_header = new_block_header;
	new_region_header->next = NULL;
	new_region_header->prev = NULL;
	new_region_header->free = true;
	new_region_header->check_num = MAGIC_32BIT;

	return new_region_header;
}

void
split_region(region_header_t *region, size_t size)
{
	block_header_t *block_header = region->block_header;
	region_header_t *new_next_region;

	new_next_region = SPLITREGION(region, size);
	new_next_region->next = region->next;
	new_next_region->prev = region;
	new_next_region->size = region->size - size - sizeof(region_header_t);
	new_next_region->block_header = block_header;
	new_next_region->free = true;
	new_next_region->check_num = MAGIC_32BIT;


	region->next = new_next_region;
	region->size = size;
}

/// @brief Realiza un split en una determinada region.
/// @param region Region a la cual se le aplicará el split.
/// @param size Tamaño que debe tener la región que luego se le retornará al usuario (Region 1).
void
try_split_region(region_header_t *region, size_t size)
{
	int required_size_to_split;

	required_size_to_split = size + sizeof(region_header_t) + MIN_REGION_LEN;
	if (region->size < required_size_to_split)
		return;

	split_region(region, size);
}

// ============================================

// FREE

region_header_t *
coalesce_region_with_its_next(region_header_t *region)
{
	block_header_t *block_header = region->block_header;
	region_header_t *next_region = region->next;

	region->size += region->next->size + sizeof(region_header_t);
	region->next = next_region->next;
	if (next_region->next)
		next_region->next->prev = region;

	return region;
}

region_header_t *
try_coalesce_regions(region_header_t *region)
{
	if (region->next && region->next->free)
		region = coalesce_region_with_its_next(region);
	if (region->prev && region->prev->free)
		region = coalesce_region_with_its_next(region->prev);

	return region;
}

void
return_block_to_OS(block_header_t *block_header)
{
	if (block_header->prev)
		block_header->prev->next = block_header->next;
	if (block_header->next)
		block_header->next->prev = block_header->prev;

	int result = munmap(block_header,
	                    block_header->size + sizeof(block_header_t));

	if (result == -1)
		printfmt("Error");  // cambiar
}
// ============================================

void *
malloc(size_t size)
{
	region_header_t *region;

	if ((size == 0) || (size > BLOCK_LG))
		return NULL;

	// aligns to multiple of 4 bytes
	size = ALIGN4(size);

	if (size < MIN_REGION_LEN)
		size = MIN_REGION_LEN;


	// updates statistics
	amount_of_mallocs++;
	requested_memory += size;

	region = find_free_region(size);
	if (!region) {
		printfmt("No hay region libre, busco un bloque ...\n");
		region = new_region(size);
		if (!region)
			return NULL;
	}

	region->free = false;
	try_split_region(region, size);

	// lo hice para ir viendo cómo va todo
	print_blocks();

	return REGION2PTR(region);
}

bool
are_all_block_free(region_header_t *region)
{
	return (!region->prev && !region->next && region->free);
}

int
check_region(region_header_t *region)
{
	if (region->free)
		return EXIT_ERROR;

	if (region->check_num != MAGIC_32BIT)
		return EXIT_ERROR;

	return EXIT_OK;
}

void
free(void *ptr)
{
	// updates statistics
	amount_of_frees++;

	region_header_t *region_to_free = PTR2REGION(ptr);
	if (check_region(region_to_free) == EXIT_ERROR)
		return;

	// ver qué pasa con punteros invalidos

	region_to_free->free = true;

	region_to_free = try_coalesce_regions(region_to_free);

	if (are_all_block_free(region_to_free))
		return_block_to_OS(region_to_free->block_header);
}

void *
calloc(size_t nmemb, size_t size)
{
	// Your code here

	return NULL;
}

bool
can_use_region(region_header_t *region, region_header_t *region_adjacent)
{
	if (!region_adjacent)
		return false;
	if (!region_adjacent->free)
		return false;
	return true;
}

int
sum_of_regions(region_header_t *region_one,
               region_header_t *region_two,
               region_header_t *region_three)
{
	int sum = region_one->size;
	if (region_two)
		sum += region_two->size + sizeof(region_header_t);
	if (region_three)
		sum += region_three->size + sizeof(region_header_t);
	return sum;
}

void
move_data(void *_dest, void *_src, int n)
{
	char *dest = (char *) _dest;
	char *src = (char *) _src;

	for (int i = 0; i < n; i++) {
		dest[i] = src[i];
	}
}

void *
realloc(void *ptr, size_t size)
{
	// 	if (size > BLOCK_LG)
	// 		return NULL;

	// 	size_t new_size = size;

	// 	new_size = ALIGN4(size);

	// 	if (new_size < MIN_REGION_LEN) {
	// 		new_size = MIN_REGION_LEN;
	// 	}

	// 	if (!ptr)
	// 		return malloc(new_size);

	// 	region_header_t *region = PTR2REGION(ptr);
	// 	if (region->size == new_size)
	// 		return region;

	// 	if (region->size > new_size) {
	// 		try_split_region(region, new_size);
	// 		if (region->next) {
	// 			try_coalesce_regions(region->next);
	// 		}
	// 		return region;
	// 	}

	// 	if (can_use_region(region, region->next) &&
	// 	    sum_of_regions(region, region->next, NULL) >= new_size) {
	// 		region = coalesce_regions(region, region->next);
	// 		try_split_region(region);
	// 		return region;
	// 	} else if (can_use_region(region, region->prev) &&
	// 	           sum_of_regions(region, region->prev, NULL) >=
	// new_size) { 		void *data = REGION2PTR(ptr); 		int
	// size_of_data = region->size; 		region =
	// coalesce_regions(region->prev, region);
	// try_split_region(region); 		move_data(REGION2PTR(region), data,
	// size_of_data); 		return region; 	} else if
	// (can_use_region(region, region->prev) && can_use_region(region,
	// region->next) && 	           sum_of_regions(region, region->prev,
	// region->next) >= new_size) { 		void *data = REGION2PTR(ptr);
	// int size_of_data = region->size; region =
	// try_coalesce_regions(region); try_split_region(region);
	// move_data(REGION2PTR(region), data, size_of_data); 		return region;
	// } else { void *data = REGION2PTR(ptr); 		int size_of_data
	// = region->size; void *new_region = malloc(new_size); if
	// (!new_region) 			return NULL;
	// move_data(new_region, data, size_of_data); 		free(ptr);
	// return new_region;
	// 	}

	// 	return NULL;
}


// TODO: SET ENOMEM
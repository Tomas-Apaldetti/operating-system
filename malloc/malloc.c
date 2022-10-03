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
#define PTR2REGION(ptr) ((region_t *) (ptr) -1)

#define KiB 1024

#define MIN_REGION_LEN 64

#define BLOCK_LEN_1 (16 * KiB)

#define EXIT_OK 0
#define EXIT_ERROR -1

#define NEWREGION(region, size) ()


// 8  -> 7 -> 0111 -> 0001 -> 0100 = 4 -> 8
// 10 -> 9 -> 1001 -> 0010 -> 1000 = 8 -> 12
// 11 -> 10 -> 1010 -> 1000 = 8 	   -> 12

typedef struct region {
	bool free;
	size_t size;
	struct region *next;
} region_t;

region_t *region_free_list = NULL;

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


static region_t *
find_region_first_fit(size_t size)
{
	region_t *actual_region = region_free_list;

	while (actual_region) {
		if ((actual_region->size >= size) && (actual_region->free)) {
			return actual_region;
		}
		actual_region = actual_region->next;
	}

	return NULL;
}

// finds the next free region
// that holds the requested size
//
static region_t *
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

/// ANTERIOR
static region_t *
grow_heap(size_t size)
{
	// finds the current heap break
	region_t *curr = (region_t *) sbrk(0);

	// allocates the requested size
	region_t *prev = (region_t *) sbrk(sizeof(region_t) + size);

	// verifies that the returned address
	// is the same that the previous break
	// (ref: sbrk(2))
	assert(curr == prev);

	// verifies that the allocation
	// is successful
	//
	// (ref: sbrk(2))
	if (curr == (region_t *) -1) {
		return NULL;
	}

	// first time here
	if (!region_free_list) {
		region_free_list = curr;
		atexit(print_statistics);
	}

	curr->size = size;
	curr->next = NULL;
	curr->free = false;

	return curr;
}


// NUEVO

// EXTRA

void
print_list()
{
	region_t *actual_region = region_free_list;
	printfmt("\n");
	printfmt("LISTA:\n");
	while (actual_region) {
		if (actual_region->free)
			printfmt("Direccion: %p - Tamaño: %d - Free\n",
			         actual_region,
			         actual_region->size);
		else
			printfmt("Direccion: %p - Tamaño: %d - Not Free\n",
			         actual_region,
			         actual_region->size);

		actual_region = actual_region->next;
	}
	printfmt("\n");
}


// ============================================

// MALLOC

/// @brief Construye un nuevo bloque dentro del Heap.
/// @param size PARA LA PARTE 2
/// @return Puntero al bloque creado o NULL en caso de error.
region_t *
new_block(size_t size)
{
	region_t *new_block;
	int block_size = BLOCK_LEN_1;

	new_block = mmap(NULL,
	                 block_size,
	                 PROT_READ | PROT_WRITE,
	                 MAP_ANON | MAP_PRIVATE,
	                 -1,
	                 0);
	if (new_block == MAP_FAILED)
		return NULL;

	new_block->size = block_size - sizeof(region_t);
	new_block->next = NULL;
	new_block->free = true;

	return new_block;
}

/// @brief Appendear el bloque al final de la lista recursivamente
/// @param actual_region Region actual
/// @param block Bloque a appendear
/// @return
region_t *
append_block(region_t *actual_region, region_t *block)
{
	if (!actual_region) {
		return block;
	}

	actual_region = append_block(actual_region->next, block);
	return actual_region;
}

/// @brief Agrega el bloque al final de la lista de regiones.
/// @param block_to_add Bloque a agregar en la lista.
void
add_block_to_region_free_list(region_t *block_to_add)
{
	region_free_list = append_block(region_free_list, block_to_add);
}

/// @brief Realiza un split en una determinada region.
/// @param region Region a la cual se le aplicará el split.
/// @param size Tamaño que debe tener la región que luego se le retornará al usuario (Region 1).
void
try_split_region(region_t *region, size_t size)
{
	region_t *region_1 = region;
	region_t *region_2;

	size_t region_1_size = size + sizeof(region_t);
	size_t region_2_size = region_1->size - size;

	// Calculo el minimo espacio que debe quedar para la region que quedará
	// libre. El espacio que debe quedar debe ser de al menos el tamaño de
	// una region_t + 1 byte. Si eso no se cumple, no le puedo hacer split.

	if (region_2_size <= sizeof(region_t)) {
		return;
	}

	region_2 = (region_t *) (((char *) region_1) + region_1_size);

	region_2->next = region_1->next;
	region_2->size = region_2_size - sizeof(region_t);
	region_2->free = true;

	region_1->next = region_2;
	region_1->size = region_1_size - sizeof(region_t);
	region_1->free = true;
}

// ============================================

// FREE

void
coalesce_regions(region_t *region_1, region_t *region_2)
{
	region_1->next = region_2->next;
	region_1->size = region_1->size + region_2->size + sizeof(region_t);
}

bool
should_be_coalsce(region_t *region_1, region_t *region_2)
{
	region_t *expected_ptr;

	if (!region_2)
		return false;

	if ((!region_1->free) || (!region_2->free))
		return false;

	expected_ptr = (region_t *) ((char *) (region_1 + 1) + region_1->size);
	return (expected_ptr == region_2);
}

void
try_coalesce_regions()
{
	region_t *previous_region = NULL;
	region_t *region = region_free_list;

	while (region) {
		if (should_be_coalsce(region, region->next)) {
			coalesce_regions(region, region->next);
		}
		region = region->next;
	}
}

// ============================================

void *
malloc(size_t size)
{
	region_t *block;
	region_t *region;

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
		block = new_block(size);
		if (!block)
			return NULL;

		add_block_to_region_free_list(block);

		region = block;
	}

	try_split_region(region, size);
	region->free = false;

	// lo hice para ir viendo cómo va todo
	print_list();

	return REGION2PTR(region);
}

void
free(void *ptr)
{
	// updates statistics
	amount_of_frees++;

	region_t *region_to_free = PTR2REGION(ptr);
	// assert(region_to_free->free == 0);

	region_to_free->free = true;

	try_coalesce_regions();
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

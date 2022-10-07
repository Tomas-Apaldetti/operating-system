#define _DEFAULT_SOURCE

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>

#include "malloc.h"
#include "printfmt.h"

#define MAGIC_32BIT 0xBE5A74CDU

#define ALIGN4(s) (((((s) -1) >> 2) << 2) + 4)
#define REGION2PTR(r) ((r) + 1)
#define PTR2REGION(ptr) ((region_header_t *) ptr - 1)

#define HEADER2REGION(ptr) ((region_header_t *) (ptr + 1);)

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
	block_header_t *header,
	struct region *next;
	struct region *prev;
} region_header_t;

typedef struct block_header {
	int size;
	int min_region;
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


region_header_t*
loop_for_region(region_header_t *actual_region, asize_t size)
{
	while (actual_region) {
		if ((actual_region->size >= size + sizeof(region_header_t)) &&
			(actual_region->free)) {
			return actual_region;
		}
		actual_region = actual_region->next;
	}
	
	return NULL;
}


static region_header_t *
find_region_first_fit(size_t size)
{
	block_header_t *actual_block = block_header_list;

	for(;actual_block;actual_block->next){
		if(actual_block -> min_region < size)
			continue;

		region_header_t * to_return = NULL;

		if((to_return = loop_for_region(HEADER2REGION(actual_block), size)))
			return to_return;
	}

	return NULL;
}

region_header_t*
loop_for_best_region(region_header_t* best_region, region_header_t* start_region,size_t size)
{
	region_header_t* curr_best = best_region;
	region_header_t* actual_region = start_region;

	while(actual_region){
		if(actual_region->size <= size){
			if(best_region && actual_region-> size < best_region->size){
				curr_best = actual_region;
			} else if (!best_region) {
				curr_best = actual_region;
			}
		}
		actual_region = actual_region -> next
	}

	return curr_best;
}

region_header_t*
find_best_fit(size_t size){
	block_header_t *actual_block = block_header_list;
	region_header_t *best_region = NULL;

	for(;actual_block;actual_block->next){
		if(actual_block -> min_region < size)
			continue;

		best_region = loop_for_best_region(best_region,HEADER2REGION(actual_block), size);
	}

	return best_region;
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
	return find_best_fit(size);
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
	new_block_header->size = new_block_header -> min_region =  block_size;

	region_header_t *region_header = HEADER2REGION(new_block_header);
	region_header->size = block_size - sizeof(region_header_t);
	region_header->header = new_block_header;
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
	region_2->header = region_1 -> header;
	region_2->free = true;

	if(region_2 -> size < region_2 -> header -> min_region )
		region_2 -> header -> min_region = region_2 -> size;

	region_1->next = region_2;
	region_1->size = required_size;
	region_1->free = false;
}

// ============================================

// FREE

int
get_min_region(region_header_t *region){
	int min = INT_MAX;
	region_header_t *curr = region;
	while(curr){
		if(curr->size < min){
			min = curr->size
		}
		curr = curr->next;
	}
	return min;
}

region_header_t *
coalesce_regions(region_header_t *region_static,
                 region_header_t *region_adjacent_right)
{
	int static_size = region_static -> size;
	int adjacent_size = region_adjacent_right -> size;
	int header_min_size = region_static -> header -> min_region;

	region_static->next = region_adjacent_right->next;
	if (region_adjacent_right->next)
		region_adjacent_right->next->prev = region_static;

	region_static->size +=
	        region_adjacent_right->size + sizeof(region_header_t);

	if(static_size == header_min_size || adjacent_size == header_min_size)
		region_static -> header -> min_region = get_min_region(HEADER2REGION(region_static -> header));
		
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
	if(size > BLOCK_LG)
		return NULL;

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

bool
can_use_region(region_header_t *region, region_header_t *region_adjacent){
	if(!region_adjacent)
		return false;
	if(!region_adjacent -> free)
		return false;
	return true;
}

int 
sum_of_regions(region_header_t *region_one, region_header_t * region_two, region_header_t * region_three){
	int sum = region_one->size;
	if(region_two)
		sum += region_two->size + sizeof(region_header_t);
	if(region_three)
		sum += region_three->size + sizeof(region_header_t);
	return sum;
}

void
move_data(void* _dest, void* _src, int n){
	char* dest = (char*) _dest;
	char* src = (char*) _src;

	for(int i = 0; i < n; i++){
		dest[i]= src[i];
	}
}

void *
realloc(void *ptr, size_t size)
{
	if(size > BLOCK_LG)
		return NULL;

	size_t new_size = size;

	new_size = ALIGN4(size);

	if (new_size < MIN_REGION_LEN) {
		new_size = MIN_REGION_LEN;
	}

	if(!ptr)
		return malloc(new_size);

	region_header_t *region = PTR2REGION(ptr);
	if(region -> size == new_size)
		return region;

	if(region -> size > new_size){
		try_split_region(region, new_size);
		if(regions -> next){
			_ = try_coalesce_regions(regions->next);
		}
		return region;
	}
	
	if(can_use_region(region, region->next) && sum_of_regions(region, region->next, NULL) >= new_size)
	{
		region = coalesce_regions(region, region -> next);
		try_split_region(region);
		return region;
	}
	else if(can_use_region(region, region->prev) && sum_of_regions(region, region->prev, NULL) >= new_size)
	{	
		void *data = REGION2PTR(ptr);
		int size_of_data = region->size;
		region = coalesce_regions(region->prev, region);
		try_split_region(region);
		move_data(REGION2PTR(region), data, size_of_data);
		return region;
	}
	else if(can_use_region(region, region->prev) && can_use_region(region, region->next) && sum_of_regions(region, region->prev, region->next) >= new_size)
	{
		void *data = REGION2PTR(ptr);
		int size_of_data = region->size;
		region = try_coalesce_regions(region);
		try_split_region(region);
		move_data(REGION2PTR(region), data, size_of_data);
		return region;
	} else {
		void *data = REGION2PTR(ptr);
		int size_of_data = region->size;
		void* new_region = malloc(new_size);
		if(!new_region)
			return NULL;
		move_data(new_region, data, size_of_data);
		free(ptr);
		return new_region;
	}

	return NULL;
}


//TODO: SET ENOMEM
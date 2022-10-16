#ifndef _MALLOC_H_
#define _MALLOC_H_

// Allocate SIZE bytes of memory.
void *malloc(size_t size);

// Free a block allocated by `malloc', `realloc' or `calloc'.s
void free(void *ptr);

// Allocate NMEMB elements of SIZE bytes each, all initialized to 0.
void *calloc(size_t nmemb, size_t size);

// Re-allocate the previously allocated block in PTR, making the new block SIZE
// bytes long. attribute_malloc is not used, because if realloc returns the same
// pointer that was passed to it, aliasing needs to be allowed between objects
// pointed by the old and new pointers.
void *realloc(void *ptr, size_t size);

#endif  // _MALLOC_H_

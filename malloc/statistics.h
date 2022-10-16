#ifndef _STATISTICS_H_
#include <stdio.h>


typedef struct stats {
	// Cantidad de llamadas
	size_t malloc_calls;
	size_t free_calls;
	size_t calloc_calls;
	size_t realloc_calls;
	// Contador de memoria
	size_t requested_amnt;  // Cantidad de memoria que el usuario pidio
	size_t mapped_amnt;     // Cantidad que malloc tiene pedida del SO
	size_t given_amnt;      // Cantidad TOTAL que se le dio al usuario
	size_t freed_amnt;      // Cantidad que el usuario libero (a malloc)
	// Contador de bloques
	size_t curr_blocks;
	size_t total_blocks;
	size_t returned_blocks;
	// Contador de regiones
	size_t curr_regions;
	size_t splitted_amnt;
	size_t coalesced_amnt;
	// Algoritmo realloc
	size_t realloc_optimized;
	size_t realloc_no_optimized;
} stats_t;

extern stats_t stats;

void reset_stats();

#endif
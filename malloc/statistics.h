#ifndef _STATISTICS_H_
typedef struct stats {
	// Cantidad de llamadas
	int malloc_calls;
	int free_calls;
	int calloc_calls;
	int realloc_calls;
	// Contador de memoria
	int requested_amnt;  // Cantidad de memoria que el usuario pidio
	int mapped_amnt;     // Cantidad que malloc tiene pedida del SO
	int given_amnt;      // Cantidad TOTAL que se le dio al usuario
	int freed_amnt;      // Cantidad que el usuario libero (a malloc)
	// Contador de bloques
	int curr_blocks;
	int total_blocks;
	int returned_blocks;
	// Contador de regiones
	int curr_regions;
	int splitted_amnt;
	int coalesced_amnt;
	// Algoritmo realloc
	int realloc_optimized;
	int realloc_no_optimized;
} stats_t;

extern stats_t stats;

void reset_stats();

#endif
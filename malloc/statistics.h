#ifdef USE_STATS
#ifndef _STATISTICS_H_
typedef struct stats{
    int malloc_calls;
    int free_calls;
    int requested_amnt;
    int mapped_amnt;
    int given_amnt;
    int freed_amnt;
    int curr_blocks;
    int total_blocks;
    int returned_blocks;
    int curr_regions;
    int splitted_amnt;
    int coalesced_amnt;
    int realloc_optimized;
    int realloc_no_optimized;
} stats_t;

extern stats_t stats;

void
reset_stats();

#endif
#endif
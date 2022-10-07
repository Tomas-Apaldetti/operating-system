#ifdef _ALLOW_STATISTICS_
#ifndef _STATISTICS_H_
typedef struct stats{
    int malloc_calls,
    int free_calls,
    int amount_asked,
    int block_asked,
    int block_freed,
    int region_coalesced,
}stats_t;

stats_t stats;

void
reset_stats();

#endif
#endif
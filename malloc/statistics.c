#ifdef USE_STATS
#include "statistics.h"

stats_t stats = { .malloc_calls = 0,
	          .free_calls = 0,
	          .requested_amnt = 0,
	          .mapped_amnt = 0,
	          .given_amnt = 0,
	          .freed_amnt = 0,
	          .curr_blocks = 0,
	          .total_blocks = 0,
	          .returned_blocks = 0,
	          .curr_regions = 0,
	          .splitted_amnt = 0,
	          .coalesced_amnt = 0,
	          .realloc_optimized = 0,
	          .realloc_no_optimized = 0 };

void
reset_stats()
{
	stats.malloc_calls = 0;
	stats.free_calls = 0;
	stats.requested_amnt = 0;
	stats.mapped_amnt = 0;
	stats.given_amnt = 0;
	stats.freed_amnt = 0;
	stats.curr_blocks = 0;
	stats.total_blocks = 0;
	stats.returned_blocks = 0;
	stats.curr_regions = 0;
	stats.splitted_amnt = 0;
	stats.coalesced_amnt = 0;
	stats.realloc_optimized = 0;
	stats.realloc_no_optimized = 0;
}

#endif
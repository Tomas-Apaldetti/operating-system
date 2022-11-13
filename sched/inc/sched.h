#ifndef JOS_INC_SCHED_H
#define JOS_INC_SCHED_H

#define TICR (0x0380 / 4)  // Timer Initial Count
#define TCCR (0x0390 / 4)  // Timer Current Count

#define NQUEUES 8
#define MLFQ_BOOST_AMNT 20
#define MLFQ_BASE_TIMER 10000000
#define MLFQ_MIN_THRESHOLD MLFQ_BASE_TIMER * 0.1
#define MLFQ_MAX_TIME_IN_QUEUE(env) MLFQ_BASE_TIMER * 0.5 * (env->queue_num + 1)
#define MLFQ_BOOST MLFQ_BASE_TIMER *MLFQ_BOOST_AMNT
#define CPU_TIME_HALT 500000

// Timer for a given priority
#define MLFQ_TIMER(prio) MLFQ_BASE_TIMER

// Set the timer for a given priority
#define TIMER_SET(remaining) lapicwr(TICR, remaining)

// Stops the LAPIC Timer
#define TIMER_STOP lapicwr(TICR, 0);

// Count the time for a given environment
#define MLFQ_TIME_COUNT(env)                                                   \
	{                                                                      \
		unsigned t = lapicr(TCCR);                                     \
		unsigned it = lapicr(TICR);                                    \
		mlfq_time_count += it - t;                                     \
		if (env) {                                                     \
			env->time_remaining = t;                               \
			env->time_in_queue = it - t;                           \
		}                                                              \
	}

// Reset the time for a given environment
#define MLFQ_TIME_RESET(env)                                                   \
	env->time_remaining = 0;                                               \
	env->time_in_queue = 0

// Check if the time is over the limit for a given environment
#define MLFQ_TQ_OVER(env) env->time_in_queue >= MLFQ_MAX_TIME_IN_QUEUE(env)

#define MLFQ_BOOST_RESET mlfq_time_count = 0

extern uint32_t mlfq_time_count;

#define NDWN_CALLS 4096 * 4
#define NRUN_CALLS 4096 * 4

typedef int32_t envid_t;

typedef struct env_info {
	envid_t env_id;
	int32_t last_prio;
	int32_t curr_prio;
} env_info_t;

typedef struct sched_stats {
	size_t sched_calls;
	size_t boost_calls;

	env_info_t downgrade_calls[NDWN_CALLS];
	size_t num_downgrade_calls;
	size_t laps_num_downgrade_calls;

	env_info_t env_run_calls[NRUN_CALLS];
	size_t num_env_run_calls;
	size_t laps_env_run_calls;

} sched_stats_t;

extern sched_stats_t stats;

#define ADD_DOWNGRADE_CALL_STATS(env, lprio, cprio)                            \
	{                                                                      \
		if (stats.num_downgrade_calls == NDWN_CALLS) {                 \
			stats.num_downgrade_calls = 0;                         \
			stats.laps_num_downgrade_calls++;                      \
		}                                                              \
		stats.downgrade_calls[stats.num_downgrade_calls].env_id =      \
		        env->env_id;                                           \
		stats.downgrade_calls[stats.num_downgrade_calls].last_prio =   \
		        lprio;                                                 \
		stats.downgrade_calls[stats.num_downgrade_calls].curr_prio =   \
		        cprio;                                                 \
		stats.num_downgrade_calls++;                                   \
	}
#define ADD_ENV_RUN_CALL_STATS(env)                                            \
	{                                                                      \
		if (stats.num_env_run_calls == NRUN_CALLS) {                   \
			stats.num_env_run_calls = 0;                           \
			stats.laps_env_run_calls++;                            \
		}                                                              \
		stats.env_run_calls[stats.num_env_run_calls].env_id =          \
		        env->env_id;                                           \
		stats.env_run_calls[stats.num_env_run_calls].curr_prio =       \
		        env->queue_num;                                        \
		stats.num_env_run_calls++;                                     \
	}
#define ADD_SCHED_CALL_STATS stats.sched_calls++
#define ADD_BOOST_CALL_STATS stats.boost_calls++


#endif /* !JOS_INC_SCHED_H */
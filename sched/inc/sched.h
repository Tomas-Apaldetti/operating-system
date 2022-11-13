#ifndef JOS_INC_SCHED_H
#define JOS_INC_SCHED_H

#define TICR (0x0380 / 4)  // Timer Initial Count
#define TCCR (0x0390 / 4)  // Timer Current Count

#define NQUEUES 8
#define MLFQ_BOOST_AMNT 10
#define MLFQ_BASE_TIMER 10000000
#define MLFQ_MIN_THRESHOLD MLFQ_BASE_TIMER * 0.1
#define MLFQ_BOOST MLFQ_BASE_TIMER *MLFQ_BOOST_AMNT

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
		if (env)                                                       \
			env->time_remaining = t;                               \
	}

// Reset the time for a given environment
#define MLFQ_TIME_RESET(env) env->time_remaining = 0

// Check if the time is over the limit for a given environment
#define MLFQ_TQ_OVER(env) env->time_remaining <= MLFQ_MIN_THRESHOLD

#define MLFQ_BOOST_RESET mlfq_time_count = 0

extern uint32_t mlfq_time_count;

#define NDWN_CALLS 4096 * 4
#define NRUN_CALLS 4096 * 4

typedef int32_t envid_t;

typedef struct sched_stats {
	size_t sched_calls;
	size_t boost_calls;
	envid_t downgrade_calls[NDWN_CALLS];
	int num_downgrade_calls;
	envid_t env_run_calls[NRUN_CALLS];
	int num_env_run_calls;
} sched_stats_t;

#define ADD_DOWNGRADE_CALL(env)                                                 \
	{                                                                       \
		stats.downgrade_calls[stats.num_downgrade_calls] = env->env_id; \
		stats.num_downgrade_calls++;                                    \
	}
#define ADD_ENV_RUN_CALL(env)                                                  \
	{                                                                      \
		stats.env_run_calls[stats.num_env_run_calls] = env->env_id;    \
		stats.num_env_run_calls++;                                     \
	}
#define ADD_SCHED_CALL stats.sched_calls++;
#define ADD_BOOST_CALL stats.boost_calls++;


#endif /* !JOS_INC_SCHED_H */
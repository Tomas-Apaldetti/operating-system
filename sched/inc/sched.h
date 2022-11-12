#ifndef JOS_INC_SCHED_H
#define JOS_INC_SCHED_H

#define TICR    (0x0380/4)   // Timer Initial Count
#define TCCR    (0x0390/4)   // Timer Current Count

#define NQUEUES 8
#define MLFQ_BOOST_AMNT 50
#define MLFQ_BASE_TIMER 500000
#define MLFQ_BOOST MLFQ_BASE_TIMER * MLFQ_BOOST_AMNT

// Timer for a given priority
#define MLFQ_TIMER(prio) MLFQ_BASE_TIMER 

// Set the timer for a given priority
#define TIMER_SET(remaining) lapicwr(TICR, remaining)

// Stops the LAPIC Timer
#define TIMER_STOP lapicwr(TICR, 0);

// Count the time for a given environment
#define MLFQ_TIME_COUNT(env) {                                      \
    unsigned t = lapicr(TCCR);                                      \
    unsigned it = lapicr(TICR);                                     \
    mlfq_time_count += it - t;                                      \
    if (env)                                                        \
        env->time_remaining = t;                                    \
}

// Reset the time for a given environment
#define MLFQ_TIME_RESET(env) env->time_remaining = 0

// Check if the time is over the limit for a given environment
#define MLFQ_TQ_OVER(env) env->time_remaining == 0

#define MLFQ_BOOST_RESET mlfq_time_count = 0

extern uint32_t mlfq_time_count;

#endif /* !JOS_INC_SCHED_H */
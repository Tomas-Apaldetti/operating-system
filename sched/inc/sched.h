#ifndef JOS_INC_SCHED_H
#define JOS_INC_SCHED_H

#define TICR    (0x0380/4)   // Timer Initial Count
#define TCCR    (0x0390/4)   // Timer Current Count

#define NQUEUES 8
#define MLFQ_BOOST 100
#define MLFQ_BASE_TIMER 500000

// Limit of time for a given priority
#define MLFQ_BASE_LIMIT(prio) MLFQ_BASE_TIMER * 4
// Timer for a given priority
#define MLFQ_TIMER(prio) MLFQ_BASE_TIMER 

// Count time for a given priority, if the process is in the last Queue, no counting is needed
#define TIMER_COUNT(prio) prio == NQUEUES - 1 ? 0 : MLFQ_TIMER(prio) - lapicr(TCCR)

// Set the timer for a given priority
#define TIMER_RESET(prio) lapicwr(TICR, MLFQ_TIMER(prio))

// Stops the LAPIC Timer
#define TIMER_STOP lapicwr(TICR, 0);

// Count the time for a given environment
#define MLFQ_TIME_COUNT(env) env->time_in_queue += TIMER_COUNT(env->queue_num)

// Reset the time for a given environment
#define MLFQ_TIME_RESET(env) env->time_in_queue = 0

// Check if the time is over the limit for a given environment
#define MLFQ_OVER_LIMIT(env) env->time_in_queue >= MLFQ_BASE_LIMIT(env->queue_num)

#endif /* !JOS_INC_SCHED_H */
/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_SCHED_H
#define JOS_KERN_SCHED_H
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <kern/cpu.h>

#define TICR    (0x0380/4)   // Timer Initial Count
#define TCCR    (0x0390/4)   // Timer Current Count
#define TDCR    (0x03E0/4)   // Timer Divide Configuration

#define NQUEUES 8
#define MLFQ_NPBOOST 15
#define MLFQ_BASE_TIMER 10000000


#define MLFQ_BASE_LIMIT(prio) MLFQ_BASE_TIMER
#define MLFQ_TIMER(prio) (MLFQ_BASE_TIMER)

#define TIMER_COUNT(prio) prio == NQUEUES ? 0 : MLFQ_TIMER(prio) - lapicr(TCCR);
#define TIMER_RESET(prio) lapicwr(TICR, MLFQ_TIMER(prio));

// This function does not return.
void sched_yield(void) __attribute__((noreturn));

// Initialize an Env for MLFQ
void env_MLFQ_init(struct Env*);

// Remove a free Env from Scheduler
void env_MLFQ_destroy(struct Env*);

void env_change_priority(struct Env *env, int32_t new_priority);

#endif  // !JOS_KERN_SCHED_H

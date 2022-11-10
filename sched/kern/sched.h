/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_SCHED_H
#define JOS_KERN_SCHED_H
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

// This function does not return.
void sched_yield(void) __attribute__((noreturn));

// Initialize an Env for MLFQ
void sched_create_env(struct Env *);

// Remove a free Env from Scheduler
void sched_destroy_env(struct Env *);

#endif  // !JOS_KERN_SCHED_H

#ifndef JOS_INC_SCHED_H
#define JOS_INC_SCHED_H

#define TICR    (0x0380/4)   // Timer Initial Count
#define TCCR    (0x0390/4)   // Timer Current Count

#define NQUEUES 8
#define MLFQ_NPBOOST 10
#define MLFQ_BASE_TIMER 100000

#define MLFQ_BASE_LIMIT(prio) MLFQ_BASE_TIMER
#define MLFQ_TIMER(prio) (MLFQ_BASE_TIMER)

#define TIMER_COUNT(prio) prio == NQUEUES - 1 ? 0 : MLFQ_TIMER(prio) - lapicr(TCCR);
#define TIMER_RESET(prio) lapicwr(TICR, MLFQ_TIMER(prio));
#define TIMER_STOP lapicwr(TICR, 0);

#define MLFQ_TIME_COUNT(env) env->time_in_queue += TIMER_COUNT(env->queue_num);
#define MLFQ_TIME_RESET(env) env->time_in_queue = 0;
#define MLFQ_OVER_LIMIT(env) env->time_in_queue >= MLFQ_BASE_LIMIT(env->queue_num)

#endif /* !JOS_INC_SCHED_H */
#include <inc/assert.h>
#include <inc/x86.h>
#include <inc/sched.h>

#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>

sched_stats_t stats = { .sched_calls = 0,
	                .boost_calls = 0,
	                .num_downgrade_calls = 0,
	                .num_env_run_calls = 0 };

uint32_t mlfq_time_count = 0;
typedef struct queue {
	struct Env *envs;
	struct Env *last_env;
	int32_t num_envs;
} queue_t;

queue_t queues[NQUEUES] = { { 0 } };

void sched_halt(void);

void sched_round_robin(struct Env *enviroments,
                       int32_t num_envs,
                       struct Env *last_run_env);

void sched_MLFQ(void);

void
sched_round_robin(struct Env *enviroments, int32_t num_envs, struct Env *last_run_env)
{
	struct Env *curr_env;

	if (last_run_env)
		curr_env = last_run_env->next_env;
	else
		curr_env = enviroments;

	while (curr_env) {
		if (curr_env->env_status == ENV_RUNNABLE) {
			curr_env->time_remaining =
			        MLFQ_TIMER(curr_env->queue_num);
			// ADD_ENV_RUN_CALL(curr_env)
			env_run(curr_env);
		}
		curr_env = curr_env->next_env;
	}

	// if there is no last_run_env, the entire list has been scrolled through
	if (!last_run_env)
		return;

	curr_env = enviroments;
	while (curr_env && (curr_env != last_run_env->next_env)) {
		if ((curr_env->env_status == ENV_RUNNABLE) ||
		    (curr_env->env_status == ENV_RUNNING &&
		     curr_env == last_run_env)) {
			curr_env->time_remaining =
			        MLFQ_TIMER(curr_env->queue_num);
			// ADD_ENV_RUN_CALL(curr_env)
			env_run(curr_env);
		}
		curr_env = curr_env->next_env;
	}
}

void
queue_remove(queue_t *queue, struct Env *env)
{
	if (!env || !queue)
		return;

	struct Env *prev = NULL;
	struct Env *curr = queue->envs;

	while (curr) {
		if (curr == env) {
			if (env == queue->envs) {
				queue->envs = env->next_env;
			}

			if (env == queue->last_env) {
				queue->last_env = prev;
			}

			if (prev) {
				prev->next_env = env->next_env;
			}

			queue->num_envs--;

			env->next_env = NULL;

			return;
		}

		prev = curr;
		curr = curr->next_env;
	}
}

void
queue_push(queue_t *queue, struct Env *env)
{
	if (!env || !queue)
		return;

	if (!queue->envs) {
		queue->envs = env;
	} else {
		queue->last_env->next_env = env;
	}

	queue->last_env = env;
	env->next_env = NULL;
	queue->num_envs++;
}

void
env_change_priority(struct Env *env, int32_t new_priority)
{
	if (new_priority >= NQUEUES)
		return;

	queue_t *pqueue = queues + env->queue_num;
	queue_t *nqueue = queues + new_priority;

	queue_remove(pqueue, env);

	env->queue_num = new_priority;
	MLFQ_TIME_RESET(env);

	queue_push(nqueue, env);
}

bool
should_env_downgrade(struct Env *env)
{
	return env && env->queue_num < NQUEUES - 1 && MLFQ_TQ_OVER(env);
}

void
downgrade_env(struct Env *env)
{
	env_change_priority(env, env->queue_num + 1);
	MLFQ_TIME_RESET(env);
	// ADD_DOWNGRADE_CALL(env)
}

bool
should_boost()
{
	return (mlfq_time_count >= MLFQ_BOOST);
}

void
boost()
{
	struct Env *curr_env;
	struct Env *next_env;

	//
	MLFQ_BOOST_RESET;

	for (int i = 1; i < NQUEUES; i++) {
		while (queues[i].envs) {
			env_change_priority(queues[i].envs, 0);
		}
	}
}

void
sched_MLFQ(void)
{
	// Check if curenv has to be downgraded
	if (should_env_downgrade(curenv)) {
		downgrade_env(curenv);
	}

	if (should_boost()) {
		boost();
	}

	// Find next env to run in RR fashion
	for (int32_t i = 0; i < NQUEUES; i++) {
		// if curenv is on the current queue, the round robin must start at curenv
		if (curenv && curenv->queue_num == i)
			sched_round_robin(queues[i].envs,
			                  queues[i].num_envs,
			                  curenv);
		else
			sched_round_robin(queues[i].envs, queues[i].num_envs, NULL);
	}
}

void
sched_alloc_env(struct Env *env)
{
	env->next_env = NULL;
	env->queue_num = 0;
	MLFQ_TIME_RESET(env);
	queue_push(queues, env);
}

void
sched_free_env(struct Env *env)
{
	queue_remove(queues + env->queue_num, env);
}

// Choose a user environment to run and run it.
void
sched_yield(void)
{
#ifdef MLFQ_SCHED
	sched_MLFQ();
#else
	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running.  Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.

	// Your code here
	sched_round_robin(queues[0].envs, NENV, curenv);
#endif
	sched_halt();
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING)) {
			break;
		}
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Once the scheduler has finishied it's work, print statistics on
	// performance. Your code here

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile("movl $0, %%ebp\n"
	             "movl %0, %%esp\n"
	             "pushl $0\n"
	             "pushl $0\n"
	             "sti\n"
	             "1:\n"
	             "hlt\n"
	             "jmp 1b\n"
	             :
	             : "a"(thiscpu->cpu_ts.ts_esp0));
}

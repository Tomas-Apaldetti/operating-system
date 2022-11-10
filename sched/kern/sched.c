#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>

// #define MLFQ_SCHED
#define NQUEUES 8
#define MLFQ_LIMIT 3
#define MLFQ_NPBOOST 15

typedef struct queue {
	struct Env *envs;
	struct Env *last_env;
	int32_t num_envs;
} queue_t;

int32_t schedno = 0;
queue_t queues[NQUEUES] = { { 0 } };

void sched_halt(void);

void sched_round_robin(struct Env *enviroments,
                       int32_t num_envs,
                       struct Env *last_run_env);

void sched_MLFQ(void);

void
log_queue(queue_t queue)
{
	cprintf("Puntero envs: %x\n", queue.envs);
	cprintf("Puntero last_env: %x\n", queue.last_env);
	cprintf("Cantidad de envs: %d\n", queue.num_envs);
}

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
			env_run(curr_env);
		}
		curr_env = curr_env->next_env;
	}
}

struct Env *
queue_remove_rec(queue_t *queue,
                 struct Env *prev_env,
                 struct Env *curr_env,
                 struct Env *env_to_rmv)
{
	if (!curr_env) {
		return NULL;
	}

	if (curr_env == env_to_rmv) {
		if (queue->last_env == env_to_rmv) {
			if (prev_env) {
				prev_env->next_env = curr_env->next_env;
			}

			queue->last_env = prev_env;
		}

		return curr_env->next_env;
	}

	return queue_remove_rec(queue, curr_env, curr_env->next_env, env_to_rmv);
}

void
print_queue(queue_t *queue)
{
	struct Env *curr_env = queue->envsenvs;
	while (curr_env) {
		cprintf("%p -> %p |", curr_env, curr_env->next_env);
	}
	cprintf("\n");
}

void
queue_remove(queue_t *queue, struct Env *env_to_rmv)
{
	queue->envs = queue_remove_rec(queue, NULL, queue->envs, env_to_rmv);
	queue->num_envs--;

	// struct Env *curr_env = queue->envs;
	// struct Env *prev_env = NULL;

	// while (curr_env) {
	// 	if (env_to_rmv == curr_env) {
	// 		// Remove from queue, treating it as a list
	// 		if (prev_env) {
	// 			prev_env->next_env = curr_env->next_env;
	// 		} else {
	// 			queue->envs = curr_env->next_env;
	// 		}

	// 		if (curr_env == queue->last_env) {
	// 			queue->last_env = curr_env->next_env;
	// 		}

	// 		queue->num_envs--;
	// 		return;
	// 	}

	// 	prev_env = curr_env;
	// 	curr_env = curr_env->next_env;
	// }
}

struct Env *
queue_pop(queue_t *queue)
{
	struct Env *ret = queue->envs;
	if (!ret)
		return NULL;
	queue->envs = ret->next_env;
	if (!queue->envs) {
		queue->last_env = NULL;
	}
	queue->num_envs -= 1;
	return ret;
}

void
queue_push(queue_t *queue, struct Env *env)
{
	// struct Env *last = queue->last_env;
	// if (!last) {
	// 	queue->envs = env;
	// 	last = env;
	// }

	// last->next_env = env;
	// env->next_env = NULL;

	// queue->last_env = env;

	//
	if (queue->num_envs == 0) {
		queue->envs = env;
	} else {
		queue->last_env->next_env = env;
	}
	queue->last_env = env;
	queue->num_envs += 1;
	env->next_env = NULL;
}

void
env_change_priority(struct Env *env, int32_t new_priority)
{
	queue_t *queue = &queues[env->queue_num];

	queue_remove(queue, env);

	env->queue_num = new_priority;

	queue_t *new_queue = &queues[env->queue_num];

	queue_push(new_queue, env);
}

void
env_try_downgrade(struct Env *env)
{
	if (env->env_runs > MLFQ_LIMIT && env->queue_num < NQUEUES - 1)
		env_change_priority(env, env->queue_num - 1);
}

void
sched_MLFQ(void)
{
	schedno++;

	// Check if curenv has to be downgraded
	if (curenv)
		env_try_downgrade(curenv);

	// Check if we need to boost priorities
	if (schedno == MLFQ_NPBOOST) {
		schedno = 0;  // Reset boost counter
		for (int i = 1; i < NQUEUES; i++) {
			struct Env *env;
			while ((env = queue_pop(&queues[i]))) {
				queue_push(&queues[0], env);
			}
		}
	}

	// Find next env to run in RR fashion
	for (int32_t i = 0; i < NQUEUES; i++) {
		// struct Env *env;
		// if (curenv && curenv->queue_num == i) {
		// 	env = curenv;
		// 	while (env) {
		// 		if (env->env_status == ENV_RUNNABLE) {
		// 			env_run(env);
		// 		}
		// 		env = env->next_env;
		// 	}
		// }
		// env = queues[i].envs;
		// while (env && env != curenv) {
		// 	if (env->env_status == ENV_RUNNABLE) {
		// 		env_run(env);
		// 	}
		// 	env = env->next_env;
		// }

		// if curenv in the current queue, the round robin must start at curenv
		if (curenv && curenv->queue_num == i)
			sched_round_robin(queues[i].envs,
			                  queues[i].num_envs,
			                  curenv);
		else
			sched_round_robin(queues[i].envs, queues[i].num_envs, NULL);
	}


	if (curenv)
		env_run(curenv);

	// If we get here, there are not runnable envs, just return for sched_halt
}

void
sched_create_env(struct Env *env)
{
	env->next_env = NULL;
	env->queue_num = 0;

	queue_push(queues, env);
}

void
sched_destroy_env(struct Env *env)
{
	queue_remove(&queues[env->queue_num], env);

	env->queue_num = 0;
	env->next_env = NULL;
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
		     envs[i].env_status == ENV_DYING))
			break;
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

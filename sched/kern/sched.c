#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>

// #define MLFQ_SHED
#define NQUEUES 8

typedef struct queue {
	struct Env *envs;
	struct Env *last_env;
	int32_t num_envs;
} queue_t;

int32_t num_scheds = 1;
queue_t queues[NQUEUES] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

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
		curr_env = last_run_env + 1;
	else
		curr_env = enviroments;

	while (curr_env < enviroments + num_envs) {
		if (curr_env->env_status == ENV_RUNNABLE) {
			env_run(curr_env);
		}
		curr_env = curr_env + 1;
	}

	// if there is no last_run_env, the entire list has been scrolled through
	if (!last_run_env)
		return;

	curr_env = enviroments;
	while (curr_env <= last_run_env) {
		if ((curr_env->env_status == ENV_RUNNABLE) ||
		    (curr_env->env_status == ENV_RUNNING &&
		     curr_env == last_run_env)) {
			env_run(curr_env);
		}
		curr_env = curr_env + 1;
	}
}


void
sched_MLFQ(void)
{
	// look if curenv has to update the queue
	// downgrade_curenv();
	if (curenv && curenv->env_runs > 10) {  // cambiar por el tiempo

		// si hay proceso actual y cumple una determinada condicion de
		// tiempo (es lo que falta invesitgar), se la baja de cola
	}

	if (num_scheds % 15 == 0) {
		struct Env *curr_env = queues[NQUEUES - 1].envs;
		// subir todos los procesos de la cola de abajo hacia arriba
		// cuando ocurren una determinada cantidad de sched
		while (curr_env) {
		}
	}

	// realizo round robin en el primer elemento que encuentre
	for (int32_t i = 0; i < NQUEUES; i++) {
		sched_round_robin(queues[i].envs, queues[i].num_envs, curenv);
	}
}

void
sched_init_MLFQ(void)
{
	// init all process into the first queue
	queues[0].envs = envs;
	queues[0].last_env = envs + (NENV - 1);

	for (int i = 0; i < NENV; i++) {
		envs[i].queue_num = 0;
		envs[i].next_env = (envs + i + 1);
	}
	envs[NENV - 1].next_env = NULL;
}

// Choose a user environment to run and run it.
void
sched_yield(void)
{
#if defined(MLFQ_SHED)
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
	sched_round_robin(envs, NENV, curenv);
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

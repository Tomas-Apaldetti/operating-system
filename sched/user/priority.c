#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	envid_t me = sys_getenvid();

	cprintf("I am the environment [%08x]\n", me);

	int prio = sys_get_prio(me);
	if (prio < 0) {
		cprintf("Couldn't get my priority.\n");
		cprintf("Shutting down\n");
		return;
	}

	cprintf("My priority at the start is %d\n", prio);

	int r = sys_set_prio(me, prio + 1);
	if (r != 0) {
		cprintf("Couldn't change my priority to something lower.\n");
		cprintf("Shutting down\n");
		return;
	}

	prio = sys_get_prio(me);
	cprintf("Then I changed my priority to something with lower priority "
	        "%d\n",
	        prio);

	r = sys_set_prio(me, prio);
	if (r != 0) {
		cprintf("Could change my priority to something with more "
		        "priority.\n");
		cprintf("Shutting down\n");
		return;
	}

	cprintf("But I couldn't change it back to my initial priority\n");

	envid_t pid = fork();
	if (pid < 0) {
		cprintf("Failed to fork\n");
		cprintf("Shutting down\n");
		return;
	} else if (pid != 0) {
		return;
	} else {
		me = sys_getenvid();
		prio = sys_get_prio(me);
		cprintf("I am the child [%08x] starting with same priority "
		        "%d\n",
		        me,
		        prio);
	}

	prio = 7;
	r = sys_set_prio(me, prio);
	if (r != 0) {
		cprintf("Couldn't change my priority to worst priority.\n");
		cprintf("Shutting down\n");
		return;
	}
	cprintf("Then I changed my priority to the worst priority %d\n", prio);

	prio++;
	r = sys_set_prio(me, prio);
	if (r == 0) {
		cprintf("Could change my priority to worst priority.\n");
		cprintf("Shutting down\n");
		return;
	}
	cprintf("I try to change my priority to something with lower "
	        "priority, but I couldn't change it\n");

	return;
}

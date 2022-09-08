#ifndef NARGS
#define NARGS 4
#endif

#define ARGS_SIZE NARGS + 2

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>

void callProg(char *args[ARGS_SIZE]);
void cleanArgs(char *args[ARGS_SIZE]);

int
main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "lab-xargs: too few arguments");
		exit(-1);
	}

	char *args[ARGS_SIZE] = { NULL };
	args[0] = argv[1];
	errno = 0;

	size_t linesize = 0;
	while (!feof(stdin)) {
		int i = 0;
		int getlineRes = 0;
		while (i < NARGS &&
		       (getlineRes = getline(&args[i + 1], &linesize, stdin)) !=
		               -1) {
			args[i + 1][strcspn(args[i + 1], "\n")] = 0;
			i++;
		}
		if (errno != EINVAL || errno != ENOMEM) {
			// getline errors
			cleanArgs(args);
			exit(-1);
		}

		if (getlineRes == -1) {
			// When EOF happens, getline reads garbage, and it does
			// not enter the while loop Therefore, outside of the
			// look needs to be checked
			free(args[i + 1]);
			args[i + 1] = NULL;
		}
		callProg(args);
	}

	return 0;
}

void
callProg(char *args[ARGS_SIZE])
{
	pid_t fRes = fork();

	int err = 0;


	if (fRes == 0) {
		int execRes = execvp(args[0], args);
		if (execRes == -1) {
			perror("lab-xargs: problema al realizar el exec");
			err = 1;
		}
	}
	if (err == 0 && wait(NULL) < 0) {
		perror("lab-xargs: problema al esperar al hijo");
		err = 1;
	}
	cleanArgs(args);
	if (err == 1) {
		exit(-1);
	}
}

void
cleanArgs(char *args[ARGS_SIZE])
{
	for (int i = 1; i <= NARGS; i++) {
		if (args[i]) {
			free(args[i]);
			args[i] = NULL;
		}
	}
}
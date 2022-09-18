#include "exec.h"

#define OVERWRITE 1

#define READ 0
#define WRITE 1

#define STDIN 0
#define STDOUT 1
#define STDERR 2


void do_redir(int to, int from);

void close_pipe(int pipe[2]);
// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	for (int i = 0; i < eargc; i++) {
		int sep;
		if ((sep = block_contains(eargv[i], '=')) == -1)
			continue;

		char key[ARGSIZE], value[ARGSIZE];
		get_environ_key(eargv[i], key);
		get_environ_value(eargv[i], value, sep);

		int env_result = setenv(key, value, OVERWRITE);
		if (env_result == -1) {
			perror_debug("Couldn't set environment "
			             "variable %s\n",
			             eargv);
		}
	}
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	if (!file) {
		return -1;
	}
	mode_t mode = S_IRUSR | S_IRGRP | S_IROTH;
	flags = flags | O_CLOEXEC;
	if (flags & O_RDWR) {
		flags = flags | O_CREAT | O_TRUNC;
		mode = mode | S_IWUSR;
	}
	return open(file, flags, mode);
}

void
do_redir(int to, int from)
{
	if (from == -1) {
		perror_debug("Can't open the file to redirect %d", to);
		exit(-1);
	}
	int dup_res = dup2(from, to);
	if (dup_res == -1) {
		perror_debug("Can't modify the std IO\n");
		close(from);
		exit(-1);
	}
}

void
close_pipe(int pipe[2])
{
	close(pipe[READ]);
	close(pipe[WRITE]);
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	switch (cmd->type) {
	case EXEC: {
		e = (struct execcmd *) cmd;
		set_environ_vars(e->eargv, e->eargc);
		int exec_result = execvp(e->argv[0], e->argv);
		if (exec_result == -1) {
			perror_debug("Can't execute\n");
			exit(-1);
		}
		break;
	}
	case BACK: {
		b = (struct backcmd *) cmd;
		exec_cmd(b->c);
		break;
	}

	case REDIR: {
		r = (struct execcmd *) cmd;

		if (r->in_file && r->in_file[0] != END_STRING) {
			int in_fd = open_redir_fd(r->in_file, O_RDONLY);
			do_redir(STDIN, in_fd);
		}

		if (r->out_file && r->out_file[0] != END_STRING) {
			int out_fd = open_redir_fd(r->out_file, O_RDWR);
			do_redir(STDOUT, out_fd);
		}

		if (r->err_file && r->err_file[0] != END_STRING) {
			if (strcmp(r->err_file, "&1") == 0 &&
			    r->out_file[0] != END_STRING) {
				do_redir(STDERR, STDOUT);
			} else {
				int err_fd = open_redir_fd(r->err_file, O_RDWR);
				do_redir(STDERR, err_fd);
			}
		}

		cmd->type = EXEC;
		exec_cmd(cmd);

		break;
	}

	case PIPE: {
		p = (struct pipecmd *) cmd;
		short pipe_mngr = 1;
		short err = 0;

		int left_to_rigth[2];
		int pipe_res = pipe(left_to_rigth);
		if (pipe_res == -1) {
			perror_debug("Can't open the pipes\n");
			exit(-1);
		}

		if (!err && pipe_mngr) {
			int left_fork_res = fork();

			if (left_fork_res == -1) {
				perror_debug("Can't create more procceses\n");
				err = -1;
			} else if (left_fork_res == 0) {
				pipe_mngr = 0;

				do_redir(STDOUT, left_to_rigth[WRITE]);

				close_pipe(left_to_rigth);

				exec_cmd(p->leftcmd);
			}
		}

		if (!err && pipe_mngr) {
			int rigth_fork_res = fork();

			if (rigth_fork_res == -1) {
				perror_debug("Can't create more procceses\n");

				err = -1;
			} else if (rigth_fork_res == 0) {
				pipe_mngr = 0;

				do_redir(STDIN, left_to_rigth[READ]);

				close_pipe(left_to_rigth);

				exec_cmd(p->rightcmd);
			}
		}

		if (pipe_mngr) {
			close_pipe(left_to_rigth);

			free_command(parsed_pipe);

			while (wait(NULL) > 0)
				;

			if (err) {
				exit(-1);
			}

			exit(0);
		}
		break;
	}
	}
}

#include "builtin.h"
#include <ctype.h>

int first_non_ws_char(char *str);

int pos_starts_with(char *cmd, char *search_term);

int
first_non_ws_char(char *str)
{
	int i = 0;
	while (str[i] && isspace(str[i]))
		i++;
	if (str[i] == END_STRING)
		return -1;
	return i;
}

int
pos_starts_with(char *cmd, char *search_term)
{
	int start = first_non_ws_char(cmd), end = 0;

	while (search_term[end] && cmd[start + end] == search_term[end]) {
		end++;
	}

	if (search_term[end])
		return -1;

	return start;
}

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	if (!cmd || pos_starts_with(cmd, "exit") == -1)
		return 0;

	return 1;
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	int cmdstart;
	if (!cmd || (cmdstart = pos_starts_with(cmd, "cd")) == -1)
		return 0;

	int dir_start = first_non_ws_char(cmd + cmdstart + 2);
	char *new_dir;
	if (dir_start == -1) {
		new_dir = getenv("HOME");
	} else {
		new_dir = cmd + dir_start + 2 + cmdstart;
	}

	int ch_res = chdir(new_dir);
	if (ch_res == -1) {
		perror_debug("Can't change directory\n");
		return 0;
	}

	strcpy(prompt, new_dir);
	return 1;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	if (!cmd || pos_starts_with(cmd, "pwd") == -1)
		return 0;

	printf_debug(prompt);
	return 1;
}

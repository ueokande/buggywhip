#ifndef UTIL_H
#define UTIL_H

#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

/*
 * Compares first word in line with command.  Returns 1 if *line starts with
 * *command, otherwise returns 0.
 */
static inline int command_eq(const char *line, const char *command) {
	size_t len = strlen(command);
	return strncmp(line, command, len) == 0;
}

/*
 * Skip whiltespaces on the head
 */
static inline char *trim_head(char *str) {
	while (isblank(*str)) {
		++str;
	}
	return str;
}

/*
 * Counts a number of the digit from an integer.
 */
int digit_number(int value, int base) {
	int count = 0;

	while(value != 0) {
		value = value / base;
		++count;
	}
	return count;
}

/*
 * Checks if a shell supports $LINENO.  Returns 0 if $LINE supported,
 * and returns 1 if not supported.  On failure returns -1.
 */
int check_lineno_support(const char *command) {
	pid_t pid;
	int status;

	pid = fork();
	if (pid == 0) {
		wait(&status);
		return status == 0;
	} else {
		int null = open("/dev/null", O_RDWR);
		dup2(null, STDOUT_FILENO);
		dup2(null, STDERR_FILENO);
		execl(command, "-u", "-c", ": $LINENO", NULL);
	}
	return 0;
}

#endif

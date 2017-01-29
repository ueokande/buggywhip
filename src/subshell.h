#ifndef SUBSHELL_H
#define SUBSHELL_H

#include <pty.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>

struct subshell_t {
	int master;			/* pseudoterminal master file descriptor */
	pid_t pid;			/* child pid */
	struct termios attrs;		/* slave terminal runtime attributes */
};

int get_master(int *master, int *slave, struct termios *attrs) {
	struct winsize win;

	if (tcgetattr(STDIN_FILENO, attrs) != 0) {
		return -1;
	}

	ioctl(STDIN_FILENO, TIOCGWINSZ, (char *)&win);
	if (openpty(master, slave, NULL, attrs, &win)) {
		return -1;
	}
	return 0;
}

void get_slave(int slave) {
	setsid();
	ioctl(slave, TIOCSCTTY, 0);
}

int open_subshell(struct subshell_t *subshell, char *path, char **argv) {
	int slave;

	if (get_master(&subshell->master, &slave, &subshell->attrs) < 0) {
		return -1;
	}

	subshell->pid = fork();

	// in parent process or fork failed, pid != 0
	if (subshell->pid == 0) {
		// child process
		get_slave(slave);

		close(subshell->master);

		dup2(slave, STDIN_FILENO);
		close(slave);

		execv(path, argv);

		exit(EXIT_FAILURE);
	}

	return 0;
}

int close_subshell(const struct subshell_t *subshell) {
	int status;
	if (waitpid(subshell->pid, &status, 0) < 0) {
		return -1;
	}
	tcsetattr(STDIN_FILENO, TCSADRAIN, &subshell->attrs);
	return status;
}

#endif

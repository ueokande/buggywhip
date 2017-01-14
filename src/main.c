#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <paths.h>
#include <poll.h>
#include <pty.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <sys/signalfd.h>
#include <sys/wait.h>

#include "fifo.h"
#include "strutil.h"

struct bgw_control {
	int master;			/* pseudoterminal master file descriptor */
	int slave;			/* pseudoterminal slave file descriptor */
	pid_t child;			/* child pid */
	struct termios attrs;		/* slave terminal runtime attributes */
	struct winsize win;		/* terminal window size */

	char *source_name;
	struct bgw_fifo fifo;
	int fifo_fd;

	int die;
} ctl = {};

void done();
void fail();
void command_do(const char *args);
void command_run();
void readline_handler(char *line);
void finalize_slave(int signum);
void do_readline();
static void get_slave();
static void get_master();

void done() {
	tcsetattr(STDIN_FILENO, TCSADRAIN, &ctl.attrs);
	kill(ctl.child, SIGTERM);

	if (remove_fifo(ctl.fifo)) {
		err(EXIT_FAILURE, "failed to remove fifo");
	}
	exit(EXIT_SUCCESS);
}

void fail() {
	kill(0, SIGTERM);
	done();
}

void command_do(const char *args) {
	if (dprintf(ctl.fifo_fd, "%s\n", args) < 0) {
		warn("failed to write to fd");
		fail();
	}
	fdatasync(ctl.fifo_fd);
}

pid_t open_subshell(char *path, char **argv) {
	get_master();

	pid_t pid = fork();

	// in parent process or fork failed, pid != 0
	if (pid == 0) {
		// child process
		get_slave(ctl);

		close(ctl.master);

		dup2(ctl.slave, STDIN_FILENO);
		close(ctl.slave);

		execv(path, argv);

		warn("failed to execute shell: %s", path);
		fail();
	}

	return pid;
}

int close_subshell() {
	return 0;
}

void command_run() {
	FILE *fp;

	size_t len = 0;
	ssize_t read_size;
	char *p;
	char *first_line;
	char *line;
	int i;

	int argc;
	char **argv;

	if ((fp = fopen(ctl.source_name, "r")) == NULL) {
		warn("failed to open %s", ctl.source_name);
		return;
	}

	read_size = getline(&first_line, &len, fp);
	if (read_size < 2 || first_line[0] != '#' || first_line[1] != '!') {
		fprintf(stderr, "%s: missing shebang", ctl.source_name);
		free(first_line);
		return;
	}

	// Trim first #! and last '\n'
	if (first_line[read_size - 1] == '\n') {
		first_line[read_size - 1] = '\0';
		--read_size;
	}
	p = trim_head(first_line + 2);
	read_size -= p - first_line;
	first_line = p;

	for (argc = 0, i = 0; i < read_size; ++i) {
		if (!isblank(first_line[i])) {
			continue;
		}
		i = trim_head(&first_line[i]) - first_line;
		++argc;
	}
	argv = malloc(sizeof(char*) * (argc + 3));

	for (argc = 0, i = 0; i < read_size; ++i) {
		if (!isblank(first_line[i])) {
			continue;
		}
		while (isblank(first_line[i])) {
			first_line[i] = '\0';
			++i;
		}
		argv[argc + 1] = &first_line[i];
		++argc;
	}
	argv[0] = ctl.source_name;
	argv[argc + 1] = ctl.fifo.name;
	argv[argc + 2] = NULL;

	if (create_fifo(&ctl.fifo) < 0) {
		warn("failed to create fifo");
	}

	ctl.child = open_subshell(first_line, argv);

	if (ctl.child < 0) {
		warn("fork failed");
		fail();
	}

	ctl.fifo_fd = open(ctl.fifo.name, O_WRONLY);
	if (ctl.fifo_fd < 0) {
		warn("failed to open fifo: %s", ctl.fifo.name);
		fail();
	}

	len = 0;
	while (true) {
		int read_size;

		read_size = getline(&line, &len, fp);
		if (read_size < 0) {
			break;
		}

		if (write(ctl.fifo_fd, line, read_size) < 0) {
			warn("failed to write to fd");
			fail();
		}
		fdatasync(ctl.fifo_fd);
	}
	free(line);

	close(ctl.fifo_fd);
	ctl.fifo_fd = -1;
}

void readline_handler(char *line) {
	add_history(line);

	if (line == NULL) {
		// EOF
		rl_callback_handler_remove ();
	} else if (line[0] == '\0') {
		// Do nothing
	} else if (command_eq(line, "quit")) {
		rl_callback_handler_remove ();
		ctl.die = 1;
	} else if (command_eq(line, "do")) {
		const char *args = strchr(line, ' ');
		if (args != NULL) {
			command_do(++args);
		}
	} else if (command_eq(line, "next")) {
	} else if (command_eq(line, "help")) {
	} else if (command_eq(line, "list")) {
	} else if (command_eq(line, "continue")) {
	} else if (command_eq(line, "edit")) {
	} else if (command_eq(line, "step")) {
	} else if (command_eq(line, "print")) {
	} else if (command_eq(line, "run")) {
		command_run();
	} else if (command_eq(line, "backtrace")) {
	} else {
		const char *rem = strchr(line, ' ');
		if (rem == NULL) {
			fprintf(stdout, "Undefined command: \"%s\"\n",	line);
		} else {
			fprintf(stdout, "Undefined command: \"%.*s\"\n", (int)(rem - line), line);
		}
	}

	free(line);
}

void finalize_slave(int signum) {
	int status;
	if (waitpid(ctl.child, &status, 0) < 0) {
		warn("failed to waitpid");
	}
	fprintf(stderr, "shell terminated with %d\n", status);
}

void do_readline() {

}

static void get_slave() {
	setsid();
	ioctl(ctl.slave, TIOCSCTTY, 0);
}

static void get_master() {
	if (tcgetattr(STDIN_FILENO, &ctl.attrs) != 0) {
		err(EXIT_FAILURE, "failed to get terminal attributes");
	}
	ioctl(STDIN_FILENO, TIOCGWINSZ, (char *)&ctl.win);
	if (openpty(&ctl.master, &ctl.slave, NULL, &ctl.attrs, &ctl.win)) {
		warn("openpty failed");
		fail();
	}
}

int main(int argc, char **argv) {
	if (argc < 2) {
		err(EXIT_FAILURE, "not enough arguments");
	}
	ctl.source_name = argv[1];

	struct pollfd pfd[] = {
		{ .fd = STDIN_FILENO, .events = POLLIN | POLLERR | POLLHUP }
	};

	rl_callback_handler_install ("> ", readline_handler);

	signal(SIGCHLD, finalize_slave);

	while (!ctl.die) {
		int ret;

		ret = poll(pfd, 1, -1);
		if (ret < 0) {
			if (errno == EINTR) {
				continue;
			}

			warn("poll failed");
			fail();
		}

		if (pfd[0].revents == 0) {
			continue;
		}

		rl_callback_read_char ();
	}

	done();

	return EXIT_FAILURE;
}

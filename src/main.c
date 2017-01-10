#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <paths.h>
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
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "strutil.h"

struct bgw_control {
	int master;			/* pseudoterminal master file descriptor */
	int slave;			/* pseudoterminal slave file descriptor */
	pid_t child;			/* child pid */
	struct termios attrs;		/* slave terminal runtime attributes */
	struct winsize win;		/* terminal window size */

	char *source_name;
	char fifo_name[PATH_MAX + 1];
} ctl = {};

static void done() {
	const char *tmpdir;

	tcsetattr(STDIN_FILENO, TCSADRAIN, &ctl.attrs);
	kill(ctl.child, SIGTERM);

	if (unlink(ctl.fifo_name)) {
		warn("unlink failed");
	}

	tmpdir = dirname(ctl.fifo_name);
	if (rmdir(tmpdir)) {
		warn("rmdir failed");
	}

	exit(EXIT_SUCCESS);
}

static void fail() {
	kill(0, SIGTERM);
	done(ctl);
}

void command_do(int fd, const char *args) {
	if (dprintf(fd, "%s\n", args) < 0) {
		warn("failed to write to fd");
		fail(ctl);
	}
	fdatasync(fd);
}

void do_readline() {
	char *line;
	int fifo_fd;
	int status;

	fifo_fd = open(ctl.fifo_name, O_WRONLY);
	if (fifo_fd < 0) {
		warn("failed to open fifo");
		fail(ctl);
	}

	while ((line = readline("> "))) {
		if (waitpid(ctl.child, &status, WNOHANG) > 0) {
			fprintf(stderr, "shell terminated with %d\n", status);
			break;
		}

		add_history(line);

		if (line[0] == '\0') {
			continue;
		} else if (command_eq(line, "quit")) {
			break;
		} else if (command_eq(line, "do")) {
			const char *args = strchr(line, ' ');
			if (args != NULL) {
				command_do(fifo_fd, ++args);
			}
		} else if (command_eq(line, "next")) {
		} else if (command_eq(line, "help")) {
		} else if (command_eq(line, "list")) {
		} else if (command_eq(line, "continue")) {
		} else if (command_eq(line, "edit")) {
		} else if (command_eq(line, "step")) {
		} else if (command_eq(line, "print")) {
		} else if (command_eq(line, "run")) {
		} else if (command_eq(line, "backtrace")) {
		} else {
			const char *rem = strchr(line, ' ');
			if (rem == NULL) {
				fprintf(stdout, "Undefined command: \"%s\"\n",	line);
			} else {
				fprintf(stdout, "Undefined command: \"%.*s\"\n", (int)(rem - line), line);
			}
		}
	}
	free(line);

	close(fifo_fd);

	done(ctl);
}

static void get_slave() {
	setsid();
	ioctl(ctl.slave, TIOCSCTTY, 0);
}

static void exec_shell() {
	FILE *fp;
	ssize_t read_size;
	size_t len = 0;
	char *first_line;
	char *p;
	int argc;
	int i;
	char **argv;

	if ((fp = fopen(ctl.source_name, "r")) == NULL) {
		warn("cannot open %s", ctl.source_name);
		fail(ctl);
	}
	read_size = getline(&first_line, &len, fp);
	fclose(fp);

	// Check if first line starts with "#!"
	if (read_size < 2 || first_line[0] != '#' || first_line[1] != '!') {
		fprintf(stderr, "%s: missing shebang", ctl.source_name);
		fail(ctl);
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
	argv[argc + 1] = ctl.fifo_name;
	argv[argc + 2] = NULL;

	execvp(first_line, argv);
}

static void do_shell() {

	get_slave(ctl);

	close(ctl.master);

	dup2(ctl.slave, STDIN_FILENO);
	close(ctl.slave);

	exec_shell(ctl);

	warn("failed to execute shell");
	fail(ctl);
}

static void get_master() {
	if (tcgetattr(STDIN_FILENO, &ctl.attrs) != 0) {
		err(EXIT_FAILURE, "failed to get terminal attributes");
	}
	ioctl(STDIN_FILENO, TIOCGWINSZ, (char *)&ctl.win);
	if (openpty(&ctl.master, &ctl.slave, NULL, &ctl.attrs, &ctl.win)) {
		warn("openpty failed");
		fail(ctl);
	}
}

static void get_fifo() {
	char tmpdir[PATH_MAX + 1] = "/tmp/bgw-XXXXXX";

	if (mkdtemp(tmpdir) == NULL) {
		warn("mkdtemp failed");
		fail(ctl);
	}

	sprintf(ctl.fifo_name, "%s/%s", tmpdir, "script");

	if (mkfifo(ctl.fifo_name, S_IWUSR | S_IRUSR) < 0) {
		warn("mkfifoat failed");
		fail(ctl);
	}
}

int main(int argc, char **argv) {

	if (argc < 2) {
		err(EXIT_FAILURE, "not enough arguments");
	}
	ctl.source_name = argv[1];

	get_fifo(&ctl);

	get_master(&ctl);

	fflush(stdout);
	ctl.child = fork();

	switch (ctl.child) {
	case -1:
		warn("fork failed");
		fail();
		break;
	case 0:	 // child process
		do_shell();
		break;
	default:	// parent process
		do_readline();
		break;
	}

	return EXIT_FAILURE;
}

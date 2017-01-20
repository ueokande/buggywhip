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

#include "subshell.h"
#include "fileutil.h"
#include "fifo.h"
#include "util.h"
#include "bitset.h"

struct bgw_control {
	struct subshell_t subshell;

	char *source_name;
	struct bgw_fifo fifo;
	int fifo_fd;

	int die;

	int last_list_line;
	struct bitset *breakpoints;
} ctl = {};

void done();
void fail();
void command_do(const char *args);
void command_list(const char *args);
void command_run();
void readline_handler(char *line);
void finalize_slave(int signum);

void done() {
	kill(ctl.subshell.pid, SIGTERM);

	bitset_delete(ctl.breakpoints);

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

void command_list(const char *args) {
	int lineno_from;
	FILE *fp;
	char *line;
	int lineno_width;
	size_t len = 0;
	int i;

	if (strlen(args) == 0) {
		lineno_from = ctl.last_list_line;
	} else {
		int num;
		char *end;

		num = strtol(args, &end, 10);
		if (*end == '\0') {
			lineno_from = num;
		} else {
			lineno_from = grep_word(args, ctl.source_name) + 1;
			if (lineno_from <= 0) {
				fprintf(stderr, "keyword \"%s\" not found\n", args);
				return;
			}
		}
	}

	if ((fp = fopen(ctl.source_name, "r")) == NULL) {
		warn("failed to open: %s", ctl.source_name);
	}

	lineno_width = digit_number(lineno_from + 10, 10);

	for (i = 1; i <= lineno_from + 10; ++i) {
		ssize_t read_size;
		read_size = getline(&line, &len, fp);
		if (read_size < 0) {
			i = 0;
			break;
		}

		if (i >= lineno_from) {
			fprintf(stdout, "%*d: %.*s", lineno_width, i, (int)read_size, line);
		}
	}

	ctl.last_list_line = i;

	free(line);
	fclose(fp);

}

void command_break(const char *args) {
	int lineno;

	if (strlen(args) == 0) {
		lineno = ctl.last_list_line;
	} else {
		int num;
		char *end;

		num = strtol(args, &end, 10);
		if (*end == '\0') {
			lineno = num;
		} else {
			lineno = grep_word(args, ctl.source_name);
			if (lineno < 0) {
				warn("failed to find a word from \"%s\"", ctl.source_name);
				return;
			}
		}
	}
	bitset_set(ctl.breakpoints, lineno);
	printf("Breakpoint at line %d\n", lineno);
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

	if (open_subshell(&ctl.subshell, first_line, argv) < 0) {
		warn("failed to open sub-shell");
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
	} else if (command_eq(line, "break")) {
		char *args = strchr(line, ' ');
		if (args == NULL) {
			args = "";
		} else {
			args = trim_head(args);
		}
		command_break(args);
	} else if (command_eq(line, "next")) {
	} else if (command_eq(line, "help")) {
	} else if (command_eq(line, "list")) {
		char *args = strchr(line, ' ');
		if (args == NULL) {
			args = "";
		} else {
			args = trim_head(args);
		}
		command_list(args);
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
	int status = close_subshell(&ctl.subshell);
	if (status < 0) {
		warn("failed to waitpid");
	}
	fprintf(stderr, "shell terminated with %d\n", status);
}

int main(int argc, char **argv) {
	ssize_t lines;

	if (argc < 2) {
		err(EXIT_FAILURE, "not enough arguments");
	}
	ctl.source_name = argv[1];
	lines = count_lines(ctl.source_name);
	if (lines < 0) {
		err(EXIT_FAILURE, "failed to open source file");
	}
	ctl.breakpoints = bitset_new(lines);

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

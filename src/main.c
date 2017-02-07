#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <paths.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <sys/signalfd.h>

#include "subshell.h"
#include "fileutil.h"
#include "fifo.h"
#include "util.h"
#include "bitset.h"

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))

struct bgw_control {
	struct subshell_t subshell;

	char *source_name;
	int source_fd;
	struct bgw_fifo fifo;
	int fifo_fd;

	int die;

	int last_list_line;
	struct bitset *breakpoints;
	int program_counter;
} ctl = {
	.source_fd = -1,
	.fifo_fd = -1,
};

void done();
void fail();
void command_do(const char *args);
void command_list(const char *args);
void command_continue();
void command_run();
void readline_handler(char *line);
void finalize_slave();
int signal_handler(int sigfd);
int get_signalfd();

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
			lineno = num - 1;
		} else {
			lineno = grep_word(args, ctl.source_name);
			if (lineno < 0) {
				warn("failed to find a word from \"%s\"", ctl.source_name);
				return;
			}
		}
	}
	bitset_set(ctl.breakpoints, lineno);
	printf("Breakpoint at line %d\n", lineno + 1);
}

void command_continue() {
	char *line;
	size_t len = 0;
	FILE *fp;

	if (ctl.source_fd < 0) {
		fprintf(stderr, "not running a script\n");
		return;
	}

	fp = fdopen(ctl.source_fd, "r");

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

		++ctl.program_counter;
		if (bitset_test(ctl.breakpoints, ctl.program_counter)) {
			fprintf(stderr, "Stopped at %d\n", ctl.program_counter);

			free(line);
			lseek(ctl.source_fd, ftell(fp), SEEK_SET);
			return;
		}
	}

	free(line);

	close(ctl.source_fd);
	ctl.source_fd = -1;

	close(ctl.fifo_fd);
	ctl.fifo_fd = -1;

	lseek(ctl.source_fd, ftell(fp), SEEK_SET);
}

void command_run() {
	ssize_t read_size;
	char *p;
	char *first_line;
	int i;

	int argc;
	char **argv;

	if (ctl.source_fd >= 0) {
		close(ctl.source_fd);
		fprintf(stderr, "re-run from the beginning\n");
	}

	read_size = get_first_line(&first_line, ctl.source_name);
	if (read_size < 2) {
		warn("failed to load %s", ctl.source_name);
		free(first_line);
		return;
	} else if (first_line[0] != '#' || first_line[1] != '!') {
		fprintf(stderr, "%s: missing shebang", ctl.source_name);
		free(first_line);
		return;
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

	ctl.source_fd = open(ctl.source_name, O_RDONLY);
	if (ctl.source_fd < 0) {
		warn("failed to open %s", ctl.source_name);
		return;
	}
	ctl.program_counter = 0;

	command_continue();
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
		command_continue();
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
	line = 0;
}

void finalize_slave() {
	int status = close_subshell(&ctl.subshell);
	if (status < 0) {
		warn("failed to waitpid");
	}
	fprintf(stderr, "shell terminated with %d\n", status);
}

int signal_handler(int sigfd) {
	struct signalfd_siginfo info;
	ssize_t bytes;

	bytes = read(sigfd, &info, sizeof(info));
	if (bytes < 0 && (errno == EAGAIN || errno == EINTR)) {
		return -1;
	} else if (bytes != sizeof(info)) {
		return 0;
	}

	switch (info.ssi_signo) {
	case SIGCHLD:
		finalize_slave();
		break;
	}
	return 0;
}

int get_signalfd() {
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGCHLD);

	sigprocmask(SIG_BLOCK, &sigset, NULL);

	int sigfd = signalfd(-1, &sigset, SFD_CLOEXEC);
	return sigfd;
}

int main(int argc, char **argv) {
	ssize_t lines;
	int sigfd;

	if (argc < 2) {
		err(EXIT_FAILURE, "not enough arguments");
	}
	ctl.source_name = argv[1];
	lines = count_lines(ctl.source_name);
	if (lines < 0) {
		err(EXIT_FAILURE, "failed to open source file");
	}
	ctl.breakpoints = bitset_new(lines);

	sigfd = get_signalfd();
	if (sigfd < 0) {
		err(EXIT_FAILURE, "failed to handle signals");
	}

	enum {
		POLLFD_STDIN,
		POLLFD_SIGNAL
	};

	struct pollfd pfd[] = {
		[POLLFD_STDIN] = { .fd = STDIN_FILENO, .events = POLLIN | POLLERR | POLLHUP },
		[POLLFD_SIGNAL] = { .fd = sigfd, .events = POLLIN | POLLERR | POLLHUP },
	};

	rl_callback_handler_install ("> ", readline_handler);

	signal(SIGCHLD, finalize_slave);

	while (!ctl.die) {
		int i, ret;

		ret = poll(pfd, ARRAY_SIZE(pfd), -1);
		if (ret < 0) {
			if (errno == EINTR) {
				continue;
			}

			warn("poll failed");
			fail();
		}
		for (i = 0; i < ARRAY_SIZE(pfd); ++i) {
			if (pfd[i].revents == 0) {
				continue;
			}
			if (i == POLLFD_STDIN) {
				rl_callback_read_char();
			} else if (i == POLLFD_SIGNAL) {
				signal_handler(sigfd);
			}
		}
	}

	done();

	return EXIT_FAILURE;
}

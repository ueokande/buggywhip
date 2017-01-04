#include <err.h>
#include <errno.h>
#include <paths.h>
#include <pty.h>
#include <signal.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/signalfd.h>
#include <sys/wait.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <readline/readline.h>

#include "strutil.h"
#include "fdutil.h"

struct bgw_control {
  char *shell;          /* shell to be executed */
  int master;           /* pseudoterminal master file descriptor */
  int slave;            /* pseudoterminal slave file descriptor */
  pid_t child;          /* child pid */
  int childstatus;      /* child process exit value */
  struct termios attrs; /* slave terminal runtime attributes */
  struct winsize win;   /* terminal window size */

  sigset_t sigset;      /* catch SIGCHLD and SIGWINCH with signalfd() */
  sigset_t sigorg;      /* original signal mask */
  int sigfd;            /* file descriptor for signalfd() */
};


static void done(struct bgw_control *ctl) {
  tcsetattr(STDIN_FILENO, TCSADRAIN, &ctl->attrs);
  kill(ctl->child, SIGTERM);

  exit(EXIT_SUCCESS);
}

static void fail(struct bgw_control *ctl) {
  kill(0, SIGTERM);
  done(ctl);
}

void command_do(struct bgw_control *ctl, const char *args) {
  size_t args_length = strlen(args);
  char *line = malloc(args_length + 1);
  memcpy(line, args, args_length);
  line[args_length] = '\n';
  if (write_all(ctl->master, line, args_length + 1)) {
    fail(ctl);
  }
  free(line);
  fdatasync(ctl->master);
}

void do_readline(struct bgw_control *ctl) {
  char *line;

  while ((line = readline("> "))) {
    add_history(line);

    if (line[0] == '\0') {
      continue;
    } else if (command_eq(line, "quit")) {
      break;
    } else if (command_eq(line, "do")) {
      const char *args = strchr(line, ' ');
      if (args != NULL) {
        command_do(ctl, ++args);
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
      fprintf(stdout, "Undefined command: \"%s\"\n",  line);
    }
  }
  free(line);

  done(ctl);
}

static void get_slave(struct bgw_control *ctl) {
  setsid();
  ioctl(ctl->slave, TIOCSCTTY, 0);
}

static void do_shell(struct bgw_control *ctl) {

  get_slave(ctl);

  close(ctl->master);
  close(ctl->sigfd);

  dup2(ctl->slave, STDIN_FILENO);
  close(ctl->slave);

  sigprocmask(SIG_SETMASK, &ctl->sigorg, NULL);

  execl("/bin/sh", "-s", NULL);

  warn("failed to execute shell");
  fail(ctl);
}

static void get_master(struct bgw_control *ctl) {
  if (tcgetattr(STDIN_FILENO, &ctl->attrs) != 0) {
    err(EXIT_FAILURE, "failed to get terminal attributes");
  }
  ioctl(STDIN_FILENO, TIOCGWINSZ, (char *)&ctl->win);
  if (openpty(&ctl->master, &ctl->slave, NULL, &ctl->attrs, &ctl->win)) {
    warn("openpty failed");
    fail(ctl);
  }
}

int main(void) {

  struct bgw_control ctl = {};

  get_master(&ctl);

  sigemptyset(&ctl.sigset);
  sigaddset(&ctl.sigset, SIGCHLD);
  sigaddset(&ctl.sigset, SIGWINCH);
  sigaddset(&ctl.sigset, SIGTERM);
  sigaddset(&ctl.sigset, SIGINT);
  sigaddset(&ctl.sigset, SIGQUIT);

  sigprocmask(SIG_BLOCK, &ctl.sigset, &ctl.sigorg);

  if ((ctl.sigfd = signalfd(-1, &ctl.sigset, SFD_CLOEXEC)) < 0) {
    err(EXIT_FAILURE, "cannot set signal handler");
  }

  fflush(stdout);
  ctl.child = fork();


  switch (ctl.child) {
  case -1:
    warn("fork failed");
    fail(&ctl);
    break;
  case 0:   // child process
    do_shell(&ctl);
    break;
  default:  // parent process
    do_readline(&ctl);
    break;
  }

  return EXIT_FAILURE;
}

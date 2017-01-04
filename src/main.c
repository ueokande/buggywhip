#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "strutil.h"

void do_readline() {
  char *line;

  while ((line = readline("> "))) {
    add_history(line);

    if (strcmp_eq(line, "quit")) {
      break;
    } else if (strcmp_eq(line, "next")) {
    } else if (strcmp_eq(line, "help")) {
    } else if (strcmp_eq(line, "list")) {
    } else if (strcmp_eq(line, "continue")) {
    } else if (strcmp_eq(line, "edit")) {
    } else if (strcmp_eq(line, "step")) {
    } else if (strcmp_eq(line, "print")) {
    } else if (strcmp_eq(line, "run")) {
    } else if (strcmp_eq(line, "backtrace")) {
    } else {
      fprintf(stdout, "Undefined command: \"%s\"\n",  line);
    }
  }
  free(line);
}

int main(void) {

  do_readline();

  return EXIT_SUCCESS;
}

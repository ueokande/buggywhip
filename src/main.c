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
    }

    free(line);
  }
}

int main(void) {

  do_readline();

  return EXIT_SUCCESS;
}

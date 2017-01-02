#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "strutil.h"

int main(void) {

  char *line;

  while ((line = readline("> "))) {
    add_history(line);

    if (strcmp_eq(line, "quit")) {
      break;
    }

    free(line);
  }

  return EXIT_SUCCESS;
}

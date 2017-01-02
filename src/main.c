#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

int main(void) {

  char *line;

  while ((line = readline("> "))) {
    add_history(line);

    free(line);
  }

  return EXIT_SUCCESS;
}

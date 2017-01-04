#ifndef STRUTIL_H
#define STRUTIL_H

#include <string.h>

/*
 * Compares first word in line with command
 */
static inline _Bool command_eq(const char *line, const char *command) {
  size_t len = strlen(command);
  return strncmp(line, command, len) == 0;
}

#endif

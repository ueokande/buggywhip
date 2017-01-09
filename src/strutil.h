#ifndef STRUTIL_H
#define STRUTIL_H

#include <string.h>
#include <ctype.h>

/*
 * Compares first word in line with command
 */
static inline _Bool command_eq(const char *line, const char *command) {
	size_t len = strlen(command);
	return strncmp(line, command, len) == 0;
}

/*
 * Skip whiltespaces on the head
 */
static inline char *trim_head(char *str) {
	while (isblank(*str)) {
		++str;
	}
	return str;
}

#endif

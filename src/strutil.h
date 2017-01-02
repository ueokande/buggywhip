#ifndef STRUTIL_H
#define STRUTIL_H

#include <string.h>

static inline _Bool strcmp_eq(const char *s1, const char *s2) {
  return strcmp(s1, s2) == 0;
}

#endif

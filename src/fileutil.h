#ifndef FILEUTIL_H
#define FILEUTIL_H

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

const char *strwordn(const char *str, ssize_t line_len, const char *word, size_t word_len) {
	const char *found;
	char before_char, after_char;
	int separate_start, separate_end;

	found = (char *)memmem(str, line_len, word, word_len);
	if (found == NULL) {
		return NULL;
	}

	before_char = *(found - 1);
	after_char = *(found + word_len);

	separate_start = found == str || (before_char != '_' && !isalnum(before_char));
	separate_end = after_char != '_' && !isalnum(after_char);
	if (separate_start && separate_end) {
		return found;
	}

	return NULL;
}

#endif

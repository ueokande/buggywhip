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

/*
 * The method finds a word from a file line by line.  The method returns a
 * number of line which contains the word.
 */
static int grep_word(const char *word, const char *filename) {
	FILE *fp;
	char *line;
	size_t len = 0;
	int errsv = 0;
	int num = -1;
	size_t word_len;

	word_len = strlen(word);

	if ((fp = fopen(filename, "r")) == NULL) {
		return -1;
	}

	while(1) {
		++num;
		ssize_t read_size;

		read_size = getline(&line, &len, fp);
		if (read_size < 0) {
			errsv = errno;
			num = -1;
			break;
		}

		if (strwordn(line, read_size, word, word_len) != NULL) {
			break;
		}
	}

	free(line);
	fclose(fp);

	errno = errsv;

	return num;
}

/*
 * count the number of lines in source file.  The last line is counted even if
 * the line does not end with a break-line.
 */
ssize_t count_lines(const char *filename) {
	ssize_t count = 0;
	FILE *fp;
	size_t len = 0;
	char *line;

	if ((fp = fopen(filename, "r")) == NULL) {
		return -1;
	}

	while(getline(&line, &len, fp) != -1) {
		++count;
	}

	fclose(fp);
	return count;
}

#endif

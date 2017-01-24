#include "fileutil.h"
#include <unistd.h>
#include <stdlib.h>

START_TEST(test_fileutil_strwordn) {
	int i;
	struct {
		char *data;
		char *word;
		int matched;
	} cases[] = {
		{"abc", "abc", 1},
		{"abc def ghi", "def", 1},
		{"abc(def)ghi", "def", 1},
		{"abcdefghi", "xyz", 0},
		{"abcdefghi", "def", 0},
		{"abcdefghi", "abc", 0},
		{"abcdefghi", "ghi", 0},
	};
	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) { 
		const char *ptr = strwordn(
				cases[i].data, strlen(cases[i].data),
				cases[i].word, strlen(cases[i].word));
		if (cases[i].matched) {
			ck_assert(ptr != NULL);
		} else {
			ck_assert(ptr == NULL);
		}
	}
}
END_TEST

START_TEST(test_fileutil_grep_word) {
	int i;
	struct {
		char *data;
		char *word;
		int expected_line;
	} cases[] = {
		{"abcdefghi\nabc def ghi\nabc def ghi\n", "def", 1},
		{"abcdefghi\nabc def ghi\nabc def ghi\n", "abcdefghi", 0},
		{"abc def ghi", "ghi", 0},
		{"abc def ghi", "xyz", -1},
		{"abc\ndef", "abc\ndef", -1},
	};
	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) { 
		int actual;
		char name[] = "/tmp/bgw-test-XXXXXX*";
		name[sizeof(name) - 2] = '\0';
		int fd = mkstemp(name);

		write(fd, cases[i].data, strlen(cases[i].data));
		close(fd);

		actual = grep_word(cases[i].word, name);
		ck_assert_int_eq(actual, cases[i].expected_line);

		unlink(name);
	}
}
END_TEST

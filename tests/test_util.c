#include "../src/util.h"

START_TEST(test_command_eq) {
	int i;
	struct {
		const char *line;
		const char *command;
		int match;
	} cases[] = {
		{ "run", "run", 1 },
		{ "run abc def", "run", 1 },
		{ "walk", "run", 0 },
		{ "walk abc def", "run", 0 },
		{ "walk and run", "run", 0 },
	};

	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
		int ret = command_eq(cases[i].line, cases[i].command);
		ck_assert_int_eq(ret, cases[i].match);
	}
}
END_TEST

START_TEST(test_trim_head) {
	int i;
	struct {
		char *string;
		int offset;
	} cases[] = {
		{ "  hello  ", 2 },
		{ "  \t  hello  ", 5 },
		{ "\n  hello  ", 0 },
	};

	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
		char *ret = trim_head(cases[i].string);
		ck_assert_ptr_eq(ret, cases[i].string + cases[i].offset);
	}
}
END_TEST

START_TEST(test_digit_number) {
	int i;
	struct {
		int value;
		int base;
		int number;
	} cases[] = {
		{ 99, 10, 2 },
		{ 100, 10, 3 },
		{ 0xFF, 16, 2 },
		{ 0x100, 16, 3 },
	};

	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
		int ret = digit_number(cases[i].value, cases[i].base);
		ck_assert_int_eq(ret, cases[i].number);
	}
}
END_TEST

START_TEST(test_check_lineno_support) {
	int i;
	struct {
		const char *command;
		int support;
	} cases[] = {
		{ "/bin/bash", 0 },
		{ "/bin/csh", 1 },
		{ "/bin/hogefugapiy", -1 },
	};

	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
		int ret = check_lineno_support(cases[i].command);
		ck_assert_int_eq(ret, cases[i].support);
		if (ret < 0) {
			ck_assert_int_ne(errno, 0);
		}
	}
}
END_TEST

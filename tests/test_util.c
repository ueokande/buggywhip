#include "../src/util.h"

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

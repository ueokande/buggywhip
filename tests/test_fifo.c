#include <check.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "fifo.h"

START_TEST(test_create_fifo) {
	struct bgw_fifo fifo;
	struct stat fifo_st, dir_st;
	char dir[256];

	create_fifo(&fifo);
	strcpy(dir, fifo.name);
	dirname(dir);

	stat(fifo.name, &fifo_st);
	stat(dir, &dir_st);

	ck_assert(fifo_st.st_mode & S_IFIFO);
	ck_assert(dir_st.st_mode & S_IFDIR);

	remove_fifo(&fifo);
}
END_TEST

START_TEST(test_remove_fifo) {
	struct bgw_fifo fifo;
	struct stat fifo_st, dir_st;
	char dir[256];

	create_fifo(&fifo);
	remove_fifo(&fifo);

	strcpy(dir, fifo.name);
	dirname(dir);

	ck_assert(stat(fifo.name, &fifo_st) < 0);
	ck_assert(stat(dir, &dir_st) < 0);
}
END_TEST

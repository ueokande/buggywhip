#include "bitset.h"

START_TEST(test_bitset_new) {
	int i;
	struct bitset *bs = bitset_new(10);

	for (i = 0; i < 10; ++i) {
		ck_assert_uint_eq(bitset_test(bs, i), 0);
	}

	bitset_delete(bs);
}
END_TEST

START_TEST(test_bitset_set) {
	int len = 10;
	int i, j;
	struct {
		int index;
		char expected[10];
		    
	} testcases[] = {
		{ 1, {0, 1, 0, 0, 0, 0, 0, 0, 0, 0}},
		{ 3, {0, 1, 0, 1, 0, 0, 0, 0, 0, 0}},
		{ 9, {0, 1, 0, 1, 0, 0, 0, 0, 0, 1}},
	};
	struct bitset *bs = bitset_new(10);

	for (i = 0; i < sizeof(testcases) / sizeof(testcases[0]); ++i) {
		bitset_set(bs, testcases[i].index);
		for (j = 0; j < len; ++j) {
			ck_assert_uint_eq(!!bitset_test(bs, j), testcases[i].expected[j]);
		}
	}

	bitset_delete(bs);
}
END_TEST

START_TEST(test_bitset_reset) {
	int len = 10;
	int i, j;
	struct {
		int index;
		char expected[10];
	} testcases[] = {
		{ 1, {1, 0, 1, 1, 1, 1, 1, 1, 1, 1}},
		{ 3, {1, 0, 1, 0, 1, 1, 1, 1, 1, 1}},
		{ 9, {1, 0, 1, 0, 1, 1, 1, 1, 1, 0}},
	};
	struct bitset *bs = bitset_new(10);
	for (i = 0; i < len; ++i) {
		bitset_set(bs, i);
	}

	for (i = 0; i < sizeof(testcases) / sizeof(testcases[0]); ++i) {
		bitset_reset(bs, testcases[i].index);
		for (j = 0; j < len; ++j) {
			ck_assert_uint_eq(!!bitset_test(bs, j), testcases[i].expected[j]);
		}
	}

	bitset_delete(bs);
}
END_TEST

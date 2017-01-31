#include <check.h>
#include <check_stdint.h>
#include <stdio.h>
#include "test_fifo.c"
#include "test_fileutil.c"
#include "test_bitset.c"

int main(void) {

    int nf;

    Suite *s1 = suite_create("Statsite");
    TCase *tc1 = tcase_create("fifo");
    TCase *tc2 = tcase_create("fileutil");
    TCase *tc3 = tcase_create("bitset");
    SRunner *sr = srunner_create(s1);

    suite_add_tcase(s1, tc1);
    tcase_add_test(tc1, test_create_fifo);
    tcase_add_test(tc1, test_remove_fifo);

    suite_add_tcase(s1, tc2);
    tcase_add_test(tc2, test_fileutil_strwordn);
    tcase_add_test(tc2, test_fileutil_grep_word);
    tcase_add_test(tc2, test_fileutil_count_lines);
    tcase_add_test(tc2, test_fileutil_get_first_line);

    suite_add_tcase(s1, tc3);
    tcase_add_test(tc3, test_bitset_new);
    tcase_add_test(tc3, test_bitset_set);
    tcase_add_test(tc3, test_bitset_reset);

    srunner_run_all(sr, CK_ENV);
    nf = srunner_ntests_failed(sr);
    srunner_free(sr);

    return nf == 0 ? 0 : 1;
}

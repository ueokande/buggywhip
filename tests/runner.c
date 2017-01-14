#include <check.h>
#include <stdio.h>
#include "test_fifo.c"

int main(void) {

    int nf;

    Suite *s1 = suite_create("Statsite");
    TCase *tc1 = tcase_create("fifo");
    SRunner *sr = srunner_create(s1);

    // Add the fifo tests
    suite_add_tcase(s1, tc1);
    tcase_add_test(tc1, test_create_fifo);
    tcase_add_test(tc1, test_remove_fifo);


    srunner_run_all(sr, CK_ENV);
    nf = srunner_ntests_failed(sr);
    srunner_free(sr);

    return nf == 0 ? 0 : 1;
}

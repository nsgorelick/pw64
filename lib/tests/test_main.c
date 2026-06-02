#include "test.h"

int test_failures;
int test_runs;
const char *test_current;

void register_xfred_tests(void);

void test_report(void) { fprintf(stderr, "Ran %d tests, %d failures\n", test_runs, test_failures); }

int main(void)
{
    test_failures = 0;
    test_runs = 0;

    register_xfred_tests();

    test_report();
    return test_failures != 0 ? 1 : 0;
}

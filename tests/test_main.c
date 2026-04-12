#include "utils.h"

#include "unity/unity.h"

void 
setUp(void) {
}

void 
tearDown(void) {
}

int 
run_all_parser_tests(void);

int 
run_all_simulation_tests(void);

int main(void) {
    int res;
    res = run_all_parser_tests();
    if (res != 0) return res;
    res = run_all_simulation_tests();
    if (res != 0) return res;
    return res;
}

#include "unity/unity.h"

void 
setUp(void) {

}

void 
tearDown(void) {

}

void test_false(void) {
    TEST_ASSERT_EQUAL_INT(42, 42);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_false);
    return UNITY_END();
}

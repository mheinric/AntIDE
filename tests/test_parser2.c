#include "unity/unity.h"
#include "parser2.h"


void test_parse2_empty_program() {
    Parse2Result result = parse2_program_from_string("");
    TEST_ASSERT_EQUAL_UINT64(0, error_list_size(&result.errors));
    TEST_ASSERT_EQUAL_UINT64(0, program2_size(&result.program));
    parse2_result_clear(&result);
}

void test_parse2_single_instruction_no_arg() {
    enum { NB_ITEMS = 4 };
    const char* inst_str[NB_ITEMS] = {
        "PICKUP", 
        "pickup",
        "DROP",
        "drop",
    };
    
    InstructionType inst_type[NB_ITEMS] = {
        INST_PICKUP, 
        INST_PICKUP, 
        INST_DROP,
        INST_DROP
    };

    for (int i = 0; i < NB_ITEMS; i++)
    {
        Parse2Result result = parse2_program_from_string(inst_str[i]);
        TEST_ASSERT_EQUAL_UINT64(0, error_list_size(&result.errors));
        TEST_ASSERT_EQUAL_UINT64(1, program2_size(&result.program));
        TEST_ASSERT_EQUAL(inst_type[i], result.program.begin->type);
        parse2_result_clear(&result);
    }
}

void test_parse2_invalid_instruction() {
    enum { NB_ITEMS = 1 };
    const char* inst_str[NB_ITEMS] = {
        "toto", 
    };

    for (int i = 0; i < NB_ITEMS; i++)
    {
        Parse2Result result = parse2_program_from_string(inst_str[i]);
        TEST_ASSERT_EQUAL_UINT64(1, error_list_size(&result.errors));
        TEST_ASSERT_EQUAL_UINT64(0, program2_size(&result.program));
        parse2_result_clear(&result);
    }

}

int run_all_parser2_tests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_parse2_empty_program);
    RUN_TEST(test_parse2_single_instruction_no_arg);
    RUN_TEST(test_parse2_invalid_instruction);
    return UNITY_END();
}

#include "unity/unity.h"
#include "parser2.h"

void 
test_instruction_equality() 
{
    TEST_ASSERT_FALSE(instruction_equal(
        instruction_create_move(argument_create_register(0)), 
        instruction_create_move(argument_create_register(1))
    ));
}


void 
test_parse2_empty_program() {
    Parse2Result result = parse2_program_from_string("");
    TEST_ASSERT_EQUAL_UINT64(0, vector_parse_error_size(&result.errors));
    TEST_ASSERT_EQUAL_UINT64(0, program2_size(&result.program));
    parse2_result_clear(&result);
}

void 
test_parse2_single_instruction_no_arg() {
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
        TEST_ASSERT_EQUAL_UINT64(0, vector_parse_error_size(&result.errors));
        TEST_ASSERT_EQUAL_UINT64(1, program2_size(&result.program));
        TEST_ASSERT_EQUAL(inst_type[i], result.program.instructions.begin->type);
        parse2_result_clear(&result);
    }
}

void 
test_parse2_single_instruction_arg1() {
    //Test instructions with a single argument.
    enum { NB_TESTS = 2 };
    const char* test_instructions[NB_TESTS] = {
        "MOVE r0", 
        "MOVE r1"
    };

    const Instruction expected_results[NB_TESTS] = {
        instruction_create_move(argument_create_register(0)),
        instruction_create_move(argument_create_register(1)),
    };

    for (int i = 0; i < NB_TESTS; i++)
    {
        Parse2Result result = parse2_program_from_string(test_instructions[i]);
        TEST_ASSERT_EQUAL_UINT64(0, vector_parse_error_size(&result.errors));
        TEST_ASSERT_EQUAL_UINT64(1, program2_size(&result.program));
        TEST_ASSERT_TRUE(instruction_equal(expected_results[i], result.program.instructions.begin[0]));
        parse2_result_clear(&result);
    }
}

void 
test_parse2_invalid_instruction() {
    enum { NB_ITEMS = 5 };
    const char* inst_str[NB_ITEMS] = {
        "toto",
        "foo-bar",
        //Missing arguments
        "MOVE",
        //Too many arguments
        "PICKUP test",
        "DROP HERE",
    };

    for (int i = 0; i < NB_ITEMS; i++)
    {
        Parse2Result result = parse2_program_from_string(inst_str[i]);
        TEST_ASSERT_EQUAL_UINT64(1, vector_parse_error_size(&result.errors));
        TEST_ASSERT_EQUAL_UINT64(1, result.errors.begin[0].line);
        TEST_ASSERT_EQUAL_UINT64(0, program2_size(&result.program));
        parse2_result_clear(&result);
    }
}

int run_all_parser2_tests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_instruction_equality);
    RUN_TEST(test_parse2_empty_program);
    RUN_TEST(test_parse2_single_instruction_no_arg);
    RUN_TEST(test_parse2_single_instruction_arg1);
    RUN_TEST(test_parse2_invalid_instruction);
    return UNITY_END();
}

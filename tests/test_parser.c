#include "unity/unity.h"
#include "parser.h"

void 
test_instruction_equality() 
{
    TEST_ASSERT_FALSE(instruction_equal(
        instruction_create_move(argument_create_register(0)), 
        instruction_create_move(argument_create_register(1))
    ));
    TEST_ASSERT_FALSE(instruction_equal(
        instruction_create_move(argument_create_value(0)), 
        instruction_create_move(argument_create_value(1))
    ));
}


void 
test_parse_read_empty_file() {
    parseResult result = parse_program_from_file("tests/sample_programs/empty_file.asm");
    TEST_ASSERT_EQUAL_UINT64(0, vector_parse_error_size(&result.errors));
    TEST_ASSERT_EQUAL_UINT64(0, Program_size(&result.program));
    parse_result_clear(&result);    
}

void 
test_parse_read_inexistant_file() {
    parseResult result = parse_program_from_file("tests/sample_programs/missing-file.asm");
    TEST_ASSERT_EQUAL_UINT64(1, vector_parse_error_size(&result.errors));
    TEST_ASSERT_EQUAL_UINT64(0, Program_size(&result.program));
    parse_result_clear(&result);    
}

void 
test_parse_empty_program() {
    parseResult result = parse_program_from_string("");
    TEST_ASSERT_EQUAL_UINT64(0, vector_parse_error_size(&result.errors));
    TEST_ASSERT_EQUAL_UINT64(0, Program_size(&result.program));
    parse_result_clear(&result);
}

void 
test_parse_single_instruction_no_arg() {
    enum { NB_ITEMS = 5 };
    const char* inst_str[NB_ITEMS] = {
        "PICKUP", 
        "pickup",
        "DROP",
        "drop",
        "drop ; This is a comment",
    };
    
    InstructionType inst_type[NB_ITEMS] = {
        INST_PICKUP, 
        INST_PICKUP, 
        INST_DROP,
        INST_DROP,
        INST_DROP
    };

    for (int i = 0; i < NB_ITEMS; i++)
    {
        parseResult result = parse_program_from_string(inst_str[i]);
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(0, vector_parse_error_size(&result.errors), inst_str[i]);
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(1, Program_size(&result.program), inst_str[i]);
        TEST_ASSERT_EQUAL_MESSAGE(inst_type[i], result.program.instructions.begin->type, inst_str[i]);
        parse_result_clear(&result);
    }
}

void 
test_parse_single_instruction_arg1() {
    //Test instructions with a single argument.
    enum { NB_TESTS = 6 };
    const char* test_instructions[NB_TESTS] = {
        "MOVE r0", 
        "MOVE r1",
        "MOVE 2",
        "MOVE -165",
        "MOVE NORTH",
        "MOVE HERE",
    };

    const Instruction expected_results[NB_TESTS] = {
        instruction_create_move(argument_create_register(0)),
        instruction_create_move(argument_create_register(1)),
        instruction_create_move(argument_create_value(2)),
        instruction_create_move(argument_create_value(-165)),
        instruction_create_move(argument_create_value(1)),
        instruction_create_move(argument_create_value(0)),
    };

    for (int i = 0; i < NB_TESTS; i++)
    {
        parseResult result = parse_program_from_string(test_instructions[i]);
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(0, vector_parse_error_size(&result.errors), test_instructions[i]);
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(1, Program_size(&result.program), test_instructions[i]);
        TEST_ASSERT_TRUE_MESSAGE(instruction_equal(expected_results[i], result.program.instructions.begin[0]), test_instructions[i]);
        parse_result_clear(&result);
    }
}

void 
test_parse_invalid_instruction() {
    enum { NB_ITEMS = 6 };
    const char* inst_str[NB_ITEMS] = {
        "toto",
        "foo-bar",
        //Missing arguments
        "MOVE",
        //Too many arguments
        "PICKUP test",
        "DROP HERE",
        //Invalid number format
        "MOVE 1ABD", 
    };

    for (int i = 0; i < NB_ITEMS; i++)
    {
        parseResult result = parse_program_from_string(inst_str[i]);
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(1, vector_parse_error_size(&result.errors), inst_str[i]);
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(1, result.errors.begin[0].line, inst_str[i]);
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(0, Program_size(&result.program), inst_str[i]);
        parse_result_clear(&result);
    }
}

int run_all_parser_tests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_instruction_equality);
    RUN_TEST(test_parse_read_empty_file);
    RUN_TEST(test_parse_read_inexistant_file);
    RUN_TEST(test_parse_empty_program);
    RUN_TEST(test_parse_single_instruction_no_arg);
    RUN_TEST(test_parse_single_instruction_arg1);
    RUN_TEST(test_parse_invalid_instruction);
    return UNITY_END();
}

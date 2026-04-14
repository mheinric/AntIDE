#include "parser.h"
#include <unity/unity.h>

void TEST_ASSERT_NO_ERRORS(ParseResult *parse_result) {
    if (vector_parse_error_size(&parse_result->errors) > 0) 
    {
        parse_result_print_errors(parse_result);
        TEST_FAIL_MESSAGE("Parse result contains errors");
    }
}

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
    ParseResult result = parse_program_from_file("tests/sample_programs/empty_file.asm");
    TEST_ASSERT_NO_ERRORS(&result);
    TEST_ASSERT_EQUAL_UINT64(0, program_size(&result.program));
    parse_result_cleanup(&result);    
}

void 
test_parse_read_inexistant_file() {
    ParseResult result = parse_program_from_file("tests/sample_programs/missing-file.asm");
    TEST_ASSERT_EQUAL_UINT64(1, vector_parse_error_size(&result.errors));
    TEST_ASSERT_EQUAL_UINT64(0, program_size(&result.program));
    parse_result_cleanup(&result);    
}

void 
test_parse_empty_program() {
    ParseResult result = parse_program_from_string("");
    TEST_ASSERT_NO_ERRORS(&result);
    TEST_ASSERT_EQUAL_UINT64(0, program_size(&result.program));
    parse_result_cleanup(&result);
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
        ParseResult result = parse_program_from_string(inst_str[i]);
        TEST_ASSERT_NO_ERRORS(&result);
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(1, program_size(&result.program), inst_str[i]);
        TEST_ASSERT_EQUAL_MESSAGE(inst_type[i], result.program.instructions.begin->type, inst_str[i]);
        parse_result_cleanup(&result);
    }
}

void 
test_parse_single_instructions() {
    //Test instructions with a single argument.

    const struct { const char* inst_string; Instruction expected_inst; } test_cases[] = {
        {
            .inst_string = "MOVE r0", 
            .expected_inst = instruction_create_move(argument_create_register(0)),
        },
        {
            .inst_string = "MOVE r1",
            .expected_inst = instruction_create_move(argument_create_register(1)),
        },
        {
            .inst_string = "MOVE 2",
            .expected_inst = instruction_create_move(argument_create_value(2)),
        },
        {
            .inst_string = "MOVE -165",
            .expected_inst = instruction_create_move(argument_create_value(-165)),
        },
        {
            .inst_string = "MOVE NORTH",
            .expected_inst = instruction_create_move(argument_create_value(DIR_NORTH)),
        },
        {
            .inst_string = "MOVE HERE",
            .expected_inst = instruction_create_move(argument_create_value(DIR_HERE)),
        },
        {
            .inst_string = "JMP r0",
            .expected_inst = instruction_create_jump(argument_create_register(0)),
        },
        {
            .inst_string = "ID r0",
            .expected_inst = instruction_create_info(INST_ID, 0),
        },
        {
            .inst_string = "CARRYING r0",
            .expected_inst = instruction_create_info(INST_CARRY, 0),
        },
        {
            .inst_string = "MARK CH_BLUE 20",
            .expected_inst = instruction_create_mark(argument_create_value(1), argument_create_value(20)),
        },
        {
            .inst_string = "SNIFF CH_RED HERE r1",
            .expected_inst = instruction_create_sniff(argument_create_value(CH_RED), argument_create_value(0), 1),
        },
        {
            .inst_string = "SMELL CH_YELLOW r2",
            .expected_inst = instruction_create_smell(argument_create_value(CH_YELLOW), 2),
        },
        {
            .inst_string = "PROBE NORTH r0",
            .expected_inst = instruction_create_probe(argument_create_value(DIR_NORTH), 0),
        },
        {
            .inst_string = "SENSE WALL r2",
            .expected_inst = instruction_create_sense(argument_create_value(1), 2),
        },
        {
            .inst_string = "TAG 5",
            .expected_inst = instruction_create_tag(argument_create_value(5)),
        },
    };
    enum { NB_TESTS = sizeof(test_cases) / sizeof(test_cases[0]) };

    for (int i = 0; i < NB_TESTS; i++)
    {
        ParseResult result = parse_program_from_string(test_cases[i].inst_string);
        TEST_ASSERT_NO_ERRORS(&result);
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(1, program_size(&result.program), test_cases[i].inst_string);
        TEST_ASSERT_TRUE_MESSAGE(instruction_equal(test_cases[i].expected_inst, result.program.instructions.begin[0]), test_cases[i].inst_string);
        parse_result_cleanup(&result);
    }
}

void
test_parse_single_instruction_arithmetic() {
    const char* test_instructions[] = {
        "SET r5 10",
        "SET r5 r4",
        "ADD r3 10",
        "SUB r2 50",
        "MOD r1 10",
        "MUL r1 10",
        "DIV r1 10",
        "AND r1 10",
        "OR  r1 0xFF",
        "XOR  r1 0b1100",
        "LSHIFT r0 2",
        "RSHIFT r0 2",
        "RANDOM r0 10",
    };
    enum { NB_TESTS = sizeof(test_instructions) / sizeof(test_instructions[0]) };

    const Instruction expected_results[NB_TESTS] = {
        instruction_create_arithmetic(INST_SET, 5, argument_create_value(10)),
        instruction_create_arithmetic(INST_SET, 5, argument_create_register(4)),
        instruction_create_arithmetic(INST_ADD, 3, argument_create_value(10)),
        instruction_create_arithmetic(INST_SUB, 2, argument_create_value(50)),
        instruction_create_arithmetic(INST_MOD, 1, argument_create_value(10)),
        instruction_create_arithmetic(INST_MUL, 1, argument_create_value(10)),
        instruction_create_arithmetic(INST_DIV, 1, argument_create_value(10)),
        instruction_create_arithmetic(INST_AND, 1, argument_create_value(10)),
        instruction_create_arithmetic(INST_OR, 1, argument_create_value(0xFF)),
        instruction_create_arithmetic(INST_XOR, 1, argument_create_value(0b1100)),
        instruction_create_arithmetic(INST_LSHIFT, 0, argument_create_value(2)),
        instruction_create_arithmetic(INST_RSHIFT, 0, argument_create_value(2)),
        instruction_create_arithmetic(INST_RANDOM, 0, argument_create_value(10)),
    };

    for (int i =0; i < NB_TESTS; i++)
    {
        ParseResult result = parse_program_from_string(test_instructions[i]);
        TEST_ASSERT_NO_ERRORS(&result);
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(1, program_size(&result.program), test_instructions[i]);
        TEST_ASSERT_TRUE_MESSAGE(instruction_equal(expected_results[i], result.program.instructions.begin[0]), test_instructions[i]);
        parse_result_cleanup(&result);
    }
}

void 
test_parse_invalid_instruction() {
    const char* inst_str[] = {
        "toto",
        "foo-bar",
        //Missing arguments
        "MOVE",
        //Too many arguments
        "PICKUP test",
        "DROP HERE",
        //Invalid number format
        "MOVE 1ABD", 
        //Invalid arguments (expect register but got number)
        "CALL 10 10",
        "SET 10 r0",
        "ADD 5 10",
        "CARRYING 10",
    };
    enum { NB_ITEMS = sizeof(inst_str) / sizeof(inst_str[0]) };

    for (int i = 0; i < NB_ITEMS; i++)
    {
        ParseResult result = parse_program_from_string(inst_str[i]);
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(1, vector_parse_error_size(&result.errors), inst_str[i]);
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(1, result.errors.begin[0].line, inst_str[i]);
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(0, program_size(&result.program), inst_str[i]);
        parse_result_cleanup(&result);
    }
}

void 
test_parse_multiple_instructions() {
    const char* program = 
        "main:\n"
        "MOVE r0\n"
        "PICKUP";
    ParseResult result = parse_program_from_string(program);
    TEST_ASSERT_NO_ERRORS(&result);
    TEST_ASSERT_EQUAL_UINT64(2, program_size(&result.program));
    parse_result_cleanup(&result);
}

void 
test_parse_conditional_jumps() {
    char program[] = 
        "main:\n"
        "ADD r0 1\n"
        "XXX r0 10 end\n"
        "end:\n"
        "PICKUP";
    struct { InstructionType inst_type; const char inst_name[3]; } test_entries[] = {
        { .inst_type = INST_JEQ, .inst_name = "JEQ" },
        { .inst_type = INST_JNE, .inst_name = "JNE" },
        { .inst_type = INST_JGT, .inst_name = "JGT" },
        { .inst_type = INST_JLT, .inst_name = "JLT" },
    };
    enum { NB_TESTS = sizeof(test_entries) / sizeof(test_entries[0]) };
    for (int i = 0; i < NB_TESTS; i++)
    {
        for (int k = 0; k < 3; k++)
        {
            program[15 + k] = test_entries[i].inst_name[k];
        }

        ParseResult result = parse_program_from_string(program);
        TEST_ASSERT_NO_ERRORS(&result);
        TEST_ASSERT_EQUAL_UINT64(3, program_size(&result.program));
        TEST_ASSERT_TRUE(instruction_equal(
            result.program.instructions.begin[1],
            instruction_create_conditional_jump(test_entries[i].inst_type, argument_create_register(0), argument_create_value(10), argument_create_value(2))
        ));
        parse_result_cleanup(&result);
    }
}

void
test_parse_call(void)
{
    ParseResult result = parse_program_from_string("CALL r0 r1");
    TEST_ASSERT_NO_ERRORS(&result);
    TEST_ASSERT_EQUAL_UINT64(1, program_size(&result.program));
    TEST_ASSERT_TRUE(instruction_equal(
        instruction_create_call(0, argument_create_register(1)),
        result.program.instructions.begin[0]
    ));
}

int 
run_all_parser_tests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_instruction_equality);
    RUN_TEST(test_parse_read_empty_file);
    RUN_TEST(test_parse_read_inexistant_file);
    RUN_TEST(test_parse_empty_program);
    RUN_TEST(test_parse_single_instruction_no_arg);
    RUN_TEST(test_parse_single_instructions);
    RUN_TEST(test_parse_single_instruction_arithmetic);
    RUN_TEST(test_parse_invalid_instruction);
    RUN_TEST(test_parse_multiple_instructions);
    RUN_TEST(test_parse_conditional_jumps);
    return UNITY_END();
}

#include "unity/unity.h"
#include "parser.h"

void test_parse_identifier(void) {
    const char* input = "id1 id2";
    const char* current_pos = input;
    Token res1 = read_identifier(&current_pos);
    Token res2 = read_identifier(&current_pos);
    Token res3 = read_identifier(&current_pos);

    TEST_ASSERT_EQUAL_PTR(input, res1.begin); //First identifier is at the beginning of the string
    TEST_ASSERT_EQUAL_PTR(input + 3, res1.end);
    TEST_ASSERT_EQUAL_PTR(input + 4, res2.begin); //Second identifier starts at 4
    TEST_ASSERT_EQUAL_PTR(input + 7, res2.end);
    TEST_ASSERT_EQUAL_PTR(NULL, res3.begin);
    TEST_ASSERT_EQUAL_PTR(NULL, res3.end);
}

void test_parse_empty_file(void) {
    Program program = parse_program_from_file("tests/sample_programs/empty_file.asm");
    TEST_ASSERT_EQUAL_PTR(program.instructions.begin, program.instructions.end);
}

void test_parse_single_instruction(void) {
    Program program = parse_program_from_file("tests/sample_programs/single_instruction.asm");
    TEST_ASSERT_EQUAL_UINT64(1, program_nb_instructions(&program));
    TEST_ASSERT_TRUE(token_equals_str(&program.instructions.begin->name, "PICKUP"));
    token_is_null(program.instructions.begin->arguments);
}

void test_parse_instruction_arguments(void) {
    Program program = parse_program_from_string("MOVE r0");
    TEST_ASSERT_EQUAL_UINT64(1, program_nb_instructions(&program));
    TEST_ASSERT_TRUE(token_equals_str(program.instructions.begin->arguments, "r0"));
    TEST_ASSERT_TRUE(token_is_null(program.instructions.begin->arguments + 1));
}

void test_parse_multiple_instructions(void) {
    Program program = parse_program_from_string("MOVE r0\nSNIFF RED_CH NORTH");
    TEST_ASSERT_EQUAL_UINT64(2, program_nb_instructions(&program));
}

void test_parse_comments(void) {
    Program program = parse_program_from_string("; this is commented and should be ignored MOVE r0\nSNIFF RED_CH NORTH ; comment at the end of an instruction");
    TEST_ASSERT_EQUAL_UINT64(1, program_nb_instructions(&program));
}

void test_parse_label(void) {
    Program program = parse_program_from_string("main:\nJMP main");
    TEST_ASSERT_EQUAL_UINT64(1, program_nb_labels(&program));
    TEST_ASSERT_EQUAL_UINT64(1, program_nb_instructions(&program));
}

void test_insert_instructions(void) {
    InstructionLinesVect vect; 
    instruction_lines_vect_init(&vect, 3); 
    InstructionLine inst; 
    instruction_line_init(&inst);
    for (int i = 0; i < 10; i++)
    {
        instruction_lines_vect_push(&vect, inst);
    }
    TEST_ASSERT_EQUAL_UINT32(10, instruction_lines_vect_size(&vect));
    TEST_ASSERT_TRUE(vect.capacity >= instruction_lines_vect_size(&vect));
}

void test_insert_labels(void) {
    const char* data = "abcdefghijklmopqrstu";
    LabelsMap map; 
    labels_map_init(&map, 3); 
    for (int i = 0; i < 10; i++)
    {
        Token tok; 
        tok.begin = data + i; 
        tok.end = data + i + 1;
        labels_map_insert(&map, tok, i);
    }
    TEST_ASSERT_EQUAL_UINT32(10, labels_map_size(&map));
    TEST_ASSERT_TRUE(map.capacity >= labels_map_size(&map));
}

int run_all_parser_tests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_parse_identifier);
    RUN_TEST(test_parse_empty_file);
    RUN_TEST(test_parse_single_instruction);
    RUN_TEST(test_parse_instruction_arguments);
    RUN_TEST(test_parse_multiple_instructions);
    RUN_TEST(test_parse_comments);
    RUN_TEST(test_parse_label);
    RUN_TEST(test_insert_instructions);
    RUN_TEST(test_insert_labels);
    return UNITY_END();
}

#pragma once
#include "utils.h"
#include "ast.h"

typedef struct {
    char* message;
    size_t line;
} ParseError;

#define VECTOR_ITEM_TYPE ParseError
#define VECTOR_ITEM_NAME parse_error
#include "vector.h"
#undef VECTOR_ITEM_TYPE
#undef VECTOR_ITEM_NAME


typedef struct {
    Program2 program;
    VectorParseError errors;
} Parse2Result;

void 
parse2_result_init(Parse2Result *parse_result);

void 
parse2_result_clear(Parse2Result *parse_result);

Parse2Result
parse2_program_from_string(const char *content);

Parse2Result 
parse2_program_from_file(const char* file_path);
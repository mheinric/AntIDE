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
    Program program;
    VectorParseError errors;
} parseResult;

void 
parse_result_init(parseResult *parse_result);

void 
parse_result_clear(parseResult *parse_result);

parseResult
parse_program_from_string(const char *content);

parseResult 
parse_program_from_file(const char* file_path);
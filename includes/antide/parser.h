#pragma once
#include "utils.h"
#include "ast.h"

typedef struct {
    const char* message;
    size_t line;
} ParseError;

typedef struct VectorParseError VectorParseError;

typedef struct {
    Program program;
    VectorParseError* errors;
} ParseResult;

void 
parse_result_init(ParseResult *parse_result);

void
parse_result_init_move(ParseResult *dest, ParseResult *source);

void 
parse_result_cleanup(ParseResult *parse_result);

void 
parse_result_print_errors(const ParseResult *parse_result);

size_t 
parse_result_nb_errors(const ParseResult *parse_result);

ParseError
parse_result_get_error(const ParseResult *parse_result, size_t index);

ParseResult
parse_program_from_string(const char *content);

ParseResult 
parse_program_from_file(const char* file_path);
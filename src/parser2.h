#pragma once
#include "utils.h"
#include "ast.h"

typedef struct {} Parse2ErrorList;

void 
parse2_error_list_init(Parse2ErrorList *error_list);

void 
parse2_error_list_clear(Parse2ErrorList *error_list);

void
parse2_error_list_move(Parse2ErrorList *target, Parse2ErrorList *source);

typedef struct {
    Program2 program;
    Parse2ErrorList errors;
} Parse2Result;

void 
parse2_result_init(Parse2Result *parse_result);

void 
parse2_result_clear(Parse2Result *parse_result);

void 
parse2_result_move(Parse2Result *target, Parse2Result *source);

uint64_t error_list_size(Parse2ErrorList *parse_result);

Parse2Result
parse2_program_from_string(const char *content);
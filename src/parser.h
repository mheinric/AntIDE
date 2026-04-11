#pragma once
#include "utils.h"

//Maximum number of arguments an instruction can have.
#define MAX_INSTRUCTION_ARGS 5

typedef struct {
    const char* begin; 
    const char* end;
} Token;

static const Token NULL_TOKEN = {
    .begin = NULL,
    .end = NULL
};

bool 
token_equals_str(const Token* token, const char* str);

bool 
token_is_null(const Token* token);

void 
token_print(const Token* token);

typedef struct {
    Token name; 
    Token arguments[MAX_INSTRUCTION_ARGS];
} InstructionLine;

void 
instruction_line_init(InstructionLine *instruction);

typedef struct {
    InstructionLine *begin;
    InstructionLine *end;
    uint64_t capacity;
} InstructionLinesVect;

void 
instruction_lines_vect_init(InstructionLinesVect *vect, uint64_t capacity);

uint64_t
instruction_lines_vect_size(const InstructionLinesVect *vect);

void 
instruction_lines_vect_push(InstructionLinesVect *vect, InstructionLine inst);

typedef struct {
    Token key;
    uint64_t value;
} LabelsMapEntry;

typedef struct {
    LabelsMapEntry *begin; 
    LabelsMapEntry *end; 
    uint64_t capacity;
} LabelsMap;

void
labels_map_init(LabelsMap *map, uint64_t capacity);

uint64_t 
labels_map_size(LabelsMap *map);

void
labels_map_insert(LabelsMap *map, Token token, uint64_t pos);

typedef struct {
    InstructionLinesVect instructions;
    LabelsMap labels;
} Program;

void 
program_init(Program *prog); //TODO create a cleanup function to de-allocate.

uint64_t
program_nb_instructions(Program *prog);

uint64_t
program_nb_labels(Program *prog);

Program
parse_program_from_file(const char *file_name);

Program
parse_program_from_string(const char *content);

Token
read_identifier(const char **input_ptr);

void
read_program_line(Program *prog, const char **input_ptr);
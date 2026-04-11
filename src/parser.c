#include "utils.h"
#include "parser.h"

bool token_equals_str(const Token *token, const char *str)
{
    const size_t size = strlen(str);
    return !token_is_null(token) && 
        size == (long unsigned int) (token->end - token->begin) && 
        strncmp(token->begin, str, size) == 0;
}

bool token_is_null(const Token *token)
{
    return token->begin == NULL && token->end == NULL;
}

void token_print(const Token *token)
{
    if (token_is_null(token))
    {
        printf("NULL_TOKEN");
        return;
    }
    printf("'%.*s'", (int) (token->end - token->begin), token->begin);
}

void instruction_line_init(InstructionLine *instr)
{
    instr->name = NULL_TOKEN;
    for (uint64_t i = 0; i < MAX_INSTRUCTION_ARGS; i++) {
        instr->arguments[i] = NULL_TOKEN;
    }
}

void instruction_lines_vect_init(InstructionLinesVect *vect, uint64_t size)
{
    vect->begin = calloc(size, sizeof(InstructionLine));
    vect->end = vect->begin;
    vect->capacity = size;
}

uint64_t instruction_lines_vect_size(const InstructionLinesVect *vect)
{
    return vect->end - vect->begin;
}

void instruction_lines_vect_push(InstructionLinesVect *vect, InstructionLine inst)
{
    const uint64_t size = instruction_lines_vect_size(vect);
    if (size >= vect->capacity) 
    {
        //Allocate new memory
        const uint64_t new_capacity = vect->capacity * 2;
        InstructionLine* new_content = calloc(new_capacity, sizeof(InstructionLine));
        //Copy data to the new memory
        memcpy(new_content, vect->begin, vect->capacity * sizeof(InstructionLine));
        //Free old memory
        free(vect->begin);
        vect->capacity = new_capacity; 
        vect->begin = new_content; 
        vect->end = vect->begin + size;
    }
    *vect->end = inst;
    vect->end++;
}

void labels_map_init(LabelsMap *map, uint64_t capacity)
{
    map->begin = calloc(capacity, sizeof(LabelsMapEntry));
    map->end = map->begin; 
    map->capacity = capacity;
}

uint64_t labels_map_size(LabelsMap *map)
{
    return map->end - map->begin;
}

void labels_map_insert(LabelsMap *map, Token token, uint64_t pos)
{
    const uint64_t size = labels_map_size(map);
    if ((uint64_t) (map->end - map->begin) >= map->capacity) {
        //Allocate new memory
        const uint64_t new_capacity = map->capacity * 2;
        LabelsMapEntry* new_content = calloc(new_capacity, sizeof(LabelsMapEntry));
        //Copy data to the new memory
        memcpy(new_content, map->begin, map->capacity * sizeof(LabelsMapEntry));
        //Free old memory
        free(map->begin);
        map->capacity = new_capacity; 
        map->begin = new_content; 
        map->end = map->begin + size;
    }
    map->end->key = token;
    map->end->value = pos;
    map->end++;
}

void program_init(Program *prog)
{
    instruction_lines_vect_init(&prog->instructions, 5); //Arbitrary initial capacity
    labels_map_init(&prog->labels, 5);
}

uint64_t program_nb_instructions(Program *prog)
{
    return instruction_lines_vect_size(&prog->instructions);
}

uint64_t program_nb_labels(Program *prog)
{
    return labels_map_size(&prog->labels);
}

char* 
read_file(const char* file_name) {
    FILE *file_ptr = fopen(file_name, "r");
    //TODO: verify that the file was correctly opened.

    // Compute the length of the file
    fseek(file_ptr, 0, SEEK_END);
    long fsize = ftell(file_ptr);
    fseek(file_ptr, 0, SEEK_SET);

    // Read the whole content
    char *content = malloc(fsize + 1);
    fread(content, fsize, 1, file_ptr);
    //TODO: fread might have read less than fsize I think?
    fclose(file_ptr);

    //Make sure the string is null terminated.
    content[fsize] = 0;
    return content;
}

Program
parse_program_from_file(const char* file_name) {
    char* content = read_file(file_name);
    //TODO memory leak here content is never freed (but it has to remain alive because the tokens in the resulting program depend on it).
    return parse_program_from_string(content);
}

Program
parse_program_from_string(const char* content) {
    const char* current_position = content;

    Program result;
    program_init(&result);

    while (*current_position != '\0') {
        read_program_line(&result, &current_position);
    }
    return result;
}

bool 
has_line_ended(const char** input_ptr) {
    return **input_ptr == '\0' || **input_ptr == '\n';
}

void
skip_whitespace(const char** input_ptr) {
    while (!has_line_ended(input_ptr) && isspace(**input_ptr)) {
        (*input_ptr)++;
    }
}

Token read_identifier(const char **input_ptr)
{
    skip_whitespace(input_ptr);
    if (has_line_ended(input_ptr)) {
        return NULL_TOKEN;
    }
    const char* result_begin = *input_ptr;

    while (isalnum(**input_ptr) || **input_ptr == '_') {
        (*input_ptr)++;
    }
    Token result; 
    result.begin = result_begin;
    result.end = *input_ptr;
    return result;
}

void skip_line(const char **input_ptr) {
    while(!has_line_ended(input_ptr)) {
        (*input_ptr)++;
    }
    if (**input_ptr == '\n') {
        (*input_ptr)++;
    }
}

void read_program_line(Program *prog, const char **input_ptr)
{
    skip_whitespace(input_ptr);
    if (has_line_ended(input_ptr))
    {
        //Empty line
        return;
    }
    if (**input_ptr == ';') {
        //Starting a comment, skip line.
        skip_line(input_ptr);
    }

    Token tok = read_identifier(input_ptr);
    skip_whitespace(input_ptr); 
    if (**input_ptr == ':') {
        // We are reading a label
        labels_map_insert(&prog->labels, tok, program_nb_instructions(prog));

        //TODO: an error should be reported if the line contains anything that is not a comment.
        skip_line(input_ptr);
        return;
    }

    InstructionLine inst; 
    instruction_line_init(&inst);
    inst.name = tok;
    int arg_index = 0;
    while (arg_index < MAX_INSTRUCTION_ARGS && !has_line_ended(input_ptr) && **input_ptr != ';')
    {
        inst.arguments[arg_index] = read_identifier(input_ptr);
        arg_index++;
    }
    //TODO handle the case where arg_index = MAX_INSTRUCTION_ARGS
    instruction_lines_vect_push(&prog->instructions, inst);
    skip_line(input_ptr);
}

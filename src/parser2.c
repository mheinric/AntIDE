#include "parser2.h"

void parse2_result_init(Parse2Result *parse_result)
{
    program2_init(&parse_result->program);
    vector_parse_error_init(&parse_result->errors);
}

void parse2_result_clear(Parse2Result *parse_result)
{
    program2_clear(&parse_result->program);
    vector_parse_error_clear(&parse_result->errors);
}

typedef struct {
    const char* begin; 
    const char* end;
} Token2;

static const Token2 NULL_TOKEN = {
    .begin = NULL,
    .end = NULL
};

bool token2_is_null(const Token2 *token)
{
    return token->begin == NULL && token->end == NULL;
}

bool token2_matches_str(const Token2 *token, const char *str)
{
    const size_t size = strlen(str);
    if (token2_is_null(token) || size != (size_t) (token->end - token->begin))
    {
        return false;
    }
    for (size_t i = 0; i < size; i++) {
        if (token->begin[i] < 0 || tolower(token->begin[i]) != tolower(str[i]))
        {
            return false;
        }
    }
    return true;
}

bool 
parser2_has_line_ended(const char** input_ptr) {
    return **input_ptr == '\0' || **input_ptr == '\n';
}

void
parser2_skip_whitespace(const char** input_ptr) {
    while (!parser2_has_line_ended(input_ptr) && isspace(**input_ptr)) {
        (*input_ptr)++;
    }
}

Token2 parser2_read_identifier(const char **input_ptr)
{
    parser2_skip_whitespace(input_ptr);
    if (parser2_has_line_ended(input_ptr)) {
        return NULL_TOKEN;
    }
    const char* result_begin = *input_ptr;

    while (isalnum(**input_ptr) || **input_ptr == '_') {
        (*input_ptr)++;
    }
    Token2 result; 
    result.begin = result_begin;
    result.end = *input_ptr;
    return result;
}

Parse2Result 
parse2_program_from_string(const char *content)
{
    Parse2Result result;
    parse2_result_init(&result);

    //First pass, read from the string input
    const char* current_position = content;
    Token2 token = NULL_TOKEN;
    if (*current_position != '\0') 
    {
        token = parser2_read_identifier(&current_position);
    }

    if (*current_position != '\0' && !isspace(*current_position))
    {
        ParseError err; 
        err.line = 1; 
        err.message = "Unexpected character";
        vector_parse_error_push(&result.errors, err);
        return result;
    }

    //Second pass, convert what was read into instructions
    if (!token2_is_null(&token)) 
    {
        if (token2_matches_str(&token, "PICKUP")) 
        {
            Instruction inst;
            inst.type = INST_PICKUP; 
            program2_push_instruction(&result.program, inst);
        }
        else if (token2_matches_str(&token, "DROP"))
        {
            Instruction inst;
            inst.type = INST_DROP; 
            program2_push_instruction(&result.program, inst);
        }
        else 
        {
            ParseError err;
            err.line = 1; 
            err.message = "Unknown instruction name";
            vector_parse_error_push(&result.errors, err);
        }
    }
    return result;
}

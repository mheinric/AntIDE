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

enum {
    INSTRUCTION_PARSE_MATCH, 
    INSTRUCTION_PARSE_NOT_MATCH, 
    INSTRUCTION_PARSE_TOO_MANY_ARGS,
    INSTRUCTION_PARSE_TOO_FEW_ARGS,
};

bool 
parser2_verify_nb_arguments(unsigned expected, unsigned actual, VectorParseError *errors)
{
    if (expected == actual)
    {
        return true;
    }
    ParseError err; 
    err.line = 1; 
    err.message = expected > actual ? "Missing argument(s)" : "Too many arguments";
    vector_parse_error_push(errors, err);
    return false;
}

bool parser2_read_argument(const Token2 *token, Argument *target, VectorParseError *errors)
{
    if (token2_matches_str(token, "r0"))
    {
        *target = argument_create_register(0);
    }
    else if (token2_matches_str(token, "r1"))
    {
        *target = argument_create_register(1);
    }
    else 
    {
        ParseError err;
        err.line = 1; 
        err.message = "Invalid argument";
        vector_parse_error_push(errors, err);
        return false;
    }
    return true;
}

void
read_instruction_from_tokens(
    Token2* tokens, 
    unsigned nb_token, 
    Program2 *program, 
    VectorParseError *errors) 
{
    if (token2_matches_str(&tokens[0], "PICKUP")) 
    {
        if (!parser2_verify_nb_arguments(1, nb_token, errors))
        {
            return;
        }
        Instruction inst;
        inst.type = INST_PICKUP; 
        program2_push_instruction(program, inst);
    }
    else if (token2_matches_str(&tokens[0], "DROP"))
    {
        if (!parser2_verify_nb_arguments(1, nb_token, errors))
        {
            return;
        }
        Instruction inst;
        inst.type = INST_DROP; 
        program2_push_instruction(program, inst);
    }
    else if (token2_matches_str(&tokens[0], "MOVE"))
    {
        if (!parser2_verify_nb_arguments(2, nb_token, errors))
        {
            return;
        }
        Instruction inst;
        inst.type = INST_MOVE;
        if (parser2_read_argument(&tokens[1], &inst.move_args.dir, errors))
        {
            program2_push_instruction(program, inst);
        }
    }
    else 
    {
        ParseError err;
        err.line = 1; 
        err.message = "Unknown instruction name";
        vector_parse_error_push(errors, err);
    }
}

Parse2Result 
parse2_program_from_string(const char *content)
{
    Parse2Result result;
    parse2_result_init(&result);

    //First pass, read from the string input
    const char* current_position = content;
    enum { MAX_NB_TOKENS = 5 };

    Token2 tokens[MAX_NB_TOKENS] = { NULL_TOKEN, NULL_TOKEN, NULL_TOKEN, NULL_TOKEN, NULL_TOKEN };
    int nb_tokens = 0;
    while (*current_position != '\0')
    {
        tokens[nb_tokens] = parser2_read_identifier(&current_position);
        nb_tokens+= 1;

        if (*current_position != '\0' && !isspace(*current_position))
        {
            ParseError err; 
            err.line = 1; 
            err.message = "Unexpected character";
            vector_parse_error_push(&result.errors, err);
            return result;
        }
    }

    //Second pass, convert what was read into instructions
    if (nb_tokens > 0) 
    {
        read_instruction_from_tokens(tokens, nb_tokens, &result.program, &result.errors);
    }
    return result;
}

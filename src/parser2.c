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

Token2 parser2_read_token(const char **input_ptr)
{
    parser2_skip_whitespace(input_ptr);
    if (parser2_has_line_ended(input_ptr)) {
        return NULL_TOKEN;
    }
    const char* result_begin = *input_ptr;

    while (isalnum(**input_ptr) || **input_ptr == '_' || **input_ptr == '-') {
        (*input_ptr)++;
    }
    Token2 result; 
    result.begin = result_begin;
    result.end = *input_ptr;
    return result;
}

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

const struct { const char* key; Argument value; } parser2_builtin_constants[] = {
    { .key = "r0",    .value = { .is_register = true, .register_index = 0 } },
    { .key = "r1",    .value = { .is_register = true, .register_index = 1 } },
    { .key = "r2",    .value = { .is_register = true, .register_index = 2 } },
    { .key = "r3",    .value = { .is_register = true, .register_index = 3 } },
    { .key = "r4",    .value = { .is_register = true, .register_index = 4 } },
    { .key = "r5",    .value = { .is_register = true, .register_index = 5 } },
    { .key = "r6",    .value = { .is_register = true, .register_index = 6 } },
    { .key = "r7",    .value = { .is_register = true, .register_index = 7 } },
    { .key = "HERE",  .value = { .is_register = false, .value = 0 } },
    { .key = "NORTH", .value = { .is_register = false, .value = 1 } },
    { .key = "EAST",  .value = { .is_register = false, .value = 2 } },
    { .key = "SOUTH", .value = { .is_register = false, .value = 3 } },
    { .key = "WEST",  .value = { .is_register = false, .value = 4 } },
};

bool 
parser2_read_argument(const Token2 *token, Argument *target, VectorParseError *errors)
{
    if (isdigit(token->begin[0]) || token->begin[0] == '-')
    {
        //We are reading a number
        bool is_negative = token->begin[0] == '-';
        int32_t value = 0;
        for (const char* it = token->begin + is_negative; it != token->end; it++)
        {
            if (!isdigit(*it))
            {
                ParseError err; 
                err.line = 1; 
                err.message = "Invalid number format";
                vector_parse_error_push(errors, err);
                return false;
            }
            value = 10 * value + (*it - '0');
        }
        *target = argument_create_value(is_negative ? -value : value);
        return true;
    }
    else
    {
        const size_t nb_constants = sizeof(parser2_builtin_constants) / sizeof(parser2_builtin_constants[0]);
        for (size_t i = 0; i < nb_constants; i++)
        {
            if (token2_matches_str(token, parser2_builtin_constants[i].key))
            {
                *target = parser2_builtin_constants[i].value;
                return true;
            }
        }
        ParseError err;
        err.line = 1; 
        err.message = "Invalid argument";
        vector_parse_error_push(errors, err);
        return false;
    }
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

bool 
parse2_read_tokens_from_line(const char **current_position, unsigned buffer_size, Token2 *buffer, unsigned *nb_tokens, VectorParseError *errors)
{
    *nb_tokens = 0;
    while (**current_position != '\0' && *nb_tokens < buffer_size)
    {
        buffer[*nb_tokens] = parser2_read_token(current_position);
        *nb_tokens+= 1;

        if (**current_position != '\0' && **current_position != ';' && !isspace(**current_position))
        {
            ParseError err; 
            err.line = 1; 
            err.message = "Unexpected character";
            vector_parse_error_push(errors, err);
            return false;
        }
        parser2_skip_whitespace(current_position);
        if (**current_position == ';')
        {
            //The rest of the line can be ignored.
            break;
        }
    }
    return true;
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
    unsigned nb_tokens = 0;
    parse2_read_tokens_from_line(&current_position, MAX_NB_TOKENS, tokens, &nb_tokens, &result.errors);

    //Second pass, convert what was read into instructions
    if (nb_tokens > 0) 
    {
        read_instruction_from_tokens(tokens, nb_tokens, &result.program, &result.errors);
    }
    return result;
}

char* 
parse2_read_file(const char* file_name) {
    FILE *file_ptr = fopen(file_name, "r");
    if (file_ptr == NULL)
    {
        return NULL;
    }

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


Parse2Result parse2_program_from_file(const char *file_path)
{
    char* content = parse2_read_file(file_path);
    if (content == NULL)
    {
        Parse2Result result;
        parse2_result_init(&result);
        ParseError error; 
        error.line = 0; 
        error.message = "Failed to open file";
        vector_parse_error_push(&result.errors, error); 
        return result;
    }
    Parse2Result result = parse2_program_from_string(content);
    free(content);
    return result;
}

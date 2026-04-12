#include "parser.h"

void 
parse_result_init(ParseResult *parse_result)
{
    program_init(&parse_result->program);
    vector_parse_error_init(&parse_result->errors);
}

void 
parse_result_clear(ParseResult *parse_result)
{
    program_clear(&parse_result->program);
    vector_parse_error_clear(&parse_result->errors);
}

void 
parse_result_print_errors(const ParseResult *parse_result)
{
    if (vector_parse_error_size(&parse_result->errors) == 0)
    {
        printf("No errors\n"); 
        return;
    }
    for (const ParseError* it = parse_result->errors.begin; it != parse_result->errors.end; it++)
    {
        if (it->line > 0) 
        {
            printf("l.%zu: %s\n", it->line, it->message);
        }
        else
        {
            printf("%s\n", it->message);
        }
    }

}

typedef struct {
    const char* begin; 
    const char* end;
} Token;

static const Token NULL_TOKEN = {
    .begin = NULL,
    .end = NULL
};

bool token_is_null(const Token *token)
{
    return token->begin == NULL && token->end == NULL;
}

bool token_matches_str(const Token *token, const char *str)
{
    const size_t size = strlen(str);
    if (token_is_null(token) || size != (size_t) (token->end - token->begin))
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
parser_has_line_ended(const char** input_ptr) {
    return **input_ptr == '\0' || **input_ptr == '\n';
}

void
parser_skip_whitespace(const char** input_ptr) {
    while (!parser_has_line_ended(input_ptr) && isspace(**input_ptr)) {
        (*input_ptr)++;
    }
}

void
parser_skip_line(const char** input_ptr) {
    while (!parser_has_line_ended(input_ptr))
    {
        (*input_ptr)++;
    }
    if (**input_ptr == '\n')
    {
        (*input_ptr)++;
    }
}

Token parser_read_token(const char **input_ptr)
{
    parser_skip_whitespace(input_ptr);
    if (parser_has_line_ended(input_ptr)) {
        return NULL_TOKEN;
    }
    const char* result_begin = *input_ptr;

    while (isalnum(**input_ptr) || **input_ptr == '_' || **input_ptr == '-') {
        (*input_ptr)++;
    }
    Token result; 
    result.begin = result_begin;
    result.end = *input_ptr;
    return result;
}

bool 
parser_verify_nb_arguments(unsigned expected, unsigned actual, VectorParseError *errors)
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

const struct { const char* key; Argument value; } parser_builtin_constants[] = {
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

bool parser_get_digit_value(char c, int32_t *digit_value, int32_t base)
{
    if (isdigit(c))
    {
        *digit_value = c - '0'; 
        return *digit_value < base;
    }
    else 
    {
        *digit_value = tolower(c) - 'a' + 10;
        return *digit_value > 0 && *digit_value < base;
    }
}

bool 
parser_read_integer(const Token* token, int32_t *target, VectorParseError *errors)
{
    //We are reading a number
    const char* it = token->begin; 
    bool is_negative = false;
    if (*it == '-') {
        is_negative = true; 
        it++;
        if (it == token->end)
        {
            ParseError err; 
            err.line = 1; 
            err.message = "Syntax error";
            vector_parse_error_push(errors, err);
            return false;
        }
    }

    int32_t base = 10;
    if (*it == '0' && it+1 != token->end && !isdigit(*(it + 1)))
    {
        if (tolower(*(it+1)) == 'b')
        {
            base = 2;
        }
        else if (tolower(*(it+1)) == 'x')
        {
            base = 16;
        }
        else
        {
            ParseError err; 
            err.line = 1; 
            err.message = "Syntax error";
            vector_parse_error_push(errors, err);
            return false;
        }
        it += 2;
    }

    *target = 0;
    for (; it != token->end; it++)
    {
        int32_t digit_value = 0; 
        if (!parser_get_digit_value(*it, &digit_value, base))
        {
            ParseError err; 
            err.line = 1; 
            err.message = "Invalid number format";
            vector_parse_error_push(errors, err);
            return false;
        }
        *target = base * (*target) + digit_value;
    }
    if (is_negative)
    {
        *target *= -1;
    }
    return true;
}

bool 
parser_read_argument(const Token *token, Argument *target, VectorParseError *errors)
{
    if (isdigit(token->begin[0]) || token->begin[0] == '-')
    {
        int32_t value = 0;
        if (!parser_read_integer(token, &value, errors))
        {
            return false;
        }

        *target = argument_create_value(value);
        return true;
    }
    else
    {
        const size_t nb_constants = sizeof(parser_builtin_constants) / sizeof(parser_builtin_constants[0]);
        for (size_t i = 0; i < nb_constants; i++)
        {
            if (token_matches_str(token, parser_builtin_constants[i].key))
            {
                *target = parser_builtin_constants[i].value;
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

bool
parser_read_register(const Token *token, uint8_t *target_reg, VectorParseError *errors)
{
    Argument arg; 
    if (!parser_read_argument(token, &arg, errors))
    {
        return false;
    }
    if (!arg.is_register)
    {
        ParseError err;
        err.line = 1; 
        err.message = "Expected a register argument but got a number";
        vector_parse_error_push(errors, err);
        return false;
    }
    *target_reg = arg.register_index;
    return true;
}

void 
parser_read_artihmetic_instruction(
    InstructionType type, 
    Token *tokens, 
    unsigned nb_token, 
    Program *program, 
    VectorParseError *errors)
{
    if (!parser_verify_nb_arguments(3, nb_token, errors))
    {
        return;
    }
    Instruction inst;
    inst.type = type;
    if (parser_read_register(&tokens[1], &inst.arith_args.target_register, errors) && 
        parser_read_argument(&tokens[2], &inst.arith_args.arg, errors))
    {
        program_push_instruction(program, inst);
    }
}

void
read_instruction_from_tokens(
    Token* tokens, 
    unsigned nb_token, 
    Program *program, 
    VectorParseError *errors) 
{
    if (token_matches_str(&tokens[0], "PICKUP")) 
    {
        if (!parser_verify_nb_arguments(1, nb_token, errors))
        {
            return;
        }
        Instruction inst;
        inst.type = INST_PICKUP; 
        program_push_instruction(program, inst);
    }
    else if (token_matches_str(&tokens[0], "DROP"))
    {
        if (!parser_verify_nb_arguments(1, nb_token, errors))
        {
            return;
        }
        Instruction inst;
        inst.type = INST_DROP; 
        program_push_instruction(program, inst);
    }
    else if (token_matches_str(&tokens[0], "MOVE"))
    {
        if (!parser_verify_nb_arguments(2, nb_token, errors))
        {
            return;
        }
        Instruction inst;
        inst.type = INST_MOVE;
        if (parser_read_argument(&tokens[1], &inst.move_args.dir, errors))
        {
            program_push_instruction(program, inst);
        }
    }
    else if (token_matches_str(&tokens[0], "SET"))
    {
        parser_read_artihmetic_instruction(INST_SET, tokens, nb_token, program, errors);
    }
    else if (token_matches_str(&tokens[0], "ADD"))
    {
        parser_read_artihmetic_instruction(INST_ADD, tokens, nb_token, program, errors);
    }
    else if (token_matches_str(&tokens[0], "SUB"))
    {
        parser_read_artihmetic_instruction(INST_SUB, tokens, nb_token, program, errors);
    }
    else if (token_matches_str(&tokens[0], "MOD"))
    {
        parser_read_artihmetic_instruction(INST_MOD, tokens, nb_token, program, errors);
    }
    else if (token_matches_str(&tokens[0], "MUL"))
    {
        parser_read_artihmetic_instruction(INST_MUL, tokens, nb_token, program, errors);
    }
    else if (token_matches_str(&tokens[0], "DIV"))
    {
        parser_read_artihmetic_instruction(INST_DIV, tokens, nb_token, program, errors);
    }
    else if (token_matches_str(&tokens[0], "AND"))
    {
        parser_read_artihmetic_instruction(INST_AND, tokens, nb_token, program, errors);
    }
    else if (token_matches_str(&tokens[0], "OR"))
    {
        parser_read_artihmetic_instruction(INST_OR, tokens, nb_token, program, errors);
    }
    else if (token_matches_str(&tokens[0], "XOR"))
    {
        parser_read_artihmetic_instruction(INST_XOR, tokens, nb_token, program, errors);
    }
    else if (token_matches_str(&tokens[0], "LSHIFT"))
    {
        parser_read_artihmetic_instruction(INST_LSHIFT, tokens, nb_token, program, errors);
    }
    else if (token_matches_str(&tokens[0], "RSHIFT"))
    {
        parser_read_artihmetic_instruction(INST_RSHIFT, tokens, nb_token, program, errors);
    }
    else if (token_matches_str(&tokens[0], "RANDOM"))
    {
        parser_read_artihmetic_instruction(INST_RANDOM, tokens, nb_token, program, errors);
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
parse_read_tokens_from_line(const char **current_position, unsigned buffer_size, Token *buffer, unsigned *nb_tokens, VectorParseError *errors)
{
    *nb_tokens = 0;
    //TODO: something to be done if the line contains only comments
    while (!parser_has_line_ended(current_position) && *nb_tokens < buffer_size)
    {
        buffer[*nb_tokens] = parser_read_token(current_position);
        *nb_tokens+= 1;

        if (**current_position != '\0' && **current_position != ';' && !isspace(**current_position))
        {
            ParseError err; 
            err.line = 1; 
            err.message = "Unexpected character";
            vector_parse_error_push(errors, err);
            parser_skip_line(current_position);
            return false;
        }
        parser_skip_whitespace(current_position);
        if (**current_position == ';')
        {
            //The rest of the line can be ignored.
            break;
        }
    }
    //Skip the remaining of the line, which is either just the newline or comments.
    parser_skip_line(current_position);
    return true;
}

ParseResult 
parse_program_from_string(const char *content)
{
    ParseResult result;
    parse_result_init(&result);

    //First pass, read from the string input
    const char* current_position = content;
    enum { MAX_NB_TOKENS = 5 };

    while (*current_position != '\0')
    {
        Token tokens[MAX_NB_TOKENS] = { NULL_TOKEN, NULL_TOKEN, NULL_TOKEN, NULL_TOKEN, NULL_TOKEN };
        unsigned nb_tokens = 0;
        if (!parse_read_tokens_from_line(&current_position, MAX_NB_TOKENS, tokens, &nb_tokens, &result.errors))
        {
            //An error was encountered in the first pass of parsing this line, we skip it.
            continue; 
        }

        //Second pass, convert what was read into instructions
        if (nb_tokens > 0) 
        {
            read_instruction_from_tokens(tokens, nb_tokens, &result.program, &result.errors);
        }
    }
    return result;
}

char* 
parse_read_file(const char* file_name) {
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


ParseResult parse_program_from_file(const char *file_path)
{
    char* content = parse_read_file(file_path);
    if (content == NULL)
    {
        ParseResult result;
        parse_result_init(&result);
        ParseError error; 
        error.line = 0; 
        error.message = "Failed to open file";
        vector_parse_error_push(&result.errors, error); 
        return result;
    }
    ParseResult result = parse_program_from_string(content);
    free(content);
    return result;
}

#include "parser.h"

void 
parse_result_init(ParseResult *parse_result)
{
    program_init(&parse_result->program);
    vector_parse_error_init(&parse_result->errors);
}

void parse_result_init_move(ParseResult *dest, ParseResult *source)
{
    program_init_move(&dest->program, &source->program);
    vector_parse_error_init_move(&dest->errors, &source->errors);
}

void 
parse_result_cleanup(ParseResult *parse_result)
{
    program_cleanup(&parse_result->program);
    vector_parse_error_cleanup(&parse_result->errors);
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

typedef struct {
    const char* content;
    const char* current_position;
    size_t current_line;
    ParseResult parse_result;
} Parser;

void 
parser_init(Parser* parser, const char* content)
{
    parser->content = content; 
    parser->current_position = content; 
    parser->current_line = 1;
    parse_result_init(&parser->parse_result);
}

void 
parser_cleanup(Parser* parser)
{
    parser->content = NULL; 
    parser->current_position = NULL; 
    parser->current_line = 0; 
    parse_result_cleanup(&parser->parse_result);
}

void
parser_advance(Parser *parser)
{
    parser->current_position++;
}

void 
parser_push_error(Parser *parser, const char* message)
{
    ParseError error; 
    error.line = parser->current_line;
    error.message = message; 
    vector_parse_error_push(&parser->parse_result.errors, error);
}

bool 
parser_has_line_ended(Parser *parser) {
    return *parser->current_position == '\0' || *parser->current_position == '\n';
}

void
parser_skip_whitespace(Parser *parser) {
    while (!parser_has_line_ended(parser) && isspace(*parser->current_position)) {
        parser_advance(parser);
    }
}

void
parser_skip_line(Parser *parser) {
    while (!parser_has_line_ended(parser))
    {
        parser_advance(parser);
    }
    if (*parser->current_position == '\n')
    {
        parser_advance(parser);
        parser->current_line++;
    }
}

Token parser_read_token(Parser *parser)
{
    parser_skip_whitespace(parser);
    if (parser_has_line_ended(parser)) {
        return NULL_TOKEN;
    }
    const char* result_begin = parser->current_position;

    while (isalnum(*parser->current_position) || *parser->current_position == '_' || *parser->current_position == '-') {
        parser_advance(parser);
    }
    if (parser->current_position == result_begin)
    {
        return NULL_TOKEN;
    }
    Token result; 
    result.begin = result_begin;
    result.end = parser->current_position;
    return result;
}

bool 
parser_verify_nb_arguments(Parser *parser, unsigned expected, unsigned actual)
{
    if (expected == actual)
    {
        return true;
    }
    parser_push_error(parser, expected > actual ? "Missing argument(s)" : "Too many arguments");
    return false;
}

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
parser_read_integer(Parser* parser, const Token* token, int32_t *target)
{
    //We are reading a number
    const char* it = token->begin; 
    bool is_negative = false;
    if (*it == '-') {
        is_negative = true; 
        it++;
        if (it == token->end)
        {
            parser_push_error(parser, "Syntax error");
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
            parser_push_error(parser, "Syntax error");
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
            parser_push_error(parser, "Invalid number format");
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

bool 
parser_read_argument(Parser* parser, const Token *token, Argument *target)
{
    if (isdigit(token->begin[0]) || token->begin[0] == '-')
    {
        int32_t value = 0;
        if (!parser_read_integer(parser, token, &value))
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
        parser_push_error(parser, "Invalid argument");
        return false;
    }
}

bool
parser_read_register(Parser *parser, const Token *token, uint8_t *target_reg)
{
    Argument arg; 
    if (!parser_read_argument(parser, token, &arg))
    {
        return false;
    }
    if (!arg.is_register)
    {
        parser_push_error(parser, "Expected a register argument but got a number");
        return false;
    }
    *target_reg = arg.register_index;
    return true;
}

void 
parser_read_artihmetic_instruction(
    Parser *parser,
    InstructionType type, 
    Token *tokens, 
    unsigned nb_token)
{
    if (!parser_verify_nb_arguments(parser, 3, nb_token))
    {
        return;
    }
    Instruction inst;
    inst.type = type;
    if (parser_read_register(parser, &tokens[1], &inst.arith_args.target_register) && 
        parser_read_argument(parser, &tokens[2], &inst.arith_args.arg))
    {
        program_push_instruction(&parser->parse_result.program, inst);
    }
}

void
read_instruction_from_tokens(
    Parser* parser,
    Token* tokens, 
    unsigned nb_token)
{
    if (token_matches_str(&tokens[0], "PICKUP")) 
    {
        if (!parser_verify_nb_arguments(parser, 1, nb_token))
        {
            return;
        }
        Instruction inst;
        inst.type = INST_PICKUP; 
        program_push_instruction(&parser->parse_result.program, inst);
    }
    else if (token_matches_str(&tokens[0], "DROP"))
    {
        if (!parser_verify_nb_arguments(parser, 1, nb_token))
        {
            return;
        }
        Instruction inst;
        inst.type = INST_DROP; 
        program_push_instruction(&parser->parse_result.program, inst);
    }
    else if (token_matches_str(&tokens[0], "MOVE"))
    {
        if (!parser_verify_nb_arguments(parser, 2, nb_token))
        {
            return;
        }
        Instruction inst;
        inst.type = INST_MOVE;
        if (parser_read_argument(parser, &tokens[1], &inst.move_args.dir))
        {
            program_push_instruction(&parser->parse_result.program, inst);
        }
    }
    else if (token_matches_str(&tokens[0], "SET"))
    {
        parser_read_artihmetic_instruction(parser, INST_SET, tokens, nb_token);
    }
    else if (token_matches_str(&tokens[0], "ADD"))
    {
        parser_read_artihmetic_instruction(parser, INST_ADD, tokens, nb_token);
    }
    else if (token_matches_str(&tokens[0], "SUB"))
    {
        parser_read_artihmetic_instruction(parser, INST_SUB, tokens, nb_token);
    }
    else if (token_matches_str(&tokens[0], "MOD"))
    {
        parser_read_artihmetic_instruction(parser, INST_MOD, tokens, nb_token);
    }
    else if (token_matches_str(&tokens[0], "MUL"))
    {
        parser_read_artihmetic_instruction(parser, INST_MUL, tokens, nb_token);
    }
    else if (token_matches_str(&tokens[0], "DIV"))
    {
        parser_read_artihmetic_instruction(parser, INST_DIV, tokens, nb_token);
    }
    else if (token_matches_str(&tokens[0], "AND"))
    {
        parser_read_artihmetic_instruction(parser, INST_AND, tokens, nb_token);
    }
    else if (token_matches_str(&tokens[0], "OR"))
    {
        parser_read_artihmetic_instruction(parser, INST_OR, tokens, nb_token);
    }
    else if (token_matches_str(&tokens[0], "XOR"))
    {
        parser_read_artihmetic_instruction(parser, INST_XOR, tokens, nb_token);
    }
    else if (token_matches_str(&tokens[0], "LSHIFT"))
    {
        parser_read_artihmetic_instruction(parser, INST_LSHIFT, tokens, nb_token);
    }
    else if (token_matches_str(&tokens[0], "RSHIFT"))
    {
        parser_read_artihmetic_instruction(parser, INST_RSHIFT, tokens, nb_token);
    }
    else if (token_matches_str(&tokens[0], "RANDOM"))
    {
        parser_read_artihmetic_instruction(parser, INST_RANDOM, tokens, nb_token);
    }
    else 
    {
        parser_push_error(parser, "Unknown instruction name");
    }
}

bool 
parse_read_tokens_from_line(Parser *parser, unsigned buffer_size, Token *buffer, unsigned *nb_tokens)
{
    *nb_tokens = 0;
    //TODO: something to be done if the line contains only comments
    while (!parser_has_line_ended(parser) && *nb_tokens < buffer_size)
    {
        buffer[*nb_tokens] = parser_read_token(parser);
        *nb_tokens+= 1;

        if (*parser->current_position != '\0' && *parser->current_position != ';' && !isspace(*parser->current_position))
        {
            parser_push_error(parser, "Unexpected character");
            //Skip the rest of this line
            parser_skip_line(parser);
            return false;
        }
        parser_skip_whitespace(parser);
        if (*parser->current_position == ';')
        {
            //The rest of the line can be ignored.
            break;
        }
    }
    //Skip the remaining of the line, which is either just the newline or comments.
    parser_skip_line(parser);
    return true;
}

ParseResult 
parse_program_from_string(const char *content)
{
    Parser parser; 
    parser_init(&parser, content); 

    //First pass, read from the string input
    enum { MAX_NB_TOKENS = 5 };

    while (*parser.current_position != '\0')
    {
        Token tokens[MAX_NB_TOKENS] = { NULL_TOKEN, NULL_TOKEN, NULL_TOKEN, NULL_TOKEN, NULL_TOKEN };
        unsigned nb_tokens = 0;
        if (!parse_read_tokens_from_line(&parser, MAX_NB_TOKENS, tokens, &nb_tokens))
        {
            //An error was encountered in the first pass of parsing this line, we skip it.
            continue; 
        }

        //Second pass, convert what was read into instructions
        if (nb_tokens > 0) 
        {
            read_instruction_from_tokens(&parser, tokens, nb_tokens);
        }
    }

    ParseResult result; 
    parse_result_init_move(&result, &parser.parse_result); 
    parser_cleanup(&parser);
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

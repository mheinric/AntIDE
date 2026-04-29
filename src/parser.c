#include "parser_private.h"
#include "parser.h"

void 
parse_result_init(ParseResult *parse_result)
{
    program_init(&parse_result->program);
    parse_result->errors = calloc(1, sizeof(VectorParseError));
    vector_parse_error_init(parse_result->errors);
}

void 
parse_result_init_move(ParseResult *dest, ParseResult *source)
{
    program_init_move(&dest->program, &source->program);
    dest->errors = calloc(1, sizeof(VectorParseError));
    vector_parse_error_init_move(dest->errors, source->errors);
}

void 
parse_result_cleanup(ParseResult *parse_result)
{
    program_cleanup(&parse_result->program);
    vector_parse_error_cleanup(parse_result->errors);
    free(parse_result->errors);
}

void 
parse_result_print_errors(const ParseResult *parse_result)
{
    if (vector_parse_error_size(parse_result->errors) == 0)
    {
        printf("No errors\n"); 
        return;
    }
    for (const ParseError* it = parse_result->errors->begin; it != parse_result->errors->end; it++)
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

size_t 
parse_result_nb_errors(const ParseResult *parse_result)
{
    return vector_parse_error_size(parse_result->errors);
}

ParseError 
parse_result_get_error(const ParseResult *parse_result, size_t index)
{
    return parse_result->errors->begin[index];
}

void 
token_init(Token *token, const char *begin, const char *end)
{
    token->begin = begin; 
    token->end = end;
}

/**
 * Initialises the token from a null-terminated string.
 */
void 
token_init_from_string(Token* token, const char* content)
{
    token->begin = content; 
    token->end = token->begin + strlen(content);
}

bool 
token_is_null(const Token *token)
{
    return token->begin == NULL && token->end == NULL;
}

bool
token_equal(const Token *first, const Token* second)
{
    if (token_is_null(first) != token_is_null(second))
    {
        return false;
    }
    const size_t size_first = first->end - first->begin;
    const size_t size_second = second->end - second->begin; 
    if (size_first != size_second)
    {
        return false;
    }
    for (size_t i = 0; i < size_first; i++)
    {
        if (tolower(first->begin[i]) != tolower(second->begin[i]))
        {
            return false;
        }
    }
    return true;
}

bool 
token_matches_str(const Token *token, const char *str)
{
    Token second; 
    token_init_from_string(&second, str); 
    return token_equal(token, &second);
}

void
token_line_init(TokenLine* token_line, size_t line_number)
{

    for (int i = 0; i < PARSER_MAX_TOKE_PER_LINE; i++)
    {
        token_line->tokens[i] = NULL_TOKEN;
    }
    token_line->nb_tokens = 0; 
    token_line->line_number = line_number;
}

const struct { const char* key; Argument value; } parser_builtin_constants[] = {
    { .key = "r0",        .value = { .is_register = true, .register_index = 0 } },
    { .key = "r1",        .value = { .is_register = true, .register_index = 1 } },
    { .key = "r2",        .value = { .is_register = true, .register_index = 2 } },
    { .key = "r3",        .value = { .is_register = true, .register_index = 3 } },
    { .key = "r4",        .value = { .is_register = true, .register_index = 4 } },
    { .key = "r5",        .value = { .is_register = true, .register_index = 5 } },
    { .key = "r6",        .value = { .is_register = true, .register_index = 6 } },
    { .key = "r7",        .value = { .is_register = true, .register_index = 7 } },
    { .key = "HERE",      .value = { .is_register = false, .value = DIR_HERE } },
    { .key = "NORTH",     .value = { .is_register = false, .value = DIR_NORTH } },
    { .key = "EAST",      .value = { .is_register = false, .value = DIR_EAST } },
    { .key = "SOUTH",     .value = { .is_register = false, .value = DIR_SOUTH } },
    { .key = "WEST",      .value = { .is_register = false, .value = DIR_WEST } },
    { .key = "CH_RED",    .value = { .is_register = false, .value = CH_RED } },
    { .key = "CH_BLUE",   .value = { .is_register = false, .value = CH_BLUE } },
    { .key = "CH_GREEN",  .value = { .is_register = false, .value = CH_GREEN } },
    { .key = "CH_YELLOW", .value = { .is_register = false, .value = CH_YELLOW } },
    { .key = "EMPTY",     .value = { .is_register = false, .value = ENT_EMPTY } },
    { .key = "WALL",      .value = { .is_register = false, .value = ENT_WALL } },
    { .key = "FOOD",      .value = { .is_register = false, .value = ENT_FOOD } },
    { .key = "NEST",      .value = { .is_register = false, .value = ENT_NEST } },
    { .key = "ANT",       .value = { .is_register = false, .value = ENT_ANT } },
};

void 
parser_init(Parser* parser, const char* content)
{
    parser->content = content; 
    parser->current_position = content; 
    parser->current_line = 1;
    vector_constant_entry_init(&parser->constants);
    vector_token_line_init(&parser->token_lines);
    const int nb_builtin_constants = sizeof(parser_builtin_constants) / sizeof(parser_builtin_constants[0]); 
    for (int i = 0; i < nb_builtin_constants; i++)
    {
        ConstantEntry entry; 
        token_init_from_string(&entry.name, parser_builtin_constants[i].key);
        entry.value = parser_builtin_constants[i].value;
        vector_constant_entry_push(&parser->constants, entry);
    }
    parse_result_init(&parser->parse_result);
}

void 
parser_cleanup(Parser* parser)
{
    parser->content = NULL; 
    parser->current_position = NULL; 
    parser->current_line = 0; 
    vector_constant_entry_cleanup(&parser->constants);
    vector_token_line_cleanup(&parser->token_lines);
    parse_result_cleanup(&parser->parse_result);
}

void
parser_advance(Parser *parser)
{
    assert(*parser->current_position != '\0');
    parser->current_position++;
}

void 
parser_push_error(Parser *parser, const char* message)
{
    ParseError error; 
    error.line = parser->current_line;
    error.message = message; 
    vector_parse_error_push(parser->parse_result.errors, error);
}

bool 
parser_has_line_ended(Parser *parser) {
    return *parser->current_position == '\0' || *parser->current_position == '\n' || *parser->current_position == ';';
}

void
parser_skip_whitespace(Parser *parser) {
    while (!parser_has_line_ended(parser) && isspace(*parser->current_position)) {
        parser_advance(parser);
    }
}

void
parser_skip_line(Parser *parser) {
    while (!parser_has_line_ended(parser) || *parser->current_position == ';')
    {
        parser_advance(parser);
    }
    if (*parser->current_position == '\n')
    {
        parser_advance(parser);
        parser->current_line++;
    }
}

Token 
parser_read_token(Parser *parser)
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
parser_verify_nb_arguments(Parser *parser, unsigned minimum, unsigned maximum, unsigned actual)
{
    if (actual >= minimum && actual <= maximum)
    {
        return true;
    }
    parser_push_error(parser, minimum > actual ? "Missing argument(s)" : "Too many arguments");
    return false;
}

bool 
parser_get_digit_value(char c, int32_t *digit_value, int32_t base)
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
        for (const ConstantEntry* it = parser->constants.begin; it != parser->constants.end; it++)
        {
            if (token_equal(token, &it->name))
            {
                *target = it->value;
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
    const Token *tokens, 
    unsigned nb_token)
{
    if (!parser_verify_nb_arguments(parser, 3, 3, nb_token))
    {
        return;
    }
    Instruction inst;
    inst.type = type;
    inst.breakpoint = false;
    if (parser_read_register(parser, &tokens[1], &inst.arith_args.target_register) && 
        parser_read_argument(parser, &tokens[2], &inst.arith_args.arg))
    {
        program_push_instruction(&parser->parse_result.program, inst, parser->current_line);
    }
}

void 
parser_read_conditional_jump_instruction(
    Parser *parser, 
    InstructionType type, 
    const Token *tokens, 
    unsigned nb_token)
{
    if (!parser_verify_nb_arguments(parser, 4, 4, nb_token))
    {
        return;
    }
    Argument value1, value2, target; 
    if (!parser_read_argument(parser, &tokens[1], &value1) || 
        !parser_read_argument(parser, &tokens[2], &value2) || 
        !parser_read_argument(parser, &tokens[3], &target))
    {
        return;
    }
    program_push_instruction(&parser->parse_result.program, instruction_create_conditional_jump(type, value1, value2, target), parser->current_line);
}

void
parser_read_info_instruction(Parser *parser, 
    InstructionType type, 
    const Token *tokens, 
    unsigned nb_token)
{
    if (!parser_verify_nb_arguments(parser, 1, 2, nb_token))
    {
        return;
    }
    uint8_t target_register; 
    if (nb_token == 1)
    {
        target_register = 0;
    }
    else if (!parser_read_register(parser, &tokens[1], &target_register))
    {
        return; 
    }
    program_push_instruction(&parser->parse_result.program, instruction_create_info(type, target_register), parser->current_line);
}

void
read_instruction_from_tokens(
    Parser* parser,
    const TokenLine* token_line)
{
    const Token* tokens = token_line->tokens;
    int nb_token = token_line->nb_tokens;
    parser->current_line = token_line->line_number;
    if (token_matches_str(&tokens[0], "PICKUP")) 
    {
        if (!parser_verify_nb_arguments(parser, 1, 1, nb_token))
        {
            return;
        }
        Instruction inst;
        inst.type = INST_PICKUP; 
        inst.breakpoint = false;
        program_push_instruction(&parser->parse_result.program, inst, parser->current_line);
    }
    else if (token_matches_str(&tokens[0], "DROP"))
    {
        if (!parser_verify_nb_arguments(parser, 1, 1, nb_token))
        {
            return;
        }
        Instruction inst;
        inst.type = INST_DROP; 
        inst.breakpoint = false;
        program_push_instruction(&parser->parse_result.program, inst, parser->current_line);
    }
    else if (token_matches_str(&tokens[0], "MOVE"))
    {
        if (!parser_verify_nb_arguments(parser, 2, 2, nb_token))
        {
            return;
        }
        Instruction inst;
        inst.type = INST_MOVE;
        inst.breakpoint = false;
        if (parser_read_argument(parser, &tokens[1], &inst.move_args.dir))
        {
            program_push_instruction(&parser->parse_result.program, inst, parser->current_line);
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
    else if (token_matches_str(&tokens[0], "JMP"))
    {
        if (!parser_verify_nb_arguments(parser, 2, 2, nb_token))
        {
            return;
        }
        Argument target; 
        if (!parser_read_argument(parser, &tokens[1], &target))
        {
            return;
        }
        program_push_instruction(&parser->parse_result.program, instruction_create_jump(target), parser->current_line);
    }
    else if (token_matches_str(&tokens[0], "JEQ"))
    {
        parser_read_conditional_jump_instruction(parser, INST_JEQ, tokens, nb_token);
    }
    else if (token_matches_str(&tokens[0], "JNE"))
    {
        parser_read_conditional_jump_instruction(parser, INST_JNE, tokens, nb_token);
    }
    else if (token_matches_str(&tokens[0], "JGT"))
    {
        parser_read_conditional_jump_instruction(parser, INST_JGT, tokens, nb_token);
    }
    else if (token_matches_str(&tokens[0], "JLT"))
    {
        parser_read_conditional_jump_instruction(parser, INST_JLT, tokens, nb_token);
    }
    else if (token_matches_str(&tokens[0], "CALL"))
    {
        if (!parser_verify_nb_arguments(parser, 3, 3, nb_token))
        {
            return;
        }
        uint8_t return_register; 
        Argument target; 
        if (!parser_read_register(parser, &tokens[1], &return_register) ||
            !parser_read_argument(parser, &tokens[2], &target))
        {
            return;
        }
        program_push_instruction(&parser->parse_result.program, instruction_create_call(return_register, target), parser->current_line);
    }
    else if (token_matches_str(&tokens[0], "ID"))
    {
        parser_read_info_instruction(parser, INST_ID, tokens, nb_token);
    }
    else if (token_matches_str(&tokens[0], "CARRYING"))
    {
        parser_read_info_instruction(parser, INST_CARRY, tokens, nb_token);
    }
    else if (token_matches_str(&tokens[0], "MARK"))
    {
        if (!parser_verify_nb_arguments(parser, 3, 3, nb_token))
        {
            return;
        } 
        Argument channel, amount; 
        if (!parser_read_argument(parser, &tokens[1], &channel) ||
            !parser_read_argument(parser, &tokens[2], &amount))
        {
            return;
        }
        program_push_instruction(&parser->parse_result.program, instruction_create_mark(channel, amount), parser->current_line);
    }
    else if (token_matches_str(&tokens[0], "SNIFF"))
    {
        if (!parser_verify_nb_arguments(parser, 3, 4, nb_token))
        {
            return;
        } 
        Argument channel, direction;
        uint8_t target_register = 0; 
        if (!parser_read_argument(parser, &tokens[1], &channel) ||
            !parser_read_argument(parser, &tokens[2], &direction) || 
            (nb_token == 4 && !parser_read_register(parser, &tokens[3], &target_register)))
        {
            return;
        }
        program_push_instruction(&parser->parse_result.program, instruction_create_sniff(channel, direction, target_register), parser->current_line);
    }
    else if (token_matches_str(&tokens[0], "SMELL"))
    {
        if (!parser_verify_nb_arguments(parser, 2, 3, nb_token))
        {
            return;
        } 
        Argument channel;
        uint8_t target_register = 0; 
        if (!parser_read_argument(parser, &tokens[1], &channel) ||
            (nb_token == 3 && !parser_read_register(parser, &tokens[2], &target_register)))
        {
            return;
        }
        program_push_instruction(&parser->parse_result.program, instruction_create_smell(channel, target_register), parser->current_line);
    }
    else if (token_matches_str(&tokens[0], "PROBE"))
    {
        if (!parser_verify_nb_arguments(parser, 2, 3, nb_token))
        {
            return;
        } 
        Argument direction;
        uint8_t target_register = 0; 
        if (!parser_read_argument(parser, &tokens[1], &direction) ||
            (nb_token == 3 && !parser_read_register(parser, &tokens[2], &target_register)))
        {
            return;
        }
        program_push_instruction(&parser->parse_result.program, instruction_create_probe(direction, target_register), parser->current_line);
    }
    else if (token_matches_str(&tokens[0], "SENSE"))
    {
        if (!parser_verify_nb_arguments(parser, 2, 3, nb_token))
        {
            return;
        } 
        Argument entity_type;
        uint8_t target_register = 0; 
        if (!parser_read_argument(parser, &tokens[1], &entity_type) ||
            (nb_token == 3 && !parser_read_register(parser, &tokens[2], &target_register)))
        {
            return;
        }
        program_push_instruction(&parser->parse_result.program, instruction_create_sense(entity_type, target_register), parser->current_line);
    }
    else if (token_matches_str(&tokens[0], "TAG"))
    {
        if (!parser_verify_nb_arguments(parser, 2, 2, nb_token))
        {
            return;
        } 
        Argument tag_value;
        if (!parser_read_argument(parser, &tokens[1], &tag_value))
        {
            return;
        }
        program_push_instruction(&parser->parse_result.program, instruction_create_tag(tag_value), parser->current_line);
    }
    else 
    {
        parser_push_error(parser, "Unknown instruction name");
    }
}

bool 
parser_read_directive(Parser* parser)
{
    bool ok = true;
    assert(*parser->current_position == '.');
    parser_advance(parser);
    Token directive_token = parser_read_token(parser);
    Token arg1 = parser_read_token(parser);
    Token arg2 = parser_read_token(parser);
    if (token_is_null(&directive_token) || token_is_null(&arg1) || token_is_null(&arg2))
    {
        parser_push_error(parser, "Syntax error");
        ok = false; 
        goto end;
    }
    if (token_matches_str(&directive_token, "const") || token_matches_str(&directive_token, "alias"))
    {
        bool is_alias = token_matches_str(&directive_token, "alias");
        Argument parsed_arg;
        if (!parser_read_argument(parser, &arg2, &parsed_arg) ||
            parsed_arg.is_register != is_alias)
        {
            parser_push_error(parser, "Invalid argument for const/alias directive");
            ok = false; 
            goto end;
        }
        ConstantEntry entry; 
        entry.name = arg1; 
        entry.value = parsed_arg;
        vector_constant_entry_push(&parser->constants, entry);
    }
    else if (token_matches_str(&directive_token, "tag"))
    {
        Argument parsed_arg;
        if (!parser_read_argument(parser, &arg1, &parsed_arg) || parsed_arg.is_register)
        {
            parser_push_error(parser, "Invalid argument for tag directive");
            ok = false; 
            goto end;
        }
        ConstantEntry entry; 
        entry.name = arg2; 
        entry.value = parsed_arg;
        vector_constant_entry_push(&parser->constants, entry);
        char* tag_name = strndup(arg2.begin, arg2.end - arg2.begin);
        program_add_tag(&parser->parse_result.program, parsed_arg.value, tag_name); 
        free(tag_name);
    }
    else
    {
        parser_push_error(parser, "Unknown directive");
        ok = false; 
        goto end;
    }
    parser_skip_whitespace(parser);
    if (!parser_has_line_ended(parser))
    {
        parser_push_error(parser, "Too many arguments in directive");
        ok = false; 
        goto end;
    }
end:
    parser_skip_line(parser);
    return ok;
}

bool 
parser_read_tokens_from_line(Parser *parser)
{
    parser_skip_whitespace(parser);
    if (*parser->current_position == '.')
    {
        return parser_read_directive(parser);
    }
    TokenLine token_line;
    token_line_init(&token_line, parser->current_line);

    while (!parser_has_line_ended(parser) && token_line.nb_tokens < PARSER_MAX_TOKE_PER_LINE)
    {
        token_line.tokens[token_line.nb_tokens] = parser_read_token(parser);
        if (token_is_null(&token_line.tokens[token_line.nb_tokens]))
        {
            parser_push_error(parser, "Unexpected character");
            parser_skip_line(parser);
            return false; 
        }
        token_line.nb_tokens++;

        parser_skip_whitespace(parser);
        if (*parser->current_position == ';')
        {
            //The rest of the line can be ignored.
            break;
        }
        if (token_line.nb_tokens == 1 && *parser->current_position == ':')
        {
            //We are reading a label.
            //TODO fail when redefining an existing constant.
            parser_advance(parser); 
            parser_skip_whitespace(parser); 
            if (!parser_has_line_ended(parser))
            {
                parser_push_error(parser, "Syntax error, can't have a command on the same line as a label");
                parser_skip_line(parser);
                return false;
            }
            ConstantEntry entry; 
            entry.name = token_line.tokens[0]; 
            entry.value = argument_create_value(vector_token_line_size(&parser->token_lines));
            vector_constant_entry_push(&parser->constants, entry);
            token_line.nb_tokens = 0;
            break;
        }
    }
    //Skip the remaining of the line, which is either just the newline or comments.
    parser_skip_line(parser);
    if (token_line.nb_tokens > 0)
    {
        vector_token_line_push(&parser->token_lines, token_line);
    }
    return true;
}

ParseResult 
parse_program_from_string(const char *content)
{
    Parser parser; 
    parser_init(&parser, content); 

    //First pass, read from the string input
    while (*parser.current_position != '\0')
    {
        parser_read_tokens_from_line(&parser);
    }
    //Second pass parse the tokens into instructions
    for (const TokenLine* it = parser.token_lines.begin; it != parser.token_lines.end; it++)
    {
        read_instruction_from_tokens(&parser, it);
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


ParseResult 
parse_program_from_file(const char *file_path)
{
    char* content = parse_read_file(file_path);
    if (content == NULL)
    {
        ParseResult result;
        parse_result_init(&result);
        ParseError error; 
        error.line = 0; 
        error.message = "Failed to open file";
        vector_parse_error_push(result.errors, error); 
        return result;
    }
    ParseResult result = parse_program_from_string(content);
    free(content);
    return result;
}

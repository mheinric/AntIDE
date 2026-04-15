#include "parser.h"

#define VECTOR_ITEM_TYPE ParseError
#define VECTOR_ITEM_NAME parse_error
#include "internals/vector.h"

typedef struct {
    const char* begin; 
    const char* end;
} Token;

static const Token NULL_TOKEN = {
    .begin = NULL,
    .end = NULL
};

void 
token_init(Token* token, const char* begin, const char* end);

/**
 * Initialises the token from a null-terminated string.
 */
void 
token_init_from_string(Token* token, const char* content);

bool 
token_is_null(const Token *token);

bool
token_equal(const Token *first, const Token* second);

bool 
token_matches_str(const Token *token, const char *str);

enum { PARSER_MAX_TOKE_PER_LINE = 5 };

typedef struct {
    Token tokens[PARSER_MAX_TOKE_PER_LINE]; 
    int nb_tokens;
    size_t line_number;
} TokenLine;

void
token_line_init(TokenLine* token_line, size_t line_number);

#define VECTOR_ITEM_TYPE TokenLine
#define VECTOR_ITEM_NAME token_line
#include "internals/vector.h"

typedef struct {
    Token name;
    Argument value; 
} ConstantEntry;

#define VECTOR_ITEM_TYPE ConstantEntry
#define VECTOR_ITEM_NAME constant_entry
#include "internals/vector.h"

typedef struct {
    const char* content;
    const char* current_position;
    size_t current_line;
    // Intermediate data structure for storing stuff between phase 1 and phase 2 of the parser
    VectorConstantEntry constants;
    VectorTokenLine token_lines; 
    //The final result that this parser produces.
    ParseResult parse_result;
} Parser;

void 
parser_init(Parser* parser, const char* content);
void 
parser_cleanup(Parser* parser);

void
parser_advance(Parser *parser);

void 
parser_push_error(Parser *parser, const char* message);

bool 
parser_has_line_ended(Parser *parser);

void
parser_skip_whitespace(Parser *parser);

void
parser_skip_line(Parser *parser);

Token 
parser_read_token(Parser *parser);

bool 
parser_verify_nb_arguments(Parser *parser, unsigned expected, unsigned actual);

bool 
parser_get_digit_value(char c, int32_t *digit_value, int32_t base);

bool 
parser_read_integer(Parser* parser, const Token* token, int32_t *target);

bool 
parser_read_argument(Parser* parser, const Token *token, Argument *target);

bool
parser_read_register(Parser *parser, const Token *token, uint8_t *target_reg);

void 
parser_read_artihmetic_instruction(
    Parser *parser,
    InstructionType type, 
    const Token *tokens, 
    unsigned nb_token);

void 
parser_read_conditional_jump_instruction(
    Parser *parser, 
    InstructionType type, 
    const Token *tokens, 
    unsigned nb_token);

void
parser_read_info_instruction(Parser *parser, 
    InstructionType type, 
    const Token *tokens, 
    unsigned nb_token);

void
read_instruction_from_tokens(
    Parser* parser,
    const TokenLine* token_line);

bool 
parser_read_directive(Parser* parser);

bool 
parser_read_tokens_from_line(Parser *parser);

char* 
parse_read_file(const char* file_name);
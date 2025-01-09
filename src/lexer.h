#pragma once

#include "utils.h"
#include <stdbool.h>

typedef enum {
    TOKEN_KEY,
    TOKEN_EQUALS,
    TOKEN_STRING,
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_BOOL,
    TOKEN_BRACE_LEFT,
    TOKEN_BRACE_RIGHT,
    TOKEN_BRACKET_LEFT,
    TOKEN_BRACKET_RIGHT,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_EOF,
    TOKEN_ERROR
} token_type_t;

typedef struct {
    token_type_t type;
    union {
        struct {
            const char* start;
            int length;
        } str;
        int integer;
        double float_val;
        bool boolean;
    } value;
    int line;
    int column;
} token_t;

typedef struct {
    const char* start;
    const char* current;
    int line;
    int column;
} lexer_t;

lexer_t *lexer_init(const char *input);
token_t *lexer_scan_token(lexer_t *lexer);

char lexer_advance(lexer_t *lexer);
char lexer_peek(lexer_t *lexer);
char lexer_peek_next(lexer_t *lexer);
bool lexer_match(lexer_t *lexer, char expected);
bool lexer_is_at_end(lexer_t *lexer);

token_t *lexer_handle_string(lexer_t *lexer);
token_t *lexer_handle_number(lexer_t *lexer);
token_t *lexer_handle_keyword(lexer_t *lexer);

token_t* token_create(token_type_t type, int line, int column);
void token_free(token_t* token);

void test_tokenizer(const char* input);

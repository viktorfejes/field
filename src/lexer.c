#include "lexer.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

lexer_t *lexer_init(const char *input) {
    lexer_t *lexer = malloc(sizeof(lexer_t));
    if (!lexer) return NULL;

    lexer->start = input;
    lexer->current = input;
    lexer->line = 1;
    lexer->column = 1;
    return lexer;
}

token_t *lexer_scan_token(lexer_t *lexer) {
    // Skip whitespace
    while (!lexer_is_at_end(lexer)) {
        char c = lexer_peek(lexer);
        if (c == ' ' || c == '\r' || c == '\t') {
            lexer_advance(lexer);
            continue;
        }
        else if (c == '\n') {
            lexer->line++;
            lexer->column = 1;
            lexer_advance(lexer);
            continue;
        }
        break;
    }

    // Store the start of the token
    lexer->start = lexer->current;

    if (lexer_is_at_end(lexer)) {
        return token_create(TOKEN_EOF, lexer->line, lexer->column);
    }

    char c = lexer_advance(lexer);

    // Handle comments
    if (c == '/' && lexer_peek(lexer) == '/') {
        while (lexer_peek(lexer) != '\n' && !lexer_is_at_end(lexer)) lexer_advance(lexer);
        return lexer_scan_token(lexer); // Recursively get the next token
    }

    // Handle string literals
    if (c == '"') return lexer_handle_string(lexer);

    // Handle numbers
    if (is_digit(c)) {
        // Back up so number() sees the first digit
        lexer->current--;
        return lexer_handle_number(lexer);
    }

    // Handle key and keywords
    if (is_alpha(c)) {
        // Back up so the keywords() sees the first character
        lexer->current--;
        return lexer_handle_keyword(lexer);
    }

    // Handle single character tokens
    switch (c) {
        case '=': return token_create(TOKEN_EQUALS, lexer->line, lexer->column - 1);
        case '{': return token_create(TOKEN_BRACE_LEFT, lexer->line, lexer->column - 1);
        case '}': return token_create(TOKEN_BRACE_RIGHT, lexer->line, lexer->column - 1);
        case '[': return token_create(TOKEN_BRACKET_LEFT, lexer->line, lexer->column - 1);
        case ']': return token_create(TOKEN_BRACKET_RIGHT, lexer->line, lexer->column - 1);
        case ';': return token_create(TOKEN_SEMICOLON, lexer->line, lexer->column - 1);
        case ',': return token_create(TOKEN_COMMA, lexer->line, lexer->column - 1);
    }

    // If we get here, error
    return token_create(TOKEN_ERROR, lexer->line, lexer->column - 1);
}

char lexer_advance(lexer_t *lexer) {
    lexer->column++;
    return *lexer->current++;
}

char lexer_peek(lexer_t *lexer) {
    return *lexer->current;
}

char lexer_peek_next(lexer_t *lexer) {
    if (lexer_is_at_end(lexer)) return '\0';
    return lexer->current[1];
}

bool lexer_match(lexer_t *lexer, char expected) {
    if (lexer_is_at_end(lexer)) return false;
    if (*lexer->current != expected) return false;

    lexer->current++;
    lexer->column++;
    return true;
}

bool lexer_is_at_end(lexer_t *lexer) {
    return *lexer->current == '\0';
}

token_t *lexer_handle_string(lexer_t *lexer) {
    while (lexer_peek(lexer) != '"' && !lexer_is_at_end(lexer)) {
        if (lexer_peek(lexer) == '\n') {
            lexer->line++;
            lexer->column = 1;
        }
        lexer_advance(lexer);
    }

    if (lexer_is_at_end(lexer)) {
        // Unterminated string
        return token_create(TOKEN_ERROR, lexer->line, lexer->column);
    }

    // Consume the closing quote
    lexer_advance(lexer);

    // Create token pointing to the string's contents (excluding quotes)
    token_t *token = token_create(TOKEN_STRING, lexer->line, lexer->column);
    token->value.str.start = lexer->start + 1; // Skipping opening quote
    token->value.str.length = (lexer->current - lexer->start) - 2; // Exclude both quotes
    return token;
}

token_t *lexer_handle_number(lexer_t *lexer) {
    bool is_float = false;

    while (is_digit(lexer_peek(lexer))) lexer_advance(lexer);

    // Look for decimal point followed by numbers
    if (lexer_peek(lexer) == '.' && is_digit(lexer_peek_next(lexer))) {
        is_float = true;
        lexer_advance(lexer); // Consume the '.'
        while (is_digit(lexer_peek(lexer))) lexer_advance(lexer);
    }

    // Create the appropriate numeric token
    token_t *token;
    int length = lexer->current - lexer->start;
    
    // TODO: I'm quite sure this could be a fixed char array as
    // we must know what's the max length for a float.
    char* num_str = malloc(length + 1);

    strncpy(num_str, lexer->start, length);
    num_str[length] = '\0';

    if (is_float) {
        token = token_create(TOKEN_FLOAT, lexer->line, lexer->column);
        token->value.float_val = atof(num_str);
    } else {
        token = token_create(TOKEN_INT, lexer->line, lexer->column);
        token->value.integer = atoi(num_str);
    }

    free(num_str);
    return token;
}

token_t *lexer_handle_keyword(lexer_t *lexer) {
    while (is_alpha(lexer_peek(lexer)) || is_digit(lexer_peek(lexer))) {
        lexer_advance(lexer);
    }

    // Check if it's a keyword (true/false)
    int length = lexer->current - lexer->start;
    if (length == 4 && strnicmp(lexer->start, "true", 4) == 0) {
        token_t *token = token_create(TOKEN_BOOL, lexer->line, lexer->column);
        token->value.boolean = true;
        return token;
    }
    if (length == 5 && strnicmp(lexer->start, "false", 5) == 0) {
        token_t *token = token_create(TOKEN_BOOL, lexer->line, lexer->column);
        token->value.boolean = false;
        return token;
    }

    // Regular key
    token_t *token = token_create(TOKEN_KEY, lexer->line, lexer->column);
    token->value.str.start = lexer->start;
    token->value.str.length = length;
    return token;
}

token_t* token_create(token_type_t type, int line, int column) {
    token_t* token = malloc(sizeof(token_t));
    if (token == NULL) return NULL;

    token->type = type;
    token->line = line;
    token->column = column;

    return token;
}

void token_free(token_t* token) {
    if (token == NULL) return;
    
    free(token);
}

void test_tokenizer(const char* input) {
    lexer_t *lexer = lexer_init(input);
    if (!lexer) {
        printf("Failed to init lexer\n");
        return;
    }

    // Token type names for debugging
    const char* token_names[] = {
        "KEY",
        "EQUALS",
        "STRING",
        "INT",
        "FLOAT",
        "BOOL",
        "BRACE_LEFT",
        "BRACE_RIGHT",
        "BRACKET_LEFT",
        "BRACKET_RIGHT",
        "SEMICOLON",
        "COMMA",
        "EOF",
        "ERROR"
    };

    while (true) {
        token_t *token = lexer_scan_token(lexer);
        if (!token) {
            printf("Failed to scan token\n");
            break;
        }

        printf("Line %d, Col %d: %s", token->line, token->column, token_names[token->type]);

        // Print token value based on type
        switch (token->type) {
            case TOKEN_STRING:
            case TOKEN_KEY:
                printf(" '%.*s'", token->value.str.length, token->value.str.start);
                break;
            
            case TOKEN_INT:
                printf(" %d", token->value.integer);
                break;

            case TOKEN_FLOAT:
                printf(" %f", token->value.float_val);
                break;

            case TOKEN_BOOL:
                printf(" %s", token->value.boolean ? "true" : "false");
                break;

            default: break;
        }
        printf("\n");

        if (token->type == TOKEN_EOF || token->type == TOKEN_ERROR) {
            token_free(token);
            break;
        }
        token_free(token);
    }

    free(lexer);
}

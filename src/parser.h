#pragma once

#include "lexer.h"

#include "bump_allocator.h"

typedef enum {
    VALUE_EMPTY,
    VALUE_STRING,
    VALUE_INT,
    VALUE_FLOAT,
    VALUE_BOOL,
    VALUE_ARRAY,
    VALUE_OBJECT,
} value_type_t;

typedef struct value value_t;
typedef struct {
    value_t *values;
    int count;
    int capacity;
} value_array_t;

struct value {
    value_type_t type;
    union {
        struct {
            const char* start;
            int length;
        } string;
        int integer;
        double float_val;
        bool boolean;
        value_array_t array;
        struct {
            char **keys;
            value_t *values;
            int count;
            int capacity;
        } object;
    } as;
};

typedef struct {
    lexer_t *lexer;
    token_t *current;
    token_t *previous;
    bool had_error;
    char *error_message;
} parser_t;

typedef struct {
    parser_t parser;
    bump_allocator_t bump_alloc;
} flex_parser_t;

value_t *parse(const char* source, void *memory);

parser_t *parser_init(lexer_t *lexer);
value_t *parser_parse(const char* source);

void parser_advance(parser_t *parser);
void parser_consume(parser_t *parser, token_type_t type, const char* error_message);
bool parser_match(parser_t *parser, token_type_t type);
void parser_error(parser_t *parser, const char* message);

value_t *parser_parse_field(parser_t *parser);
value_t *parser_parse_object(parser_t *parser);
value_t *parser_parse_value(parser_t *parser);
value_t *parser_parse_array(parser_t *parser);

void parser_test(const char* input);

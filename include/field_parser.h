/*
*   fld_parser - v0.4
*   Header-only library for parsing configuration files in the FLD format.
*
*   RECENT CHANGES:
*       0.4     (2025-01-10)    Added iterator support to the parser;
*       0.3     (2025-01-10)    Renamed `fld_get_string` to `fld_get_str_view`,
*                               Added `fld_get_cstr` for immediate c string retrival,
*                               Added `fld_has field`, `fld_get_type`, `fld_get_array_size`,
*                               `fld_string_view_to_cstr`, `fld_string_view_eq`,
*                               `fld_get_last_error`, `fld_error_string`, and `fld_estimate_memory`;
*       0.21    (2025-01-10)    Removed stddef.h that wasn't needed anymore;
*       0.2     (2025-01-10)    Removed some remaining temp code,
*                               Added block comment support,
*                               Updated readme and `test.c` to reflect changes;
*       0.1     (2025-01-09)    Finalized first implementation;
*
*   LICENSE: MIT License
*       Copyright (c) 2025 Viktor Fejes
*
*       Permission is hereby granted, free of charge, to any person obtaining a copy
*       of this software and associated documentation files (the "Software"), to deal
*       in the Software without restriction, including without limitation the rights
*       to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*       copies of the Software, and to permit persons to whom the Software is
*       furnished to do so, subject to the following conditions:
*
*       The above copyright notice and this permission notice shall be included in all
*       copies or substantial portions of the Software.
*
*       THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*       IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*       FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*       AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*       LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*       OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*       SOFTWARE.
*
*   TODOs:
*       - [ ] A bit more testing...
*       - [ ] Run some tests to see how close the
*             memory estimating function gets.
*       - [ ] Remove stdlib.h include by writing own
*             int and float parser.
*       - [ ] Full documentation of public API
*
 */

#ifndef FLD_PARSER_H
#define FLD_PARSER_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *start;
    int length;
} fld_string_view;

typedef enum fld_value_type {
    FLD_VALUE_EMPTY,
    FLD_VALUE_STRING,
    FLD_VALUE_INT,
    FLD_VALUE_FLOAT,
    FLD_VALUE_BOOL,
    FLD_VALUE_ARRAY,
    FLD_VALUE_OBJECT,
} fld_value_type;

typedef struct fld_value {
    fld_value_type type;
    union {
        fld_string_view string;
        int integer;
        float float_val;
        bool boolean;
        struct {
            fld_value_type type;
            int count;
            void *items;
        } array;
        struct fld_object *object;
    };
} fld_value;

typedef struct fld_object {
    fld_string_view key;
    fld_value value;

    struct fld_object *next;
    struct fld_object *parent;
} fld_object;

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
} fld_token_type;

typedef struct {
    fld_token_type type;
    union {
        fld_string_view string;
        int integer;
        double float_val;
        bool boolean;
    } value;
    int line;
    int column;
} fld_token;

typedef struct {
    char *start;
    char *current;
    int line;
    int column;
} fld_lexer;

typedef enum {
    FLD_ERROR_NONE,
    FLD_ERROR_OUT_OF_MEMORY,
    FLD_ERROR_UNEXPECTED_TOKEN,
    FLD_ERROR_INVALID_NUMBER,
    FLD_ERROR_INSUFFICIENT_MEMORY,
    FLD_ERROR_ARRAY_TYPE_MISMATCH,
    FLD_ERROR_ARRAY_NOT_SUPPORTED_TYPE,
    FLD_ERROR_ARRAY_TOO_MANY_ITEMS
} fld_error_code;

typedef struct {
    fld_error_code code;
    int line;
    int column;
} fld_error;

typedef struct {
    uint8_t *start;
    uint8_t *end;
    uint8_t *current;
} fld_bump_allocator;

typedef struct {
    fld_lexer lexer;
    fld_token *current;
    fld_token *previous;
    
    char *source;
    fld_error last_error;
    fld_bump_allocator allocator;
    fld_object *root;
} fld_parser;

typedef enum fld_iter_type {
    FLD_ITER_FIELDS,    // Iterate over fields at current level
    FLD_ITER_RECURSIVE, // Recursively iterate all fields including nested
} fld_iter_type;

typedef struct {
    fld_object *current;
    fld_object *parent;
    fld_iter_type type;
    int depth;
} fld_iterator;

#if defined(__cplusplus)
    #define ALIGNOF(type) alignof(type)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #define ALIGNOF(type) _Alignof(type)
#elif defined(__GNUC__) || defined(__clang__)
    #define ALIGNOF(type) __alignof__(type)
#elif defined(_MSC_VER)
    #define ALIGNOF(type) __alignof(type)
#else
    #define ALIGNOF(type) ((size_t)(&((struct { char c; type d; }*)0)->d))
#endif

#define FLD_MAX_PATH_LENGTH 128
#define FLD_MAX_ARRAY_ITEMS 128

/**
 * @brief Parses the given source using the specified parser.
 * 
 * @param parser A pointer to the fld_parser structure that will be used for parsing.
 * @param source A constant character pointer to the source string to be parsed.
 * @param memory A pointer to the memory where the parsed data will be stored.
 * @param size The size of the memory buffer.
 * @return true if parsing is successful, false otherwise.
 */
extern bool fld_parse(fld_parser *parser, const char *source, void *memory, size_t size);

extern fld_object *fld_get_field(fld_object *object, const char *key);
extern fld_object *fld_get_field_by_path(fld_object *object, const char *path);

/**
 * @brief Retrieves a string view associated with a given key from the specified fld_object.
 * 
 * @param object Pointer to the fld_object from which the string view is to be retrieved.
 * @param key The key associated with the desired string view.
 * @param str_view Pointer to an fld_string_view structure where the result will be stored.
 * @return true if the string view is successfully retrieved, false otherwise.
 */
extern bool fld_get_str_view(fld_object *object, const char *key, fld_string_view *str_view);

/**
 * @brief Retrieves a C-string value associated with a given key from a field object.
 *
 * This function searches for a key within the specified field object and, if found,
 * copies the associated C-string value into the provided buffer. The buffer size
 * must be large enough to hold the value, including the null-terminator.
 *
 * @param object Pointer to the field object from which to retrieve the value.
 * @param key The key associated with the desired C-string value.
 * @param buffer Pointer to the buffer where the retrieved C-string value will be stored.
 * @param buffer_size The size of the buffer, in bytes.
 * @return true if the key was found and the value was successfully copied to the buffer; 
 *         false otherwise.
 */
extern bool fld_get_cstr(fld_object *object, const char *key, char *buffer, size_t buffer_size);

/**
 * @brief Retrieves an integer value from a field object based on the provided key.
 *
 * This function searches for the specified key within the given field object and,
 * if found, assigns the corresponding integer value to the output parameter.
 *
 * @param object A pointer to the field object from which to retrieve the value.
 * @param key The key associated with the desired integer value.
 * @param out_value A pointer to an integer where the retrieved value will be stored.
 * @return true if the key was found and the value was successfully retrieved, false otherwise.
 */
extern bool fld_get_int(fld_object *object, const char *key, int *out_value);
extern bool fld_get_float(fld_object *object, const char *key, float *out_value);
extern bool fld_get_bool(fld_object *object, const char *key, bool *out_value);
extern bool fld_get_array(fld_object *object, const char *key, fld_value_type *out_type, void** out_items, size_t* out_count);
extern bool fld_get_object(fld_object *object, const char *key, fld_object **out_object);

extern bool fld_has_field(fld_object *object, const char *key);
extern fld_value_type fld_get_type(fld_object *object, const char *key);
extern bool fld_get_array_size(fld_object *object, const char *key, size_t *out_size);

extern bool fld_string_view_to_cstr(fld_string_view str_view, char *buffer, size_t buffer_size);
extern bool fld_string_view_eq(fld_string_view str_view, const char* cstr);

extern size_t fld_estimate_memory(const char *source);

/**
 * @brief Advances the iterator to the next field object.
 *
 * This function retrieves the next field object in the sequence
 * represented by the iterator. If there are no more objects, it
 * returns nullptr.
 *
 * @param iter A pointer to the field iterator.
 * @return A pointer to the next field object, or nullptr if there
 *         are no more objects.
 */
extern fld_object *fld_iter_next(fld_iterator *iter);

/**
 * @brief Initializes a field iterator.
 *
 * This function initializes a field iterator with the given root object and iterator type.
 *
 * @param iter Pointer to the field iterator to initialize.
 * @param root Pointer to the root field object.
 * @param type Type of the iterator.
 */
static inline void fld_iter_init(fld_iterator *iter, fld_object *root, fld_iter_type type) {
    iter->current = root;
    iter->parent = NULL;
    iter->type = type;
    iter->depth = 0;
}

static inline fld_error fld_get_last_error(fld_parser *parser) {
    return parser->last_error;
}

static inline const char *fld_error_string(fld_error_code code) {
    switch (code) {
        case FLD_ERROR_NONE: return "No error";
        case FLD_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case FLD_ERROR_UNEXPECTED_TOKEN: return "Unexpected token";
        case FLD_ERROR_INVALID_NUMBER: return "Invalid number format";
        case FLD_ERROR_INSUFFICIENT_MEMORY: return "Insufficient memory provided";
        case FLD_ERROR_ARRAY_TYPE_MISMATCH: return "Array type mismatch";
        case FLD_ERROR_ARRAY_NOT_SUPPORTED_TYPE: return "Unsupported array type";
        case FLD_ERROR_ARRAY_TOO_MANY_ITEMS: return "Too many items in array";
        default: return "Unknown error";
    }
}

#ifdef __cplusplus
}
#endif

// TEMP: For developing
// #define FLD_PARSER_IMPLEMENTATION
#ifdef FLD_PARSER_IMPLEMENTATION

// Forward declarations
static fld_value *_parse_object(fld_parser *parser);
static fld_value *_parse_value(fld_parser *parser);
static fld_object *_parse_field(fld_parser *parser);

static inline void _bump_init(fld_bump_allocator *alloc, void *memory, size_t size) {
    alloc->start = (uint8_t*)memory;
    alloc->current = alloc->start;
    alloc->end = alloc->start + size;
}

static inline uintptr_t _align_up(uintptr_t addr, size_t align) {
    return (addr + (align - 1) & ~(align - 1));
}

static inline void *_bump_alloc(fld_bump_allocator *alloc, size_t size, size_t align) {
    uint8_t *aligned = (uint8_t*)_align_up((uintptr_t)alloc->current, align);
    uint8_t *new_current = aligned + size;

    // Check if we have enough space by address comparison
    if (new_current > alloc->end) {
        return 0;
    }

    alloc->current = new_current;
    return aligned;
}
// TODO: this can be used by the aligned bump_alloc
static inline void *_bump_alloc_raw(fld_bump_allocator *alloc, size_t size) {
    uint8_t *new_current = alloc->current + size;

    // Check if we have enough space by address comparison
    if (new_current > alloc->end) {
        return 0;
    }

    uint8_t* result = alloc->current;
    alloc->current = new_current;
    return result;
}

// TODO: Run a couple tests to see how close this estimate is
static inline size_t _estimate_memory_needed(const char *source, size_t *out_len) {
    *out_len = strlen(source);

    // Heuristic
    // - Assume we might need a value/object for every ~4 chars on avg.
    // - Plus the full length of the source text as we'll be copying it.
    // - With some overhead for worst case.
    size_t estimate = (*out_len / 4) * (sizeof(fld_value) + sizeof(fld_object));
    size_t overhead = 1024;

    return estimate + overhead;
}

static inline bool _is_digit(char c) {
    return c >= '0' && c <= '9';
}

static inline bool _is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool _lexer_is_at_end(fld_lexer *lexer) {
    return *lexer->current == '\0';
}

static char _lexer_advance(fld_lexer *lexer) {
    lexer->column++;
    return *lexer->current++;
}

static char _lexer_peek(fld_lexer *lexer) {
    return *lexer->current;
}

static char _lexer_peek_next(fld_lexer *lexer) {
    if (_lexer_is_at_end(lexer)) return '\0';
    return lexer->current[1];
}

static fld_token *_token_create(fld_parser *parser, fld_token_type type, int line, int column) {
    fld_token *token = (fld_token*)_bump_alloc(&parser->allocator, sizeof(fld_token), ALIGNOF(fld_token));
    if (token == NULL) {
        parser->last_error.line = line;
        parser->last_error.column = column;
        parser->last_error.code = FLD_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    token->type = type;
    token->line = line;
    token->column = column;

    return token;
}

static fld_token *_lexer_handle_string(fld_parser *parser, fld_lexer *lexer) {
    while (_lexer_peek(lexer) != '"' && !_lexer_is_at_end(lexer)) {
        if (_lexer_peek(lexer) == '\n') {
            lexer->line++;
            lexer->column = 1;
        }
        _lexer_advance(lexer);
    }

    if (_lexer_is_at_end(lexer)) {
        // Unterminated string
        return _token_create(parser, TOKEN_ERROR, lexer->line, lexer->column);
    }

    // Consume the closing quote
    _lexer_advance(lexer);

    // Create token pointing to the string's contents (excluding quotes)
    fld_token *token = _token_create(parser, TOKEN_STRING, lexer->line, lexer->column);
    token->value.string.start = lexer->start + 1; // Skipping opening quote
    token->value.string.length = (lexer->current - lexer->start) - 2; // Exclude both quotes

    return token;
}

static fld_token *_lexer_handle_number(fld_parser *parser, fld_lexer *lexer) {
    bool is_float = false;

    // As long as we are encountering digits, let's advance
    while (_is_digit(_lexer_peek(lexer))) {
        _lexer_advance(lexer);
    }

    // Look for decimal point followed by numbers
    if (_lexer_peek(lexer) == '.' && _is_digit(_lexer_peek_next(lexer))) {
        is_float = true;
        // Consume the '.'
        _lexer_advance(lexer);

        // Continue with the remaining digits
        while (_is_digit(_lexer_peek(lexer))) {
            _lexer_advance(lexer);
        }
    }

    // Create the appropriate numeric token
    fld_token *token;
    int length = lexer->current - lexer->start;
    
    // FLT_DECIMAL_DIG is 9 but let's do 16 just to be safe
    // no dynamic allocations needed...
    char num_str[16];
    strncpy(num_str, lexer->start, length);
    num_str[length] = '\0';

    if (is_float) {
        token = _token_create(parser, TOKEN_FLOAT, lexer->line, lexer->column);
        token->value.float_val = atof(num_str);
    } else {
        token = _token_create(parser, TOKEN_INT, lexer->line, lexer->column);
        token->value.integer = atoi(num_str);
    }

    return token;
}

static fld_token *_lexer_handle_keyword(fld_parser *parser, fld_lexer *lexer) {
    while (_is_alpha(_lexer_peek(lexer)) || _is_digit(_lexer_peek(lexer))) {
        _lexer_advance(lexer);
    }

    // Check if it's a keyword (true/false)
    int length = lexer->current - lexer->start;
    if (length == 4 && strncmp(lexer->start, "true", 4) == 0) {
        fld_token *token = _token_create(parser, TOKEN_BOOL, lexer->line, lexer->column);
        token->value.boolean = true;
        return token;
    }
    if (length == 5 && strncmp(lexer->start, "false", 5) == 0) {
        fld_token *token = _token_create(parser, TOKEN_BOOL, lexer->line, lexer->column);
        token->value.boolean = false;
        return token;
    }

    // Regular key
    fld_token *token = _token_create(parser, TOKEN_KEY, lexer->line, lexer->column);
    token->value.string.start = lexer->start;
    token->value.string.length = length;

    return token;
}

static fld_token *_lexer_scan_token(fld_parser *parser) {
    fld_lexer *lexer = &parser->lexer;
    
    // Skip whitespace
    while (!_lexer_is_at_end(lexer)) {
        char c = _lexer_peek(lexer);
        if (c == ' ' || c == '\r' || c == '\t') {
            _lexer_advance(lexer);
            continue;
        }
        else if (c == '\n') {
            lexer->line++;
            lexer->column = 1;
            _lexer_advance(lexer);
            continue;
        }
        break;
    }

    // Store the start of the token
    lexer->start = lexer->current;

    if (_lexer_is_at_end(lexer)) {
        return _token_create(parser, TOKEN_EOF, lexer->line, lexer->column);
    }

    char c = _lexer_advance(lexer);

    // Handle comments
    if (c == '/') {
        // Line comment
        if (_lexer_peek(lexer) == '/') {
            while (_lexer_peek(lexer) != '\n' && !_lexer_is_at_end(lexer)) {
                _lexer_advance(lexer);
            }
            // Recursively get the next token
            return _lexer_scan_token(parser);
        }
        // Block comment
        else if (_lexer_peek(lexer) == '*') {
            // Consume the '*'
            _lexer_advance(lexer);

            while (!_lexer_is_at_end(lexer)) {
                if (_lexer_peek(lexer) == '*' && _lexer_peek_next(lexer) == '/') {
                    // Consume '*'
                    _lexer_advance(lexer);
                    // Consume '/'
                    _lexer_advance(lexer);
                    // Recursively get the next token
                    return _lexer_scan_token(parser);
                }

                // Handle newlines in block comments
                if (_lexer_peek(lexer) == '\n') {
                    lexer->line++;
                    lexer->column = 1;
                }

                _lexer_advance(lexer);
            }
        }
    }

    // Handle string literals
    if (c == '"') return _lexer_handle_string(parser, lexer);

    // Handle numbers
    if (_is_digit(c)) {
        // Back up so number() sees the first digit
        lexer->current--;
        return _lexer_handle_number(parser, lexer);
    }

    // Handle key and keywords
    if (_is_alpha(c)) {
        // Back up so the keywords() sees the first character
        lexer->current--;
        return _lexer_handle_keyword(parser, lexer);
    }

    // Handle single character tokens
    switch (c) {
        case '=': return _token_create(parser, TOKEN_EQUALS, lexer->line, lexer->column - 1);
        case '{': return _token_create(parser, TOKEN_BRACE_LEFT, lexer->line, lexer->column - 1);
        case '}': return _token_create(parser, TOKEN_BRACE_RIGHT, lexer->line, lexer->column - 1);
        case '[': return _token_create(parser, TOKEN_BRACKET_LEFT, lexer->line, lexer->column - 1);
        case ']': return _token_create(parser, TOKEN_BRACKET_RIGHT, lexer->line, lexer->column - 1);
        case ';': return _token_create(parser, TOKEN_SEMICOLON, lexer->line, lexer->column - 1);
        case ',': return _token_create(parser, TOKEN_COMMA, lexer->line, lexer->column - 1);
    }

    // If we get here, error
    return _token_create(parser, TOKEN_ERROR, lexer->line, lexer->column - 1);
}

static void _parser_error(fld_parser *parser, fld_error_code error_code) {
    // Avoid cascading errors
    if (parser->last_error.code != FLD_ERROR_NONE) return;

    parser->last_error.code = error_code;
    parser->last_error.line = parser->current->line;
    parser->last_error.column = parser->current->column;
}

static void _parser_advance(fld_parser *parser) {
    // Save the current token for error reporting
    parser->previous = parser->current;

    // Keep getting tokens until we get a non-error one or reach EOF
    while (true) {
        parser->current = _lexer_scan_token(parser);
        if (parser->current->type != TOKEN_ERROR) break;

        // Handle error token by reporting it
        _parser_error(parser, FLD_ERROR_UNEXPECTED_TOKEN);
    }
}

static void _parser_consume(fld_parser *parser, fld_token_type type, fld_error_code error_code) {
    // Like match, but raises an error if the token isn't what we expect
    if (parser->current->type == type) {
        _parser_advance(parser);
        return;
    }

    _parser_error(parser, error_code);
}

static bool _parser_match(fld_parser *parser, fld_token_type type) {
    // Check if current token matches expected type
    if (parser->current->type != type) return false;
    _parser_advance(parser);
    return true;
}

static bool _parser_expect(fld_parser *parser, fld_token_type type, fld_error_code error_code) {
    if (!_parser_match(parser, type)) {
        _parser_error(parser, error_code);
        return false;
    }

    return true;
}

static fld_object *_parse_object_fields(fld_parser *parser) {
    // Skip the opening brace
    _parser_advance(parser);

    // Handle empty object
    if (_parser_match(parser, TOKEN_BRACE_RIGHT)) {
        // Empty object has no fields
        return NULL;
    }

    fld_object *first_field = NULL;
    fld_object *last_field = NULL;

    // Parse fields until we hit the closing brace
    while (true) {
        // Expect a key token
        if (parser->current->type != TOKEN_KEY) {
            _parser_error(parser, FLD_ERROR_UNEXPECTED_TOKEN);
            return NULL;
        }

        // Parse a field
        fld_object *field = _parse_field(parser);
        if (!field) {
            return NULL;
        }

        // Initialize field's relationships
        field->parent = NULL;
        field->next = NULL;

        // Add to the linked list
        if (!first_field) {
            first_field = field;
        } else {
            last_field->next = field;
        }
        last_field = field;

        // If we see a closing brace, we're done
        if (parser->current->type == TOKEN_BRACE_RIGHT) {
            break;
        }
    }

    // Consume the closing brace
    if (!_parser_expect(parser, TOKEN_BRACE_RIGHT, FLD_ERROR_UNEXPECTED_TOKEN)) {
        return NULL;
    }

    return first_field;
}

static fld_value *_parse_object(fld_parser *parser) {
    // Allocate and init value
    fld_value *value = (fld_value*)_bump_alloc(&parser->allocator, sizeof(fld_value), ALIGNOF(fld_value));
    if (!value) {
        _parser_error(parser, FLD_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    // Init the structure to zero
    memset(value, 0, sizeof(fld_value));
    value->type = FLD_VALUE_OBJECT;

    // Parse the object's fields
    value->object = _parse_object_fields(parser);
    if (parser->last_error.code != FLD_ERROR_NONE) {
        return NULL;
    }

    // Set parent relationships for all fields
    fld_object *field = value->object;
    while (field) {
        field->parent = NULL;
        field = field->next;
    }

    return value;
}

static size_t _get_type_size(fld_value_type type) {
    switch (type) {
        case FLD_VALUE_STRING: return sizeof(fld_string_view);
        case FLD_VALUE_INT: return sizeof(int);
        case FLD_VALUE_FLOAT: return sizeof(float);
        case FLD_VALUE_BOOL: return sizeof(bool);
        default: return sizeof(fld_value);
    }
}

static size_t _get_type_alignment(fld_value_type type) {
    switch (type) {
        case FLD_VALUE_STRING: return ALIGNOF(fld_string_view);
        case FLD_VALUE_INT: return ALIGNOF(int);
        case FLD_VALUE_FLOAT: return ALIGNOF(float);
        case FLD_VALUE_BOOL: return ALIGNOF(bool);
        default: return ALIGNOF(fld_value);
    }
}

static fld_value *_parse_array(fld_parser *parser) {
    // Save lexer state for rewinding
    char* start_pos = parser->lexer.current;
    int start_line = parser->lexer.line;
    int start_column = parser->lexer.column;

    // First pass - count items and validate first type
    _parser_advance(parser);  // Skip '['

    // Handle empty array fast path
    if (_parser_match(parser, TOKEN_BRACKET_RIGHT)) {
        fld_value *array = (fld_value*)_bump_alloc(&parser->allocator, sizeof(fld_value), ALIGNOF(fld_value));
        if (!array) {
            _parser_error(parser, FLD_ERROR_OUT_OF_MEMORY);
            return NULL;
        }
        array->type = FLD_VALUE_ARRAY;
        array->array.count = 0;
        array->array.items = NULL;
        array->array.type = FLD_VALUE_EMPTY;
        return array;
    }

    // Parse first value to get type
    fld_value *first_value = _parse_value(parser);
    if (!first_value) return NULL;

    // Validate array element type - no nested arrays or objects allowed
    if (first_value->type == FLD_VALUE_ARRAY || first_value->type == FLD_VALUE_OBJECT) {
        _parser_error(parser, FLD_ERROR_ARRAY_NOT_SUPPORTED_TYPE);
        return NULL;
    }

    // Count remaining items
    size_t count = 1;  // We already have one
    while (_parser_match(parser, TOKEN_COMMA)) {
        if (count >= FLD_MAX_ARRAY_ITEMS) {
            _parser_error(parser, FLD_ERROR_ARRAY_TOO_MANY_ITEMS);
            return NULL;
        }
        
        // Skip the value token, don't need to parse it yet
        _parser_advance(parser);
        count++;
    }

    if (!_parser_expect(parser, TOKEN_BRACKET_RIGHT, FLD_ERROR_UNEXPECTED_TOKEN)) {
        return NULL;
    }

    // Restore lexer state
    parser->lexer.current = start_pos;
    parser->lexer.line = start_line;
    parser->lexer.column = start_column;
    parser->current = _lexer_scan_token(parser);

    // Now allocate array and storage
    fld_value *array = (fld_value*)_bump_alloc(&parser->allocator, sizeof(fld_value), ALIGNOF(fld_value));
    if (!array) {
        _parser_error(parser, FLD_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    array->type = FLD_VALUE_ARRAY;
    array->array.type = first_value->type;
    array->array.count = count;

    // Allocate storage for all items
    size_t item_size = _get_type_size(array->array.type);
    size_t item_align = _get_type_alignment(array->array.type);

    void* items = _bump_alloc(&parser->allocator, item_size * count, item_align);
    if (!items) {
        _parser_error(parser, FLD_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    array->array.items = items;

    // Second pass - parse and store values
    char* current = (char*)items;

    for (size_t i = 0; i < count; i++) {
        fld_value *value = _parse_value(parser);
        if (!value) return NULL;

        if (value->type != array->array.type) {
            _parser_error(parser, FLD_ERROR_ARRAY_TYPE_MISMATCH);
            return NULL;
        }

        // Copy value based on type
        switch (array->array.type) {
            case FLD_VALUE_STRING:
                *((fld_string_view*)current) = value->string;
                break;
            case FLD_VALUE_INT:
                *((int*)current) = value->integer;
                break;
            case FLD_VALUE_FLOAT:
                *((float*)current) = value->float_val;
                break;
            case FLD_VALUE_BOOL:
                *((bool*)current) = value->boolean;
                break;
            default:
                _parser_error(parser, FLD_ERROR_ARRAY_TYPE_MISMATCH);
                return NULL;
        }

        current += item_size;
        
        // Skip comma except for last item
        if (i < count - 1) {
            _parser_consume(parser, TOKEN_COMMA, FLD_ERROR_UNEXPECTED_TOKEN);
        }
    }

    // Consume final bracket
    _parser_consume(parser, TOKEN_BRACKET_RIGHT, FLD_ERROR_UNEXPECTED_TOKEN);

    return array;
}

static fld_value *_parse_value(fld_parser *parser) {
    // For objects and arrays we'll just use their appropriate parse functions
    if (parser->current->type == TOKEN_BRACE_LEFT) {
        return _parse_object(parser);
    }
    if (parser->current->type == TOKEN_BRACKET_LEFT) {
        return _parse_array(parser);
    }
    
    // For primitive types allocate and init a value
    fld_value *value = (fld_value*)_bump_alloc(&parser->allocator, sizeof(fld_value), ALIGNOF(fld_value));
    if (!value) {
        _parser_error(parser, FLD_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    // Init the structure to zero for safety
    memset(value, 0, sizeof(fld_value));

    switch (parser->current->type) {
        case TOKEN_STRING: {
            // Validate string view
            if (parser->current->value.string.length > 0 &&
                parser->current->value.string.start != NULL) {
                value->type = FLD_VALUE_STRING;
                value->string = parser->current->value.string;
                _parser_advance(parser);
                return value;
            }
            _parser_error(parser, FLD_ERROR_UNEXPECTED_TOKEN);
            return NULL;
        }

        case TOKEN_INT: {
            value->type = FLD_VALUE_INT;
            value->integer = parser->current->value.integer;
            _parser_advance(parser);
            return value;
        }

        case TOKEN_FLOAT: {
            value->type = FLD_VALUE_FLOAT;
            value->float_val = parser->current->value.float_val;
            _parser_advance(parser);
            return value;
        }

        case TOKEN_BOOL: {
            value->type = FLD_VALUE_BOOL;
            value->boolean = parser->current->value.boolean;
            _parser_advance(parser);
            return value;
        }

        default: {
            _parser_error(parser, FLD_ERROR_UNEXPECTED_TOKEN);
            return NULL;
        }
    }

    // Should never reach here
    return NULL;
}

static fld_object *_parse_field(fld_parser *parser) {
    // Allocate new object
    fld_object *obj = (fld_object*)_bump_alloc(&parser->allocator, sizeof(fld_object), ALIGNOF(fld_object));
    if (!obj) {
        parser->last_error.code = FLD_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    // Null initialize links
    obj->next = NULL;
    obj->parent = NULL;

    // Store the key
    obj->key = parser->current->value.string;

    // Consume key
    _parser_advance(parser);
    
    // Expect the equals sign
    _parser_consume(parser, TOKEN_EQUALS, FLD_ERROR_UNEXPECTED_TOKEN);
    if (parser->last_error.code != FLD_ERROR_NONE) {
        return NULL;
    }

    // Now parse the value
    fld_value *value = _parse_value(parser);
    if (!value) {
        // TODO: error?
        return NULL;
    }

    // Store the value
    obj->value = *value;

    // Expect the semicolon
    _parser_consume(parser, TOKEN_SEMICOLON, FLD_ERROR_UNEXPECTED_TOKEN);
    if (parser->last_error.code != FLD_ERROR_NONE) {
        return NULL;
    }

    return obj;
}

bool fld_parse(fld_parser *parser, const char *source, void *memory, size_t size) {
    // Set up parser
    parser->root = NULL;
    parser->last_error.code = FLD_ERROR_NONE;
    parser->last_error.line = 1;
    parser->last_error.column = 1;
    
    // Calculate memory needed and if enought was passed in or not...
    size_t string_length;
    size_t needed = _estimate_memory_needed(source, &string_length);
    if (size < needed) {
        parser->last_error.code = FLD_ERROR_INSUFFICIENT_MEMORY;
        return false;
    }

    // Init bump allocator
    _bump_init(&parser->allocator, memory, size);

    // Copy source text so it does not get invalidated
    parser->source = (char*)_bump_alloc(&parser->allocator, string_length + 1, ALIGNOF(char));
    if (!parser->source) {
        parser->last_error.code = FLD_ERROR_OUT_OF_MEMORY;
        return false;
    }

    memcpy(parser->source, source, string_length + 1);
    // Null terminate it
    parser->source[string_length] = '\0';

    // Set up the lexer
    parser->lexer.start = parser->source;
    parser->lexer.current = parser->source;
    parser->lexer.line = 1;
    parser->lexer.column = 1;

    // Get the first token
    parser->current = _lexer_scan_token(parser);
    parser->previous = NULL;

    fld_object *last = NULL;

    // Parse all top-level fields
    while (parser->current->type != TOKEN_EOF) {
        // Everything starts with a key...
        if (parser->current->type != TOKEN_KEY) {
            parser->last_error.code = FLD_ERROR_UNEXPECTED_TOKEN;
            parser->last_error.line = parser->current->line;
            parser->last_error.column = parser->current->column;
            
            return false;
        }

        fld_object *field = _parse_field(parser);
        if (!field) {
            // TODO: error?
            return false;
        }

        // If this is the first, make it root
        if (!parser->root) {
            parser->root = field;
        } else {
            last->next = field;
        }

        last = field;
    }

    return parser->last_error.code == FLD_ERROR_NONE;
}

fld_object *fld_get_field(fld_object *object, const char *key) {
    fld_object *current = object;
    int key_len = strlen(key);

    while (current) {
        if (current->key.length == key_len &&
            memcmp(current->key.start, key, key_len) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

fld_object *fld_get_field_by_path(fld_object *object, const char *path) {
    // Empty or NULL path
    if (!path || !*path) {
        return NULL;
    }

    // Make a copy of the path
    char path_copy[FLD_MAX_PATH_LENGTH];
    strcpy(path_copy, path);

    fld_object *current = object;
    char *token = strtok(path_copy, ".");
    char *next_token;

    while (token) {
        // Get the next token to see if we're at the final part
        next_token = strtok(NULL, ".");

        // Find the field
        fld_object *field = fld_get_field(current, token);
        if (!field) {
            return NULL;
        }

        if (next_token) {
            // If there are more tokens, this must be an object
            if (field->value.type != FLD_VALUE_OBJECT) {
                return NULL;
            }
            current = field->value.object;
        } else {
            // Last token - this is our target field
            fld_object *result = field;
            return result;
        }
        token = next_token;
    }

    return NULL;
}

bool fld_get_str_view(fld_object *object, const char *key, fld_string_view *str_view) {
    fld_object *field = fld_get_field_by_path(object, key);
    if (!field || field->value.type != FLD_VALUE_STRING) {
        return false;
    }
    str_view->start = field->value.string.start;
    str_view->length = field->value.string.length;
    return true;
}

bool fld_get_cstr(fld_object *object, const char *key, char *buffer, size_t buffer_size) {
    fld_string_view str_view;

    // Validate input
    if (!buffer || buffer_size == 0) return false;
    
    // Try to get string view first
    if (!fld_get_str_view(object, key, &str_view)) {
        // Null terminate on failure
        buffer[0] = '\0';
        return false;
    }

    // Check if buffer is large enough plus null terminator
    if (str_view.length >= buffer_size) {
        buffer[0] = '\0';
        return false;
    }

    memcpy(buffer, str_view.start, str_view.length);
    buffer[str_view.length] = '\0';
    return true;
}

bool fld_get_int(fld_object *object, const char *key, int *out_value) {
    fld_object *field = fld_get_field_by_path(object, key);
    if (!field || field->value.type != FLD_VALUE_INT) {
        return false;
    }
    *out_value = field->value.integer;
    return true;
}

bool fld_get_float(fld_object *object, const char *key, float *out_value) {
    fld_object *field = fld_get_field_by_path(object, key);
    if (!field || field->value.type != FLD_VALUE_FLOAT) {
        return false;
    }
    *out_value = field->value.float_val;
    return true;
}

bool fld_get_bool(fld_object *object, const char *key, bool *out_value) {
    fld_object *field = fld_get_field_by_path(object, key);
    if (!field || field->value.type != FLD_VALUE_BOOL) {
        return false;
    }
    *out_value = field->value.boolean;
    return true;
}

bool fld_get_array(fld_object *object, const char *key, fld_value_type *out_type, void** out_items, size_t* out_count) {
    fld_object* field = fld_get_field_by_path(object, key);
    if (!field || field->value.type != FLD_VALUE_ARRAY) {
        return false;
    }
    *out_type = field->value.array.type;
    *out_items = field->value.array.items;
    *out_count = field->value.array.count;
    return true;
}

bool fld_get_object(fld_object *object, const char *key, fld_object **out_object) {
    fld_object* field = fld_get_field_by_path(object, key);
    if (!field || field->value.type != FLD_VALUE_OBJECT) {
        return false;
    }
    *out_object = field->value.object;
    return true;
}

bool fld_has_field(fld_object *object, const char *key) {
    return fld_get_field_by_path(object, key) != NULL;
}

fld_value_type fld_get_type(fld_object *object, const char *key) {
    fld_object *field = fld_get_field_by_path(object, key);
    if (!field) {
        return FLD_VALUE_EMPTY;
    }
    return field->value.type;
}

bool fld_get_array_size(fld_object *object, const char *key, size_t *out_size) {
    fld_object *field = fld_get_field_by_path(object, key);
    if (!field || field->value.type != FLD_VALUE_ARRAY) {
        return false;
    }
    *out_size = field->value.array.count;
    return true;
}

bool fld_string_view_to_cstr(fld_string_view str_view, char *buffer, size_t buffer_size) {
    if (buffer_size <= str_view.length) return false;
    memcpy(buffer, str_view.start, str_view.length);
    buffer[str_view.length] = '\0';
    return true;
}

bool fld_string_view_eq(fld_string_view str_view, const char* cstr) {
    size_t cstr_len = strlen(cstr);
    if (cstr_len != str_view.length) return false;
    return memcmp(str_view.start, cstr, str_view.length) == 0;
}

size_t fld_estimate_memory(const char *source) {
    size_t len;
    return _estimate_memory_needed(source, &len);
}

fld_object *fld_iter_next(fld_iterator *iter) {
    if (!iter->current) return NULL;

    fld_object *result = iter->current;

    // For recursive, try to go deeper if we have an object
    if (iter->type == FLD_ITER_RECURSIVE && result->value.type == FLD_VALUE_OBJECT && result->value.object) {
        // Set current as parent and descend
        iter->parent = result;
        iter->current = result->value.object;
        iter->depth++;
        return result;
    }

    // Move to the next sibling
    if (iter->current->next) {
        iter->current = iter->current->next;
        return result;
    }

    // For recursive iter, go back up if we can't move to siblings
    if (iter->type == FLD_ITER_RECURSIVE && iter->parent) {
        iter->current = iter->parent->next;
        iter->parent = iter->parent->parent;
        iter->depth--;
        return result;
    }

    // No more nodes to visit
    iter->current = NULL;
    return result;
}

#endif // FLD_PARSER_IMPLEMENTATION
#endif // FLD_PARSER_H

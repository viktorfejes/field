/*
*   fld_parser - v0.60
*   Header-only library for parsing configuration files in the FLD format.
*
*   RECENT CHANGES:
*       0.60    (2025-01-12)    Added vec2, vec3, vec4 (float) support;
*                               Vectors are added to examples and tests;
*                               Updated readme with description and help for new vec types;
*       0.52    (2025-01-11)    Changed `key` argument in the appropriate functions to `path`;
*       0.51    (2025-01-11)    Fixed a bug in the implementation of _fast_atoi;
*                               Parser now errors out if a number is too big;
*       0.50    (2025-01-11)    Removed stdlib.h by implementing _fast_atof and _fast_atoi;
*                               Fixed number handling to handle negative numbers and + prefixed positive numbers;
*       0.41    (2025-01-10)    Fixed some bugs where proper parent relationship was not established;
*                               Updated readme with new functions and explanations;
*                               Added a function to be able to get path from iterator;
*                               Fixed a bug with the iterator where it skipped the first object;
*                               Changed anonymus union in `fld_value` to `as`;
*                               Added proper tests in the tests/tests.c;
*                               Created a quite comprehensive example file in examples/main_example.c;
*       0.40    (2025-01-10)    Added iterator support to the parser;
*       0.30    (2025-01-10)    Renamed `fld_get_string` to `fld_get_str_view`,
*                               Added `fld_get_cstr` for immediate c string retrival,
*                               Added `fld_has field`, `fld_get_type`, `fld_get_array_size`,
*                               `fld_string_view_to_cstr`, `fld_string_view_eq`,
*                               `fld_get_last_error`, `fld_error_string`, and `fld_estimate_memory`;
*       0.21    (2025-01-10)    Removed stddef.h that wasn't needed anymore;
*       0.20    (2025-01-10)    Removed some remaining temp code,
*                               Added block comment support,
*                               Updated readme and `test.c` to reflect changes;
*       0.10    (2025-01-09)    Finalized first implementation;
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
*       - [ ] Run some tests to see how close the
*             memory estimating function gets.
*       - [x] Remove stdlib.h include by writing own
*             int and float parser.
*       - [ ] Full documentation of public API
*       - [x] Change key to path in getters to make more sense
*       - [x] Add digit checking for lexer's number handling
*       - [ ] Eliminate all compilation warnings.
*       - [ ] Consider allowing boolean for vectors, as well.
*
 */

#ifndef FLD_PARSER_H
#define FLD_PARSER_H

#include <stdint.h>
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
    FLD_VALUE_VEC2,
    FLD_VALUE_VEC3,
    FLD_VALUE_VEC4,
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
        struct {
            float x, y;
        } vec2;
        struct {
            float x, y, z;
        } vec3;
        struct {
            float x, y, z, w;
        } vec4;
    } as;
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
    TOKEN_PAREN_LEFT,
    TOKEN_PAREN_RIGHT,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_VEC,
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
#define FLD_MAX_DIGITS 21

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

/**
 * @brief Retrieves a string view associated with a given path starting
 * from the specified fld_object.
 * 
 * @param object Pointer to the object to start the search from.
 * @param path The path associated with the desired string view.
 * @param str_view Pointer to an fld_string_view structure where the result will be stored.
 * @return true if the string view is successfully retrieved, false otherwise.
 */
extern bool fld_get_str_view(fld_object *object, const char *path, fld_string_view *str_view);

/**
 * @brief Retrieves a C-string value associated with a given path starting from a field object.
 *
 * This function searches for a path starting from  the specified field object and, if found,
 * copies the associated C-string value into the provided buffer. The buffer size
 * must be large enough to hold the value, including the null-terminator.
 *
 * @param object Pointer to the object to start the search from.
 * @param path The path associated with the desired C-string value.
 * @param buffer Pointer to the buffer where the retrieved C-string value will be stored.
 * @param buffer_size The size of the buffer, in bytes.
 * @return true if the object was found and the value was successfully copied to the buffer; 
 *         false otherwise.
 */
extern bool fld_get_cstr(fld_object *object, const char *path, char *buffer, size_t buffer_size);

/**
 * @brief Retrieves an integer value from an object at a given path.
 *
 * This function searches for the specified path starting from the given field object and,
 * if found, assigns the corresponding integer value to the output parameter.
 *
 * @param object Pointer to the object to start the search from.
 * @param path The path associated with the desired integer value.
 * @param out_value A pointer to an integer where the retrieved value will be stored.
 * @return true if the path was found and the value was successfully retrieved, false otherwise.
 */
extern bool fld_get_int(fld_object *object, const char *path, int *out_value);

/**
 * @brief Retrieves a float value from a field object based on the provided path.
 *
 * This function searches for the specified path starting from the given object and,
 * if found, assigns the corresponding float value to the output parameter.
 *
 * @param object Pointer to the object to start the search from.
 * @param path The path associated with the desired float value.
 * @param out_value A pointer to a float where the retrieved value will be stored.
 * @return true if the object was found and the value was successfully retrieved, false otherwise.
 */
extern bool fld_get_float(fld_object *object, const char *path, float *out_value);

/**
 * @brief Retrieves a bool value from a field object based on the provided path.
 *
 * This function searches for an object with a specified path starting from
 * the given field object and, if found, assigns the corresponding bool value
 * to the output parameter.
 *
 * @param object Pointer to the object to start the search from.
 * @param path The path associated with the desired bool value.
 * @param out_value A pointer to a bool where the retrieved value will be stored.
 * @return true if the object was found and the value was successfully retrieved, false otherwise.
 */
extern bool fld_get_bool(fld_object *object, const char *path, bool *out_value);


/**
 * @brief Retrieves an array from a field object based on a path.
 *
 * This function searches for an array associated with the specified path starting from the given field object.
 * If the key is found and the associated value is an array, assigns the array's type, items,
 * and count through the output parameters.
 *
 * @param object Pointer to the object to start the search from.
 * @param path The path associated with the array to retrieve.
 * @param out_type Pointer to a variable to receive the type of the array elements.
 * @param out_items Pointer to a variable to receive the pointer to the array items.
 * @param out_count Pointer to a variable to receive the count of items in the array.
 * @return true if the array is found and successfully retrieved, false otherwise.
 */
extern bool fld_get_array(fld_object *object, const char *path, fld_value_type *out_type, void** out_items, size_t* out_count);

/**
 * @brief Retrieves a vec2 value from a field object based on the provided path.
 *
 * @param object Pointer to the object to start the search from.
 * @param path The path associated with the desired vec2 value.
 * @param out_x A pointer to store the x component.
 * @param out_y A pointer to store the y component.
 * @return true if the object was found and the value was successfully retrieved, false otherwise.
 */
extern bool fld_get_vec2(fld_object *object, const char *path, float *out_x, float *out_y);

/**
 * @brief Retrieves a vec3 value from a field object based on the provided path.
 *
 * @param object Pointer to the object to start the search from.
 * @param path The path associated with the desired vec3 value.
 * @param out_x A pointer to store the x component.
 * @param out_y A pointer to store the y component.
 * @param out_z A pointer to store the z component.
 * @return true if the object was found and the value was successfully retrieved, false otherwise.
 */
extern bool fld_get_vec3(fld_object *object, const char *path, float *out_x, float *out_y, float *out_z);

/**
 * @brief Retrieves a vec4 value from a field object based on the provided path.
 *
 * @param object Pointer to the object to start the search from.
 * @param path The path associated with the desired vec4 value.
 * @param out_x A pointer to store the x component.
 * @param out_y A pointer to store the y component.
 * @param out_z A pointer to store the z component.
 * @param out_w A pointer to store the w component.
 * @return true if the object was found and the value was successfully retrieved, false otherwise.
 */
extern bool fld_get_vec4(fld_object *object, const char *path, float *out_x, float *out_y, float *out_z, float *out_w);

/**
 * @brief Retrieves a vector's components as an array of floats.
 *
 * @param object Pointer to the object to start the search from.
 * @param path The path associated with the desired vector.
 * @param out_components Buffer to store the components (must be large enough).
 * @param out_count Number of components written (2, 3, or 4).
 * @return true if the object was found and components were retrieved, false otherwise.
 */
extern bool fld_get_vec_components(fld_object *object, const char *path, float *out_components, size_t* out_count);

/**
 * @brief Retrieves an object based on a specified path.
 *
 * This function searches for an object starting from the given field object using the provided path.
 * If the object is found, it is returned through the out_object parameter.
 *
 * @param object Pointer to the object to start the search from.
 * @param path The path to search for within the field object.
 * @param out_object A pointer to a pointer where the found object will be stored.
 * @return true if the object is found and retrieved successfully, false otherwise.
 */
extern bool fld_get_object(fld_object *object, const char *path, fld_object **out_object);

/**
 * @brief Retrieves an object based on a given key on the same level as given object.
 *
 * This function searches for an object within the given field object using the provided key.
 * It only searches horizontally, staying at the same level, without going deeper.
 *
 * @param object The field object to search within.
 * @param key The key to search for within the field object.
 * @param out_object A pointer to a pointer where the found object will be stored.
 * @return true if the object is found and retrieved successfully, false otherwise.
 */
extern fld_object *fld_get_field(fld_object *object, const char *key);
extern fld_object *fld_get_field_by_path(fld_object *object, const char *path);

extern bool fld_has_field(fld_object *object, const char *path);
extern fld_value_type fld_get_type(fld_object *object, const char *path);
extern bool fld_get_array_size(fld_object *object, const char *path, size_t *out_size);

/**
 * @brief Converts a fld_string_view to a null-terminated C string.
 *
 * This function takes a fld_string_view and copies its content into the provided buffer,
 * ensuring that the result is null-terminated. The buffer must be large enough to hold
 * the string view content and the null terminator.
 *
 * @param str_view The fld_string_view to be converted.
 * @param buffer The buffer where the null-terminated C string will be stored.
 * @param buffer_size The size of the buffer.
 * @return true if the conversion was successful and the buffer was large enough,
 *         false if the buffer was too small to hold the string view content and the null terminator.
 */
extern bool fld_string_view_to_cstr(fld_string_view str_view, char *buffer, size_t buffer_size);

/**
 * @brief Compares a fld_string_view with a C-style string for equality.
 * 
 * This function checks if the given fld_string_view is equal to the provided
 * null-terminated C-style string. The comparison is case-sensitive.
 * 
 * @param str_view The fld_string_view to compare.
 * @param cstr The null-terminated C-style string to compare against.
 * @return true if the fld_string_view and the C-style string are equal, false otherwise.
 */
extern bool fld_string_view_eq(fld_string_view str_view, const char* cstr);

/**
 * @brief Estimates the memory required to parse the given source string.
 *
 * This function analyzes the provided source string and returns the estimated
 * amount of memory (in bytes) that would be required to parse it.
 *
 * @param source A pointer to the source string to be analyzed.
 * @return The estimated memory size in bytes required for parsing the source string.
 */
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
 * @brief Retrieves the path from the field iterator.
 *
 * This function extracts the path from the given field iterator and stores it
 * in the provided buffer. The buffer size must be sufficient to hold the path.
 *
 * @param iter Pointer to the field iterator.
 * @param buffer Pointer to the buffer where the path will be stored.
 * @param buffer_size Size of the buffer.
 * @return true if the path was successfully retrieved and stored in the buffer,
 *         false otherwise.
 */
extern bool fld_iter_get_path(fld_iterator *iter, char *buffer, size_t buffer_size);

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
    iter->type = type;
    iter->current = root;
    iter->depth = -1;
    iter->parent = NULL;
}

/**
 * @brief Retrieves the last error encountered by the parser.
 *
 * This function returns the last error that was recorded in the given parser.
 *
 * @param parser A pointer to the fld_parser structure.
 * @return The last error encountered by the parser.
 */
static inline fld_error fld_get_last_error(fld_parser *parser) {
    return parser->last_error;
}

/**
 * @brief Converts a field error code to a human-readable string.
 *
 * This function takes a field error code and returns a constant string
 * describing the error. It is useful for logging and debugging purposes.
 *
 * @param code The field error code to convert.
 * @return A constant string describing the error.
 */
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
static fld_value *_parse_object(fld_parser *parser, fld_object *parent);
static fld_value *_parse_value(fld_parser *parser, fld_object *parent);
static fld_object *_parse_field(fld_parser *parser, fld_object *parent);
static fld_value *_parse_vec(fld_parser *parser, fld_object *parent);

static inline void _bump_init(fld_bump_allocator *alloc, void *memory, size_t size) {
    alloc->start = (uint8_t*)memory;
    alloc->current = alloc->start;
    alloc->end = alloc->start + size;
    // TODO: maybe zero out the whole block of memory?
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

// Only handles only digit positive numbers
static int _fast_atoi(const char *str) {
    int res = *str - '0';
    ++str;
    while (*str) {
        res = res * 10 + (*str++ - '0');
    }
    return res;
}

// Only sign and digits, no white-spaces
static double _fast_atof(const char* str) {
    double value = 0;
    while (*str != '.') {
        value = value * 10 + (*str++ - '0');
        if (!*str) return value;  // No decimal point
    }
    double factor = 0.1;
    while (*++str) {
        value += (*str - '0') * factor;
        factor *= 0.1;
    }
    return value;
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

static fld_token *_lexer_handle_number(fld_parser *parser, fld_lexer *lexer, bool is_negative) {
    // TODO: Add error for too long/large numbers
    
    // A bit larger than max digits for null terminator and safety
    char num_str[FLD_MAX_DIGITS + 3];
    char *num_ptr = num_str;
    uint8_t digit_count = 0;

    // Skip the sign in input but don't copy it
    if (*lexer->current == '+' || *lexer->current == '-') {
        _lexer_advance(lexer);
    }

    // Conver bool to 1/-1 for multiplication without branching
    int sign = 1 - (2 * is_negative);
    fld_token *token;

    // As long as we are encountering digits, let's advance
    while (_is_digit(_lexer_peek(lexer))) {
        *num_ptr++ = _lexer_advance(lexer);
        digit_count++;
    }

    // Look for decimal point followed by numbers
    if (_lexer_peek(lexer) == '.' && _is_digit(_lexer_peek_next(lexer))) {
        // Consume the '.'
        *num_ptr++ = _lexer_advance(lexer);

        // Continue with the remaining digits
        while (_is_digit(_lexer_peek(lexer)) && digit_count < FLD_MAX_DIGITS) {
            *num_ptr++ = _lexer_advance(lexer);
            digit_count++;
        }

        // Check if we hit the digit limit, and if we have more
        if (_is_digit(_lexer_peek(lexer))) {
            parser->last_error.code = FLD_ERROR_INVALID_NUMBER;
            parser->last_error.line = lexer->line;
            parser->last_error.column = lexer->column;
            return _token_create(parser, TOKEN_ERROR, lexer->line, lexer->column);
        }

        *num_ptr = '\0';
        
        token = _token_create(parser, TOKEN_FLOAT, lexer->line, lexer->column);
        token->value.float_val = _fast_atof(num_str) * sign;
    } else {
        if (_is_digit(_lexer_peek(lexer))) {
            parser->last_error.code = FLD_ERROR_INVALID_NUMBER;
            parser->last_error.line = lexer->line;
            parser->last_error.column = lexer->column;
            return _token_create(parser, TOKEN_ERROR, lexer->line, lexer->column);
        }

        *num_ptr = '\0';
        token = _token_create(parser, TOKEN_INT, lexer->line, lexer->column);
        token->value.integer = _fast_atoi(num_str) * sign;
    }
    return token;
}

static fld_token *_lexer_handle_keyword(fld_parser *parser, fld_lexer *lexer) {
    while (_is_alpha(_lexer_peek(lexer)) || _is_digit(_lexer_peek(lexer))) {
        _lexer_advance(lexer);
    }

    // Check if it's a keyword (true/false)
    int length = lexer->current - lexer->start;
    if (length == 4) {
        // false
        // TODO: Make it case insensitive!
        if (strncmp(lexer->start, "true", 4) == 0) {
            fld_token *token = _token_create(parser, TOKEN_BOOL, lexer->line, lexer->column);
            token->value.boolean = true;
            return token;
        }
        // We just check if it starts with Vec and according to the number
        // that follows we emit the right token. If something is fishy, we error.
        if (strncmp(lexer->start, "vec", 3) == 0) {
            // If it's not 2, 3, 4 -> error!
            if (lexer->start[3] > '4' || lexer->start[3] < '2') {
                return _token_create(parser, TOKEN_ERROR, lexer->line, lexer->column);
            }

            // Emit Vec token where the integer value contains the supposed size
            fld_token *token = _token_create(parser, TOKEN_VEC, lexer->line, lexer->column);
            token->value.integer = lexer->start[3] - 48;
            return token;
        }
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
    if (_is_digit(c) || (c == '-' && _is_digit(_lexer_peek(lexer)))) {
        // Back up so number() sees the first digit
        lexer->current--;
        bool is_negative = (*lexer->current == '-');
        return _lexer_handle_number(parser, lexer, is_negative);
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
        case '(': return _token_create(parser, TOKEN_PAREN_LEFT, lexer->line, lexer->column - 1);
        case ')': return _token_create(parser, TOKEN_PAREN_RIGHT, lexer->line, lexer->column - 1);
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

static fld_object *_parse_object_fields(fld_parser *parser, fld_object *parent) {
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
        fld_object *field = _parse_field(parser, parent);
        if (!field) {
            return NULL;
        }

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

    last_field->next = NULL;
    return first_field;
}

static fld_value *_parse_object(fld_parser *parser, fld_object *parent) {
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
    value->as.object = _parse_object_fields(parser, parent);
    if (parser->last_error.code != FLD_ERROR_NONE) {
        return NULL;
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

static fld_value *_parse_array(fld_parser *parser, fld_object *parent) {
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
        array->as.array.count = 0;
        array->as.array.items = NULL;
        array->as.array.type = FLD_VALUE_EMPTY;
        return array;
    }

    // Parse first value to get type
    fld_value *first_value = _parse_value(parser, parent);
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
    array->as.array.type = first_value->type;
    array->as.array.count = count;

    // Allocate storage for all items
    size_t item_size = _get_type_size(array->as.array.type);
    size_t item_align = _get_type_alignment(array->as.array.type);

    void* items = _bump_alloc(&parser->allocator, item_size * count, item_align);
    if (!items) {
        _parser_error(parser, FLD_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    array->as.array.items = items;

    // Second pass - parse and store values
    char* current = (char*)items;

    for (size_t i = 0; i < count; i++) {
        fld_value *value = _parse_value(parser, parent);
        if (!value) return NULL;

        if (value->type != array->as.array.type) {
            _parser_error(parser, FLD_ERROR_ARRAY_TYPE_MISMATCH);
            return NULL;
        }

        // Copy value based on type
        switch (array->as.array.type) {
            case FLD_VALUE_STRING:
                *((fld_string_view*)current) = value->as.string;
                break;
            case FLD_VALUE_INT:
                *((int*)current) = value->as.integer;
                break;
            case FLD_VALUE_FLOAT:
                *((float*)current) = value->as.float_val;
                break;
            case FLD_VALUE_BOOL:
                *((bool*)current) = value->as.boolean;
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

static fld_value *_parse_vec(fld_parser *parser, fld_object *parent) {
    // Get vector size from keyword token
    int vec_size = parser->current->value.integer;

    // Consume vec token
    _parser_advance(parser);

    // We expect opening parenthesis here
    if(!_parser_expect(parser, TOKEN_PAREN_LEFT, FLD_ERROR_UNEXPECTED_TOKEN)) {
        return NULL;
    }

    // Allocate value
    fld_value *value = (fld_value*)_bump_alloc(&parser->allocator, sizeof(fld_value), ALIGNOF(fld_value));
    if (!value) {
        _parser_error(parser, FLD_ERROR_OUT_OF_MEMORY);
    }
    memset(value, 0, sizeof(fld_value));

    // Set appropriate vector type based on size
    value->type = (fld_value_type)(FLD_VALUE_VEC2 + vec_size - 2);

    // Parse the components (they can only be float!)
    // Here, we are casting to the first vec size in the union
    // so we can get all the others from this address
    float *components = (float*)&value->as.vec2;
    for (int i = 0; i < vec_size; ++i) {
        // Check for comma between components (except first)
        if (i > 0 && !_parser_expect(parser, TOKEN_COMMA, FLD_ERROR_UNEXPECTED_TOKEN)) {
            return NULL;
        }

        // Each component must be a number (int or float)
        if (parser->current->type != TOKEN_INT && parser->current->type != TOKEN_FLOAT) {
            _parser_error(parser, FLD_ERROR_UNEXPECTED_TOKEN);
            return NULL;
        }

        // Store the component value (convert to float from int if needed)
        components[i] = (parser->current->type == TOKEN_INT)
            ? (float)parser->current->value.integer
            : (float)parser->current->value.float_val;

        _parser_advance(parser);
    }

    // Expect closing parenthesis
    if (!_parser_expect(parser, TOKEN_PAREN_RIGHT, FLD_ERROR_UNEXPECTED_TOKEN)) {
        return NULL;
    }

    return value;
}

static fld_value *_parse_value(fld_parser *parser, fld_object *parent) {
    // For objects and arrays we'll just use their appropriate parse functions
    if (parser->current->type == TOKEN_BRACE_LEFT) {
        return _parse_object(parser, parent);
    }
    if (parser->current->type == TOKEN_BRACKET_LEFT) {
        return _parse_array(parser, parent);
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
                value->as.string = parser->current->value.string;
                _parser_advance(parser);
                return value;
            }
            _parser_error(parser, FLD_ERROR_UNEXPECTED_TOKEN);
            return NULL;
        }

        case TOKEN_INT: {
            value->type = FLD_VALUE_INT;
            value->as.integer = parser->current->value.integer;
            _parser_advance(parser);
            return value;
        }

        case TOKEN_FLOAT: {
            value->type = FLD_VALUE_FLOAT;
            value->as.float_val = parser->current->value.float_val;
            _parser_advance(parser);
            return value;
        }

        case TOKEN_BOOL: {
            value->type = FLD_VALUE_BOOL;
            value->as.boolean = parser->current->value.boolean;
            _parser_advance(parser);
            return value;
        }

        case TOKEN_VEC: {
            return _parse_vec(parser, parent);
        }

        default: {
            _parser_error(parser, FLD_ERROR_UNEXPECTED_TOKEN);
            return NULL;
        }
    }

    // Should never reach here
    return NULL;
}

static fld_object *_parse_field(fld_parser *parser, fld_object *parent) {
    // Allocate new object
    fld_object *obj = (fld_object*)_bump_alloc(&parser->allocator, sizeof(fld_object), ALIGNOF(fld_object));
    if (!obj) {
        parser->last_error.code = FLD_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    memset(obj, 0, sizeof(fld_object));

    // Null initialize links
    obj->next = NULL;
    obj->parent = parent;

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
    fld_value *value = _parse_value(parser, obj);
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

        // Parse field -- top level has no parent (like batman)
        fld_object *field = _parse_field(parser, NULL);
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

fld_object *fld_get_field(fld_object *object, const char *path) {
    fld_object *current = object;
    int key_len = strlen(path);

    while (current) {
        if (current->key.length == key_len &&
            memcmp(current->key.start, path, key_len) == 0) {
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
            current = field->value.as.object;
        } else {
            // Last token - this is our target field
            fld_object *result = field;
            return result;
        }
        token = next_token;
    }

    return NULL;
}

bool fld_get_str_view(fld_object *object, const char *path, fld_string_view *str_view) {
    fld_object *field = fld_get_field_by_path(object, path);
    if (!field || field->value.type != FLD_VALUE_STRING) {
        return false;
    }
    str_view->start = field->value.as.string.start;
    str_view->length = field->value.as.string.length;
    return true;
}

bool fld_get_cstr(fld_object *object, const char *path, char *buffer, size_t buffer_size) {
    fld_string_view str_view;

    // Validate input
    if (!buffer || buffer_size == 0) return false;
    
    // Try to get string view first
    if (!fld_get_str_view(object, path, &str_view)) {
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

bool fld_get_int(fld_object *object, const char *path, int *out_value) {
    fld_object *field = fld_get_field_by_path(object, path);
    if (!field || field->value.type != FLD_VALUE_INT) {
        return false;
    }
    *out_value = field->value.as.integer;
    return true;
}

bool fld_get_float(fld_object *object, const char *path, float *out_value) {
    fld_object *field = fld_get_field_by_path(object, path);
    if (!field || field->value.type != FLD_VALUE_FLOAT) {
        return false;
    }
    *out_value = field->value.as.float_val;
    return true;
}

bool fld_get_bool(fld_object *object, const char *path, bool *out_value) {
    fld_object *field = fld_get_field_by_path(object, path);
    if (!field || field->value.type != FLD_VALUE_BOOL) {
        return false;
    }
    *out_value = field->value.as.boolean;
    return true;
}

bool fld_get_array(fld_object *object, const char *path, fld_value_type *out_type, void** out_items, size_t* out_count) {
    fld_object* field = fld_get_field_by_path(object, path);
    if (!field || field->value.type != FLD_VALUE_ARRAY) {
        return false;
    }
    *out_type = field->value.as.array.type;
    *out_items = field->value.as.array.items;
    *out_count = field->value.as.array.count;
    return true;
}

bool fld_get_vec2(fld_object *object, const char *path, float *out_x, float *out_y) {
    fld_object *field = fld_get_field_by_path(object, path);
    if (!field || field->value.type != FLD_VALUE_VEC2) {
        return false;
    }
    *out_x = field->value.as.vec2.x;
    *out_y = field->value.as.vec2.y;
    return true;
}

bool fld_get_vec3(fld_object *object, const char *path, float *out_x, float *out_y, float *out_z) {
    fld_object *field = fld_get_field_by_path(object, path);
    if (!field || field->value.type != FLD_VALUE_VEC3) {
        return false;
    }
    *out_x = field->value.as.vec3.x;
    *out_y = field->value.as.vec3.y;
    *out_z = field->value.as.vec3.z;
    return true;
}

bool fld_get_vec4(fld_object *object, const char *path, float *out_x, float *out_y, float *out_z, float *out_w) {
    fld_object *field = fld_get_field_by_path(object, path);
    if (!field || field->value.type != FLD_VALUE_VEC4) {
        return false;
    }
    *out_x = field->value.as.vec4.x;
    *out_y = field->value.as.vec4.y;
    *out_z = field->value.as.vec4.z;
    *out_w = field->value.as.vec4.w;
    return true;
}

bool fld_get_vec_components(fld_object *object, const char *path, float *out_components, size_t* out_count) {
    fld_object* field = fld_get_field_by_path(object, path);
    if (!field || field->value.type < FLD_VALUE_VEC2 || field->value.type > FLD_VALUE_VEC4) {
        return false;
    }

    // Calculate component count from type
    *out_count = (field->value.type - FLD_VALUE_VEC2) + 2;

    // Copy components using the fact that vectors are contiguous in memory
    const float *src = (const float*)&field->value.as.vec2;
    memcpy(out_components, src, *out_count * sizeof(float));
    return true;
}

bool fld_get_object(fld_object *object, const char *path, fld_object **out_object) {
    fld_object* field = fld_get_field_by_path(object, path);
    if (!field || field->value.type != FLD_VALUE_OBJECT) {
        return false;
    }
    *out_object = field->value.as.object;
    return true;
}

bool fld_has_field(fld_object *object, const char *path) {
    return fld_get_field_by_path(object, path) != NULL;
}

fld_value_type fld_get_type(fld_object *object, const char *path) {
    fld_object *field = fld_get_field_by_path(object, path);
    if (!field) {
        return FLD_VALUE_EMPTY;
    }
    return field->value.type;
}

bool fld_get_array_size(fld_object *object, const char *path, size_t *out_size) {
    fld_object *field = fld_get_field_by_path(object, path);
    if (!field || field->value.type != FLD_VALUE_ARRAY) {
        return false;
    }
    *out_size = field->value.as.array.count;
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

    // For the initial case
    // Alternatively, I could create a pre_root in the struct
    // with its .next = root...
    if (iter->depth == -1) {
        iter->depth = 0;
        return iter->current;
    }

    // For recursive, try to go deeper if we have an object
    if (iter->type == FLD_ITER_RECURSIVE && iter->current->value.type == FLD_VALUE_OBJECT && iter->current->value.as.object) {
        // Set current as parent and descend
        iter->parent = iter->current;
        iter->current = iter->current->value.as.object;
        iter->depth++;
        return iter->current;
    }

    // Move to the next sibling
    if (iter->current->next) {
        iter->current = iter->current->next;
        return iter->current;
    }

    // For recursive iter, go back up if we can't move to siblings
    if (iter->type == FLD_ITER_RECURSIVE && iter->parent) {
        iter->current = iter->parent->next;
        iter->parent = iter->parent->parent;
        iter->depth--;
        return iter->current;
    }

    // No more nodes to visit
    iter->current = NULL;
    return NULL;  // Return NULL instead of the last node
}

bool fld_iter_get_path(fld_iterator *iter, char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size <= 0) return false;

    buffer[0] = '\0';
    size_t pos = 0;

    // Build path from root to current
    fld_object *obj = iter->current;
    // Max depth 32, right now
    fld_object *parents[32];
    int depth = 0;

    // Collect parents
    while (obj && depth < 32) {
        parents[depth++] = obj;
        obj = obj->parent;
    }

    // Build path from root to leaf
    for (int i = depth - 1; i >= 0; --i) {
        // Add separator, except for first element
        if (buffer[0] != '\0') {
            if (pos + 1 >= buffer_size) return false;
            buffer[pos++] = '.';
        }
        // Add field name
        size_t name_len = parents[i]->key.length;
        if (pos + name_len >= buffer_size) return false;

        memcpy(buffer + pos, parents[i]->key.start, name_len);
        pos += name_len;
        buffer[pos] = '\0';
    }
    return true;
}

#endif // FLD_PARSER_IMPLEMENTATION
#endif // FLD_PARSER_H

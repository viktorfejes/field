#include "parser.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PARSER_MEMORY_SIZE (1024 * 1024)
static uint8_t parser_memory[PARSER_MEMORY_SIZE];
static bump_allocator_t parser_allocator;

// Forward declarations
static value_t *create_value(value_type_t type);
static value_t *create_string_value(token_t *token);
static value_t *create_object_value(void);
static void object_add(value_t *object, const char *key, int key_length, value_t *value);
static void free_value(value_t *value);
static void print_value(value_t *value, int indent);

value_t *parse(const char* source, void *memory) {
    if (!memory) return NULL;

    // Initialize the bump allocator with the memory for the parser
    // bump_init()
}

parser_t *parser_init(lexer_t *lexer) {
    parser_t *parser = bump_alloc(&parser_allocator, sizeof(parser_t), ALIGNOF(parser_t));
    if (!parser) return NULL;

    parser->lexer = lexer;
    parser->had_error = false;
    parser->error_message = NULL;

    // Get the first token
    parser->current = lexer_scan_token(lexer);
    parser->previous = NULL;
    return parser;
}

value_t *parser_parse(const char* source) {
    // Restart the bump allocator for this parse
    bump_init(&parser_allocator, parser_memory, PARSER_MEMORY_SIZE);
    
    // Init our lexer and parser
    lexer_t *lexer = lexer_init(source);
    parser_t *parser = parser_init(lexer);

    // Create our root object to hold all top-level fields
    value_t *root = create_object_value();

    // Keep parsing fields until we reach the end
    while (!parser_match(parser, TOKEN_EOF)) {
        // Store the key token before we parse the field
        if (parser->current->type != TOKEN_KEY) {
            parser_error(parser, "Expected field name at top level");
            return NULL;
        }
        token_t key = *parser->current;

        // Parse the field
        value_t *value = parser_parse_field(parser);
        if (value == NULL) {
            // Handle error
            free_value(root);
            return NULL;
        }

        // Add to our root object
        object_add(root, key.value.str.start, key.value.str.length, value);
    }
    
    // Check if we had any errors
    if (parser->had_error) {
        free_value(root);
        printf("Parse error: %s\n", parser->error_message);
        return NULL;
    }

    // Clean up
    free(parser->error_message);
    // free(parser);
    free(lexer);

    return root;
}

void parser_advance(parser_t *parser) {
    // Save the current token for error reporting
    parser->previous = parser->current;

    // Keep getting tokens until we get a non-error one or reach EOF
    while (true) {
        parser->current = lexer_scan_token(parser->lexer);
        if (parser->current->type != TOKEN_ERROR) break;

        // Handle error token by reporting it
        parser_error(parser, "Unexpected character");
    }
}

void parser_consume(parser_t *parser, token_type_t type, const char* error_message) {
    // Like match, but raises an error if the token isn't what we expect
    if (parser->current->type == type) {
        parser_advance(parser);
        return;
    }

    parser_error(parser, error_message);
}

bool parser_match(parser_t *parser, token_type_t type) {
    // Check if current token matches expected type
    if (parser->current->type != type) return false;
    parser_advance(parser);
    return true;
}

void parser_error(parser_t *parser, const char* message) {
    // Avoid cascading errors
    if (parser->had_error) return;

    parser->had_error = true;

    // Allocate space for a detailed error message
    int message_size = strlen(message) + 100; // Extra space for line/column info
    parser->error_message = malloc(message_size);

    // Format error with location information
    snprintf(parser->error_message, message_size,
        "Error at line %d, column %d: %s",
        parser->current->line,
        parser->current->column,
        message);
}

value_t *parser_parse_field(parser_t *parser) {
    // We expect key = value;

    // First, get the field name (key)
    if (parser->current->type != TOKEN_KEY) {
        parser_error(parser, "Expected field name");
        return NULL;
    }

    token_t key = *parser->current;
    parser_advance(parser);

    // Expect the equals sign
    parser_consume(parser, TOKEN_EQUALS, "Expected '=' after field name");

    // Now parse the value
    value_t *value = parser_parse_value(parser);
    if (value == NULL) return NULL;

    // Expect semicolon
    parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' after value");

    return value;
}

value_t *parser_parse_object(parser_t *parser) {
    // Consume the opening brace
    parser_advance(parser);

    value_t *object = create_object_value();

    // Handle empty object
    if (parser_match(parser, TOKEN_BRACE_RIGHT)) {
        return object;
    }

    // Parse fields until we hit the closing brace
    while (true) {

        // Remember the key token before we parse the field
        if (parser->current->type != TOKEN_KEY) {
            parser_error(parser, "Expected field name");
            return NULL;
        }
        token_t key = *parser->current;

        // Use our common parser_parse_field function
        value_t *value = parser_parse_field(parser);
        if (value == NULL) return NULL;

        // Add to our object using the saved key
        object_add(object, key.value.str.start, key.value.str.length, value);

        // Check for end of object
        if (parser_match(parser, TOKEN_BRACE_RIGHT)) break;
    }

    return object;
}

value_t *parser_parse_value(parser_t *parser) {
    switch (parser->current->type) {
        case TOKEN_STRING: {
            value_t *value = create_string_value(parser->current);
            parser_advance(parser);
            return value;
        }

        case TOKEN_INT: {
            value_t *value = create_value(VALUE_INT);
            value->as.integer = parser->current->value.integer;
            parser_advance(parser);
            return value;
        }

        case TOKEN_FLOAT: {
            value_t *value = create_value(VALUE_FLOAT);
            value->as.float_val = parser->current->value.float_val;
            parser_advance(parser);
            return value;
        }

        case TOKEN_BOOL: {
            value_t *value = create_value(VALUE_BOOL);
            value->as.boolean = parser->current->value.boolean;
            parser_advance(parser);
            return value;
        }

        case TOKEN_BRACE_LEFT: {
            return parser_parse_object(parser);
        }

        case TOKEN_BRACKET_LEFT: {
            return parser_parse_array(parser);
        }

        default: {
            parser_error(parser, "Expected value");
            return NULL;
        }
    }
}

value_t *parser_parse_array(parser_t *parser) {
    // Consume the opening bracket [
    parser_advance(parser);

    value_t *array = create_value(VALUE_ARRAY);
    array->as.array.count = 0;
    array->as.array.capacity = 8;
    array->as.array.values = malloc(sizeof(value_t) * array->as.array.capacity);

    // Handle empty array
    if (parser_match(parser, TOKEN_BRACKET_RIGHT)) {
        return array;
    }

    // Parse values until we hit the closing bracket
    while (true) {
        value_t *element = parser_parse_value(parser);
        if (element == NULL) {
            free_value(array);
            return NULL;
        }

        // Add to our array
        if (array->as.array.count >= array->as.array.capacity) {
            array->as.array.capacity *= 2;
            array->as.array.values = realloc(array->as.array.values, sizeof(value_t) * array->as.array.capacity);
        }

        array->as.array.values[array->as.array.count++] = *element;
        free_value(element); // Free the original structure

        if (!parser_match(parser, TOKEN_COMMA)) break;
    }

    parser_consume(parser, TOKEN_BRACKET_RIGHT, "Expected ']' after array elements");
    return array;
}

void parser_test(const char* input) {
    value_t *root = parser_parse(input);
    if (root == NULL) {
        printf("Parsing failed!\n");
        return;
    }
    
    printf("Parsed structure:\n");
    print_value(root, 0);
    printf("\n");

    free_value(root);
}

static value_t *create_value(value_type_t type) {
    value_t *value = malloc(sizeof(value_t));
    if (!value) return NULL;

    value->type = type;
    return value;
}

static value_t *create_string_value(token_t *token) {
    value_t *value = create_value(VALUE_STRING);
    // Store the string position and length to not have to copy
    value->as.string.start = token->value.str.start;
    value->as.string.length = token->value.str.length;
    return value;
}

static value_t *create_object_value(void) {
    value_t *value = create_value(VALUE_OBJECT);
    value->as.object.count = 0;
    value->as.object.capacity = 8;
    value->as.object.keys = malloc(sizeof(char*) * value->as.object.capacity);
    value->as.object.values = malloc(sizeof(value_t) * value->as.object.capacity);
    return value;
}

// Add a key-value par to an object
static void object_add(value_t *object, const char *key, int key_length, value_t *value) {
    // Grow the arrays if needed
    if (object->as.object.count >= object->as.object.capacity) {
        int new_capacity = object->as.object.capacity * 2;
        object->as.object.keys = realloc(object->as.object.keys, sizeof(char*) * new_capacity);
        object->as.object.values = realloc(object->as.object.values, sizeof(value_t) * new_capacity);
        object->as.object.capacity = new_capacity;
    }

    // Store the key and value
    char *key_copy = malloc(key_length + 1);
    memcpy(key_copy, key, key_length);
    key_copy[key_length] = '\0';

    object->as.object.keys[object->as.object.count] = key_copy;
    object->as.object.values[object->as.object.count] = *value;
    object->as.object.count++;
}

static void free_value_internal(value_t *value, bool free_root) {
    if (value == NULL) return;

    switch (value->type) {
        case VALUE_OBJECT: {
            for (int i = 0; i < value->as.object.count; ++i) {
                free(value->as.object.keys[i]);
                free_value_internal(&value->as.object.values[i], false);
            }
            free(value->as.object.keys);
            free(value->as.object.values);
        } break;

        case VALUE_ARRAY: {
            for (int i = 0; i < value->as.array.count; ++i) {
                free_value_internal(&value->as.array.values[i], false);
            }
            free(value->as.array.values);
        } break;

        // Other types don't need special cleanup
        default: break;
    }

    if (free_root) {
        free(value);
    }
}

static void free_value(value_t *value) {
    free_value_internal(value, true);
}

static void print_value(value_t *value, int indent) {
    if (value == NULL) return;

    char indent_str[256] = {0};
    for (int i = 0; i < indent; ++i) indent_str[i] = ' ';

    switch (value->type) {
        case VALUE_STRING:
            printf("%s\"%.*s\"", indent_str, value->as.string.length, value->as.string.start);
            break;

        case VALUE_INT:
            printf("%s%d", indent_str, value->as.integer);
            break;

        case VALUE_FLOAT:
            printf("%s%.2f", indent_str, value->as.float_val);
            break;

        case VALUE_BOOL:
            printf("%s%s", indent_str, value->as.boolean ? "true" : "false");
            break;

        case VALUE_ARRAY: {
            printf("%s[\n", indent_str);
            for (int i = 0; i < value->as.array.count; ++i) {
                print_value(&value->as.array.values[i], indent + 2);
                if (i < value->as.array.count - 1) printf(",");
                printf("\n");
            }
            printf("%s]", indent_str);
        } break;

        case VALUE_OBJECT: {
            printf("%s{\n", indent_str);
            for (int i = 0; i < value->as.object.count; ++i) {
                printf("%s %s = ", indent_str, value->as.object.keys[i]);
                print_value(&value->as.object.values[i], indent + 2);
                printf(";\n");
            }
            printf("%s}", indent_str);
        } break;

        default: break;
    }
}

# Field Parser

A lightweight, header-only configuration parser for the `.fld` format with an efficient bump allocator implementation.

## Features

- Header-only C implementation
- Lightweight and performant
- Built-in bump allocator for efficient memory management
- Support for nested structures
- Rich type system including strings, integers, floats, booleans, and typed arrays
- Simple dot notation for accessing nested values
- Single-line compatibility unlike YAML, making it ideal for command-line tools and simple configurations
- Strongly typed arrays that enforce consistent value types
- Support for both single-line and block comments
- Iterator support for traversing fields
- String view utilities for efficient string operations
- Vector types (vec2, vec3, vec4) with type safety

## File Format

Field uses a simple, readable syntax for configuration files with the `.fld` extension. The format supports comments, multiple data types, and nested structures.

### Comments

The format supports both single-line and block comments:

```field
// This is a single-line comment

/* This is a block comment
   that can span multiple lines
   and is ideal for longer documentation */

username = "jane_doe";  // Inline comments are supported too

/* Block comments can also
   document complex structures */
settings = {
    theme = "dark";    /* With inline
                         block comments */
    notifications = true;
};
```

### Supported Types

- **Strings**: Double-quoted text values
- **Integers**: Whole numbers
- **Floats**: Decimal numbers
- **Booleans**: `true` or `false`
- **Vectors**: 2D, 3D, and 4D float vectors
- **Arrays**: Homogeneous collections of values (must contain elements of the same type)
- **Structures**: Nested configuration blocks (can be nested to any depth)

### Syntax Examples

#### Multi-line Format
The configuration can be formatted with newlines for readability:

```field
// String fields
username = "jane_doe";      // populated string value
description = "";           // zero value for strings

/* User information block
   Contains demographic data */
age = 30;               // populated integer value
login_count = 0;        // zero value for integers

// Float fields
height = 1.75;      // populated float value
progress = 0.0;     // zero value for floats

// Vector fields
position = vec2(100.0, 200.0);          // 2D vector
scale = vec3(1.0, 1.0, 1.0);            // 3D vector
color = vec4(1.0, 0.0, 0.0, 1.0);       // 4D vector

// Boolean fields
is_active = true;           // populated boolean value
is_verified = false;        // zero value for booleans

// Array fields
hobbies = ["reading", "hiking", "photography"];     // populated array
tags = [];                                          // zero value for arrays

/* Settings block
   Contains all user preferences
   and display configurations */
settings = {
    theme = "dark";
    notifications = true;
    refresh_rate = 60;
    window = {
        size = vec2(1920.0, 1080.0);
        position = vec2(0.0, 0.0);
    };
    camera = {
        position = vec3(0.0, 1.0, -5.0);
        rotation = vec3(0.0, 180.0, 0.0);
    };
};
```

#### Single-line Format
The same configuration can be written on a single line, making it perfect for command-line tools:

```field
/* Basic configuration */ username = "jane_doe"; age = 30; /* User settings */ settings = { theme = "dark"; notifications = true; display = { brightness = 0.8; }; };
```

### Vector Type Rules

- Vectors must have the correct number of components (2 for vec2, 3 for vec3, 4 for vec4)
- Components can be integers or floats (integers are automatically converted to floats)
- Vector types:
  ```field
  position = vec2(1.0, 2.0);               // 2D vector
  scale = vec3(1.0, 1.0, 1.0);            // 3D vector
  color = vec4(1.0, 0.0, 0.0, 0.5);       // 4D vector
  mixed = vec3(1, 2.0, 3);                // Valid: ints convert to floats
  invalid = vec3(1.0, "string", true);    // Invalid: wrong types
  ```

### Array Type Rules

- Arrays must contain elements of the same type
- Supported array types: string[], int[], float[], bool[]
- Arrays cannot be nested (no array of arrays)
- Examples:
  ```field
  numbers = [1, 2, 3];                    // Valid int array
  names = ["Alice", "Bob", "Charlie"];    // Valid string array
  mixed = [1, "two", true];              // Invalid: mixed types
  nested = [[1, 2], [3, 4]];             // Invalid: nested arrays
  ```

## Installation & Setup

The parser follows the single-header file philosophy (similar to [stb](https://github.com/nothings/stb) libraries). To use it:

1. Copy `field_parser.h` to your project
2. Include the header normally in any file that needs to *declare* the parser functions:
```c
#include "field_parser.h"
```

3. In *exactly one* source file, define `FLD_PARSER_IMPLEMENTATION` before including the header to include the implementation:
```c
#define FLD_PARSER_IMPLEMENTATION
#include "field_parser.h"
```

This pattern prevents multiple definition errors while maintaining the convenience of a single header file.

## Usage

### Including the Parser

Add the necessary include (and implementation define if this is the implementation file):

```c
// In your implementation file (e.g. main.c):
#define FLD_PARSER_IMPLEMENTATION
#include "field_parser.h"

// In other files that just need declarations:
#include "field_parser.h"
```

### Basic Parsing

```c
// Allocate memory for the parser
char memory[1024 * 8];
fld_parser parser;

// Parse the input
if (!fld_parse(&parser, input_string, memory, sizeof(memory))) {
    // Handle parsing error
    fld_error error = fld_get_last_error(&parser);
    printf("Error at line %d, column %d: %s\n", 
           error.line, 
           error.column, 
           fld_error_string(error.code));
}
```

### Accessing Values

The parser provides several methods to access and validate values:

```c
// Check if a field exists
if (fld_has_field(parser.root, "settings.theme")) {
    // Field exists
}

// Get a string value (two methods available)
fld_string_view str_view;
if (fld_get_str_view(parser.root, "settings.theme", &str_view)) {
    printf("Theme: %.*s\n", str_view.length, str_view.start);
}

char theme_buffer[256];
if (fld_get_cstr(parser.root, "settings.theme", theme_buffer, sizeof(theme_buffer))) {
    printf("Theme: %s\n", theme_buffer);
}

// String view utilities
char buffer[256];
fld_string_view_to_cstr(str_view, buffer, sizeof(buffer));
bool matches = fld_string_view_eq(str_view, "expected_string");

// Get an integer value
int int_val;
if (fld_get_int(parser.root, "age", &int_val)) {
    printf("Age: %d\n", int_val);
}

// Get a float value
float float_val;
if (fld_get_float(parser.root, "settings.display.brightness", &float_val)) {
    printf("Brightness: %f\n", float_val);
}

// Get a boolean value
bool bool_val;
if (fld_get_bool(parser.root, "settings.notifications", &bool_val)) {
    printf("Notifications: %s\n", bool_val ? "true" : "false");
}

// Get a vec2 value
float x, y;
if (fld_get_vec2(parser.root, "settings.window.size", &x, &y)) {
    printf("Window size: %.0fx%.0f\n", x, y);
}

// Get a vec3 value
float x, y, z;
if (fld_get_vec3(parser.root, "settings.camera.position", &x, &y, &z)) {
    printf("Camera position: %.1f, %.1f, %.1f\n", x, y, z);
}

// Get a vec4 value
float r, g, b, a;
if (fld_get_vec4(parser.root, "color", &r, &g, &b, &a)) {
    printf("Color (RGBA): %.1f, %.1f, %.1f, %.1f\n", r, g, b, a);
}

// Get raw vector components
float components[4];
size_t count;
if (fld_get_vector_components(parser.root, "settings.camera.position", components, &count)) {
    // components array contains the values, count tells you how many (2, 3, or 4)
}

// Get an array
fld_value_type array_type;
void *array;
size_t array_count;
if (fld_get_array(parser.root, "settings.another_array", &array_type, &array, &array_count)) {
    // Handle array based on array_type
}

// Get array information
size_t array_size;
if (fld_get_array_size(parser.root, "settings.colors", &array_size)) {
    printf("Array has %zu items\n", array_size);
}
```

### Accessing Nested Values

Use dot notation to access nested values:
- `settings.theme` accesses the theme field inside settings
- `settings.display.brightness` accesses the brightness field inside the display structure inside settings

## Memory Management

The parser uses a bump allocator for efficient memory management. You need to provide a memory buffer during initialization:

```c
char memory[1024 * 8];  // Adjust size based on your needs

// You can also estimate required memory
size_t needed = fld_estimate_memory(source_string);
void* memory = malloc(needed);  // Or other allocation method
```

The parser will use this memory for all allocations during parsing. No manual memory management is required.

### Iterating Over Fields

The parser supports two types of iteration:

```c
// Initialize iterator for flat iteration (current level only)
fld_iterator iter;
fld_iter_init(&iter, parser.root, FLD_ITER_FIELDS);

// Or for recursive iteration (including nested fields)
fld_iter_init(&iter, parser.root, FLD_ITER_RECURSIVE);

// Iterate over fields
fld_object *field;
while ((field = fld_iter_next(&iter))) {
    // Access field properties
    printf("Field: %.*s\n", field->key.length, field->key.start);
}
```

## Error Handling

All getter functions return a boolean indicating success or failure. Always check the return value before using the retrieved data.

The parser provides comprehensive error handling:

```c
// After parsing
fld_error error = fld_get_last_error(&parser);
if (error.code != FLD_ERROR_NONE) {
    printf("Error at line %d, column %d: %s\n",
           error.line,
           error.column,
           fld_error_string(error.code));
}
```

Error codes include:
- `FLD_ERROR_NONE`: No error
- `FLD_ERROR_OUT_OF_MEMORY`: Memory allocation failed
- `FLD_ERROR_UNEXPECTED_TOKEN`: Syntax error
- `FLD_ERROR_INVALID_NUMBER`: Invalid number format
- `FLD_ERROR_INSUFFICIENT_MEMORY`: Provided memory buffer too small
- `FLD_ERROR_ARRAY_TYPE_MISMATCH`: Array contains mixed types
- `FLD_ERROR_ARRAY_NOT_SUPPORTED_TYPE`: Unsupported array element type
- `FLD_ERROR_ARRAY_TOO_MANY_ITEMS`: Array exceeds maximum size

## Building

The parser is header-only, so no building is required. Simply include the header file in your project.
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

## File Format

Field uses a simple, readable syntax for configuration files with the `.fld` extension. The format supports comments, multiple data types, and nested structures.

### Supported Types

- **Strings**: Double-quoted text values
- **Integers**: Whole numbers
- **Floats**: Decimal numbers
- **Booleans**: `true` or `false`
- **Arrays**: Homogeneous collections of values (must contain elements of the same type)
- **Structures**: Nested configuration blocks (can be nested to any depth)

### Syntax Examples

#### Multi-line Format
The configuration can be formatted with newlines for readability:

```field
// String fields
username = "jane_doe";           // populated string value
description = "";               // zero value for strings

// Integer fields  
age = 30;                       // populated integer value
login_count = 0;               // zero value for integers

// Float fields
height = 1.75;                  // populated float value
progress = 0.0;                // zero value for floats

// Boolean fields
is_active = true;               // populated boolean value
is_verified = false;           // zero value for booleans

// Array fields
hobbies = ["reading", "hiking", "photography"];    // populated array
tags = [];                                        // zero value for arrays

// Nested structure demonstration
settings = {
    theme = "dark";
    notifications = true;
    refresh_rate = 60;
    custom_colors = ["#FF0000", "#00FF00", "#0000FF"];
    display = {
        brightness = 0.8;
        contrast = 1.0;
    };
};
```

#### Single-line Format
The same configuration can be written on a single line, making it perfect for command-line tools:

```field
username = "jane_doe"; age = 30; settings = { theme = "dark"; notifications = true; display = { brightness = 0.8; }; };
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

## Usage

### Including the Parser

```c
#define FLD_PARSER_IMPLEMENTATION
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
}
```

### Accessing Values

The parser provides type-specific functions to access values:

```c
// Get a string value
fld_string_view str_val;
if (fld_get_string(parser.root, "settings.theme", &str_val.start, &str_val.length)) {
    printf("Theme: %.*s\n", str_val.length, str_val.start);
}

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

// Get an array
fld_value_type array_type;
void *array;
size_t array_count;
if (fld_get_array(parser.root, "settings.another_array", &array_type, &array, &array_count)) {
    // Handle array based on array_type
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
```

The parser will use this memory for all allocations during parsing. No manual memory management is required.

## Error Handling

All getter functions return a boolean indicating success or failure. Always check the return value before using the retrieved data.

## Building

The parser is header-only, so no building is required. Simply include the header file in your project.

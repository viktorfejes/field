/*
 * Field Parser Example
 * 
 * This example demonstrates the main features of the FLD parser, including:
 * - Parsing primitive types (strings, numbers, booleans)
 * - Handling nested objects
 * - Working with arrays
 * - Using vectors (vec2, vec3)
 * - Using iterators (both flat and recursive)
 * - Accessing fields via dot notation
 */

#define FLD_PARSER_IMPLEMENTATION
#include "../include/field_parser.h"
#include <stdio.h>

// Example configuration in FLD format
const char* example_config = 
    "// User profile configuration\n"
    "user = {\n"
    "    name = \"John Doe\";\n"
    "    age = 30;\n"
    "    email = \"john.doe@example.com\";\n"
    "    verified = true;\n"
    "    avatar_scale = vec3(1.0, 1.0, 1.0);\n"
    "};\n"
    "\n"
    "/* Application settings:\n"
    "   theme, notifications. */\n"
    "settings = {\n"
    "    theme = {\n"
    "        mode = \"dark\";\n"
    "        colors = [\"#1a1a1a\", \"#ffffff\", \"#007acc\"];\n"
    "        opacity = -0.95;\n"
    "        size = vec2(1920.0, 1080);\n"
    "    };\n"
    "    notifications = {\n"
    "        enabled = true;\n"
    "        volume = 0.8;\n"
    "        priorities = [1, 2, -3, 5];\n"
    "    };\n"
    "};\n"
    "\n"
    "// Feature flags\n"
    "features = {\n"
    "    experimental = false;\n"
    "    beta_functions = [\"cloud_sync\", \"ai_assist\", \"dark_mode\"];\n"
    "};\n";

// Helper function to print array contents
void print_array(const char* name, void* items, size_t count, fld_value_type type) {
    printf("%s (%zu items): [", name, count);
    
    switch (type) {
        case FLD_VALUE_INT: {
            int* values = (int*)items;
            for (size_t i = 0; i < count; i++) {
                printf("%d%s", values[i], i < count - 1 ? ", " : "");
            }
            break;
        }
        case FLD_VALUE_STRING: {
            fld_string_view* values = (fld_string_view*)items;
            for (size_t i = 0; i < count; i++) {
                printf("\"%.*s\"%s", (int)values[i].length, values[i].start, 
                       i < count - 1 ? ", " : "");
            }
            break;
        }
        case FLD_VALUE_FLOAT: {
            float* values = (float*)items;
            for (size_t i = 0; i < count; i++) {
                printf("%.2f%s", values[i], i < count - 1 ? ", " : "");
            }
            break;
        }
        default:
            printf("(unsupported type)");
    }
    printf("]\n");
}

int main(void) {
    // Initialize parser
    fld_parser parser = {0};
    char memory[1024 * 1024];  // Adjust size based on your needs
    
    printf("=== FLD Parser Example ===\n\n");
    
    // Parse the configuration
    if (!fld_parse(&parser, example_config, memory, sizeof(memory))) {
        printf("Error parsing configuration!\n");
        printf("Error at line %d, column %d: %s\n",
               parser.last_error.line,
               parser.last_error.column,
               fld_error_string(parser.last_error.code));
        return 1;
    }
    
    printf("1. Accessing primitive values:\n");
    printf("-------------------------------\n");
    
    // Demonstrate string access
    char name[64];
    if (fld_get_cstr(parser.root, "user.name", name, sizeof(name))) {
        printf("User name: %s\n", name);
    }
    
    // Demonstrate number access
    int age;
    if (fld_get_int(parser.root, "user.age", &age)) {
        printf("User age: %d\n", age);
    }
    
    // Demonstrate boolean and float access
    bool verified;
    float volume;
    if (fld_get_bool(parser.root, "user.verified", &verified) &&
        fld_get_float(parser.root, "settings.notifications.volume", &volume)) {
        printf("Account verified: %s\n", verified ? "yes" : "no");
        printf("Notification volume: %.2f\n", volume);
    }
    
    printf("\n2. Working with arrays:\n");
    printf("-------------------------------\n");
    
    // Demonstrate array access
    fld_value_type type;
    void* items;
    size_t count;
    
    if (fld_get_array(parser.root, "settings.theme.colors", &type, &items, &count)) {
        print_array("Theme colors", items, count, type);
    }
    
    if (fld_get_array(parser.root, "settings.notifications.priorities", &type, &items, &count)) {
        print_array("Notification priorities", items, count, type);
    }

    printf("\n2. Working with vectors:\n");
    printf("-------------------------------\n");
    
    // Demonstrate vector access
    float width, height;
    if (fld_get_vec2(parser.root, "settings.theme.size", &width, &height)) {
        printf("Theme window size: %.0fx%.0f\n", width, height);
    }

    float scale_x, scale_y, scale_z;
    if (fld_get_vec3(parser.root, "user.avatar_scale", &scale_x, &scale_y, &scale_z)) {
        printf("Avatar scale: %.1f, %.1f, %.1f\n", scale_x, scale_y, scale_z);
    }
    
    printf("\n3. Using iterators:\n");
    printf("-------------------------------\n");
    
    // Demonstrate recursive iteration
    printf("Full configuration structure:\n");
    fld_iterator iter;
    fld_iter_init(&iter, parser.root, FLD_ITER_RECURSIVE);
    
    char path[256];
    fld_object* obj;
    while ((obj = fld_iter_next(&iter))) {
        if (fld_iter_get_path(&iter, path, sizeof(path))) {
            printf("%*s- %s", iter.depth * 2, "", path);
            
            // Print values for primitive types
            switch (obj->value.type) {
                case FLD_VALUE_STRING: {
                    char value[64];
                    if (fld_string_view_to_cstr(obj->value.as.string, value, sizeof(value))) {
                        printf(" = \"%s\"", value);
                    }
                    break;
                }
                case FLD_VALUE_INT:
                    printf(" = %d", obj->value.as.integer);
                    break;
                case FLD_VALUE_FLOAT:
                    printf(" = %.2f", obj->value.as.float_val);
                    break;
                case FLD_VALUE_BOOL:
                    printf(" = %s", obj->value.as.boolean ? "true" : "false");
                    break;
                case FLD_VALUE_ARRAY: {
                    printf(" = [%d items]", obj->value.as.array.count);
                    break;
                }
                case FLD_VALUE_VEC2:
                    printf(" = vec2(%.1f, %.1f)", 
                           obj->value.as.vec2.x, 
                           obj->value.as.vec2.y);
                    break;
                case FLD_VALUE_VEC3:
                    printf(" = vec3(%.1f, %.1f, %.1f)", 
                           obj->value.as.vec3.x, 
                           obj->value.as.vec3.y,
                           obj->value.as.vec3.z);
                    break;
                case FLD_VALUE_VEC4:
                    printf(" = vec4(%.1f, %.1f, %.1f, %.1f)", 
                           obj->value.as.vec4.x, 
                           obj->value.as.vec4.y,
                           obj->value.as.vec4.z,
                           obj->value.as.vec4.w);
                    break;
                case FLD_VALUE_OBJECT:
                    printf(" = {...}");
                    break;
                default: break;
            }
            printf("\n");
        }
    }
    
    return 0;
}

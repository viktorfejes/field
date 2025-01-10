#define FLD_PARSER_IMPLEMENTATION
#include "../include/field_parser.h"

#include <stdio.h>

// !!!THIS IS NOT A UNIT TEST OR ANYTHING SERIOUS!!!
// At this point this is more like a playground to test the API
// !!!THIS IS NOT A UNIT TEST OR ANYTHING SERIOUS!!!

int main(void) {
    const char* test_input = 
        "// This is a test input\n"
        "username = \"jane_doe\";\n"
        "age = 30; /* This is a random comment here, and no new line */"
        "height = 1.75;\n"
        "is_active = true;\n"
        "hobbies = [\"reading\", \"hiking\"];\n"
        "settings = {\n"
        "    theme = \"dark\";\n"
        "    notifications = false;\n"
        "    display = {\n"
        "        brightness = 0.8;\n"
        "    };\n"
        "    another_array = [0.0, 2.1, 3.2, 4.4];\n"
        "};\n"
        "last_name = \"Fejes\";\n";

    char memory[1024 * 8];
    fld_parser parser;
    if (!fld_parse(&parser, test_input, memory, sizeof(memory))) {
        printf("Parsing failed!\n");
        return 1;
    }

    char str_buff[64];
    if (fld_get_cstr(parser.root, "username", str_buff, sizeof(str_buff))) {
        printf("Username: %s\n", str_buff);
    }

    fld_string_view test;
    if (fld_get_str_view(parser.root, "settings.theme", &test)) {
        printf("Settings.theme: %.*s\n", test.length, test.start);
    }

    int age;
    if (fld_get_int(parser.root, "age", &age)) {
        printf("Age: %d\n", age);
    }

    float brightness;
    if (fld_get_float(parser.root, "settings.display.brightness", &brightness)) {
        printf("Settings.display.brightness: %f\n", brightness);
    }

    bool notifications;
    if (fld_get_bool(parser.root, "settings.notifications", &notifications)) {
        printf("Settings.notifications: %s\n", notifications ? "true" : "false");
    }

    fld_value_type array_type;
    void *hobbies;
    size_t hobbies_count;
    if (fld_get_array(parser.root, "settings.another_array", &array_type, &hobbies, &hobbies_count)) {
        printf("Hobbies:\n");
        for (size_t i = 0; i < hobbies_count; ++i) {
            float *hobby = (float*)hobbies;
            printf("  %f\n", hobby[i]);
        }
    }

    return 0;
}

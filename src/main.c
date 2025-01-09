#include "parser.h"

int main(void) {
    const char* test_input = 
        "// This is a test input\n"
        "username = \"jane_doe\";\n"
        "age = 30;\n"
        "height = 1.75;\n"
        "is_active = TRUE;\n"
        "hobbies = [\"reading\", \"hiking\"];\n"
        "settings = {\n"
        "    theme = \"dark\";\n"
        "    notifications = false;\n"
        "    display = {\n"
        "        brightness = 0.8;\n"
        "    };\n"
        "};\n";
    
    // test_tokenizer(test_input);

    parser_test(test_input);

    return 0;
}

#define VF_TEST_IMPLEMENTATION
#include "lib/vf_test.h"

#define FLD_PARSER_IMPLEMENTATION
#include "../include/field_parser.h"

// Helper function to create a parser with memory
static bool setup_parser(fld_parser* parser, const char* source, void** memory) {
    size_t needed = fld_estimate_memory(source);
    *memory = malloc(needed);
    if (!*memory) return false;
    
    return fld_parse(parser, source, *memory, needed);
}

// Helper to clean up parser memory
static void cleanup_parser(void* memory) {
    free(memory);
}

TEST(Parser, BasicPrimitives) {
    const char* source = 
        "string_val = \"test\";\n"
        "int_val = 42;\n"
        "float_val = 3.14;\n"
        "bool_val = true;\n"
        "negative_int = -2141;\n"
        "negative_float = -3.14;\n";
    
    fld_parser parser = {0};
    void* memory = NULL;
    
    EXPECT_TRUE(setup_parser(&parser, source, &memory));
    
    // Test string value
    fld_string_view str_view;
    EXPECT_TRUE(fld_get_str_view(parser.root, "string_val", &str_view));
    char str_buffer[32];
    EXPECT_TRUE(fld_string_view_to_cstr(str_view, str_buffer, sizeof(str_buffer)));
    EXPECT_TRUE(strcmp(str_buffer, "test") == 0);
    
    // Test integer value
    int int_val;
    EXPECT_TRUE(fld_get_int(parser.root, "int_val", &int_val));
    EXPECT_EQ_INT(int_val, 42);
    
    // Test float value
    float float_val;
    EXPECT_TRUE(fld_get_float(parser.root, "float_val", &float_val));
    EXPECT_TRUE(float_val > 3.13f && float_val < 3.15f);
    
    // Test boolean value
    bool bool_val;
    EXPECT_TRUE(fld_get_bool(parser.root, "bool_val", &bool_val));
    EXPECT_TRUE(bool_val);

    // Test negative integer value
    int neg_int;
    EXPECT_TRUE(fld_get_int(parser.root, "negative_int", &neg_int));
    EXPECT_EQ_INT(neg_int, -2141);

    // Test negative float value
    float neg_float;
    EXPECT_TRUE(fld_get_float(parser.root, "negative_float", &neg_float));
    EXPECT_EQ_FLOAT(neg_float, -3.14);
    
    cleanup_parser(memory);
    return true;
}

TEST(Parser, BigNumbers) {
    const char* source = 
        "big_int = 999999999999999;\n"
        "big_float = 999999999.99999999999999;\n";
    
    fld_parser parser = {0};
    void* memory = NULL;
    
    EXPECT_FALSE(setup_parser(&parser, source, &memory));

    cleanup_parser(memory);
    return true;
}

TEST(Parser, NestedObjects) {
    const char* source = 
        "outer = {\n"
        "    inner = {\n"
        "        value = 123;\n"
        "    };\n"
        "};";
    
    fld_parser parser = {0};
    void* memory = NULL;
    
    EXPECT_TRUE(setup_parser(&parser, source, &memory));
    
    // Test nested path access
    int val;
    EXPECT_TRUE(fld_get_int(parser.root, "outer.inner.value", &val));
    EXPECT_EQ_INT(val, 123);
    
    // Test object getter
    fld_object* outer;
    EXPECT_TRUE(fld_get_object(parser.root, "outer", &outer));
    EXPECT_TRUE(outer != NULL);
    
    fld_object* inner;
    EXPECT_TRUE(fld_get_object(outer, "inner", &inner));
    EXPECT_TRUE(inner != NULL);
    
    cleanup_parser(memory);
    return true;
}

TEST(Parser, Arrays) {
    const char* source = 
        "int_array = [1, 2, 3, 4];\n"
        "string_array = [\"one\", \"two\", \"three\"];\n"
        "bool_array = [true, false, true];\n";
    
    fld_parser parser = {0};
    void* memory = NULL;
    
    EXPECT_TRUE(setup_parser(&parser, source, &memory));
    
    // Test int array
    fld_value_type type;
    void* items;
    size_t count;
    EXPECT_TRUE(fld_get_array(parser.root, "int_array", &type, &items, &count));
    EXPECT_EQ(type, FLD_VALUE_INT);
    EXPECT_EQ(count, 4);
    int* int_items = (int*)items;
    EXPECT_EQ(int_items[0], 1);
    EXPECT_EQ(int_items[3], 4);
    
    // Test string array
    EXPECT_TRUE(fld_get_array(parser.root, "string_array", &type, &items, &count));
    EXPECT_EQ(type, FLD_VALUE_STRING);
    EXPECT_EQ(count, 3);
    fld_string_view* str_items = (fld_string_view*)items;
    char buffer[32];
    EXPECT_TRUE(fld_string_view_to_cstr(str_items[0], buffer, sizeof(buffer)));
    EXPECT_TRUE(strcmp(buffer, "one") == 0);
    
    cleanup_parser(memory);
    return true;
}

TEST(Parser, ErrorHandling) {
    const char* invalid_source = "invalid = };";
    
    fld_parser parser = {0};
    void* memory = NULL;
    
    EXPECT_FALSE(setup_parser(&parser, invalid_source, &memory));
    EXPECT_TRUE(parser.last_error.code != FLD_ERROR_NONE);
    
    if (memory) cleanup_parser(memory);
    return true;
}

TEST(Parser, TypeChecking) {
    const char* source = "value = 42;";
    
    fld_parser parser = {0};
    void* memory = NULL;
    
    EXPECT_TRUE(setup_parser(&parser, source, &memory));
    
    // Test correct type
    EXPECT_EQ(fld_get_type(parser.root, "value"), FLD_VALUE_INT);
    
    // Test wrong type access
    float float_val;
    EXPECT_FALSE(fld_get_float(parser.root, "value", &float_val));
    
    // Test non-existent field
    EXPECT_EQ(fld_get_type(parser.root, "nonexistent"), FLD_VALUE_EMPTY);
    
    cleanup_parser(memory);
    return true;
}

TEST(Parser, Comments) {
    const char* source = 
        "// Line comment\n"
        "value1 = 1; // End of line comment\n"
        "/* Block comment */\n"
        "value2 = 2;\n"
        "/* Multi-line\n"
        "   block comment */\n"
        "value3 = 3;";
    
    fld_parser parser = {0};
    void* memory = NULL;
    
    EXPECT_TRUE(setup_parser(&parser, source, &memory));
    
    int val;
    EXPECT_TRUE(fld_get_int(parser.root, "value1", &val));
    EXPECT_EQ(val, 1);
    EXPECT_TRUE(fld_get_int(parser.root, "value2", &val));
    EXPECT_EQ(val, 2);
    EXPECT_TRUE(fld_get_int(parser.root, "value3", &val));
    EXPECT_EQ(val, 3);
    
    cleanup_parser(memory);
    return true;
}

TEST(Parser, Iterator) {
    const char* source = 
        "field1 = 1;\n"
        "obj = {\n"
        "    nested1 = 2;\n"
        "    nested2 = 3;\n"
        "    nested3 = {\n"
        "       opacity = 1.2;\n"
        "    };\n"
        "};\n"
        "field2 = 4;\n";
    
    fld_parser parser = {0};
    void* memory = NULL;
    
    EXPECT_TRUE(setup_parser(&parser, source, &memory));
    
    // Test flat iteration
    fld_iterator iter;
    fld_iter_init(&iter, parser.root, FLD_ITER_FIELDS);
    
    int count = 0;
    while (fld_iter_next(&iter)) {
        count++;
    }
    EXPECT_EQ(count, 3); // field1, obj, field2
    
    // Test recursive iteration
    fld_iter_init(&iter, parser.root, FLD_ITER_RECURSIVE);
    
    count = 0;
    char path[FLD_MAX_PATH_LENGTH];
    while (fld_iter_next(&iter)) {
        EXPECT_TRUE(fld_iter_get_path(&iter, path, sizeof(path)));
        count++;
    }
    EXPECT_EQ(count, 6); // field1, obj, obj.nested1, obj.nested2, field2
    
    cleanup_parser(memory);
    return true;
}

TEST(Parser, PathTraversal) {
    const char* source = 
        "user = {\n"
        "    profile = {\n"
        "        name = \"test\";\n"
        "        settings = {\n"
        "            active = true;\n"
        "        };\n"
        "    };\n"
        "};\n";
    
    fld_parser parser = {0};
    void* memory = NULL;
    
    EXPECT_TRUE(setup_parser(&parser, source, &memory));
    
    // Test deep path traversal
    bool active;
    EXPECT_TRUE(fld_get_bool(parser.root, "user.profile.settings.active", &active));
    EXPECT_TRUE(active);
    
    // Test non-existent paths
    EXPECT_FALSE(fld_get_bool(parser.root, "user.profile.nonexistent", &active));
    EXPECT_FALSE(fld_get_bool(parser.root, "nonexistent.path", &active));
    
    // Test invalid path (trying to traverse through a non-object)
    EXPECT_FALSE(fld_get_bool(parser.root, "user.profile.name.invalid", &active));
    
    cleanup_parser(memory);
    return true;
}

TEST(Parser, EdgeCases) {
    const char* source = 
        "empty = {};\n"
        "str = \"test\";\n"
        "obj = {\n"
        "    nested = true;\n"
        "};\n";
    
    fld_parser parser = {0};
    void* memory = NULL;
    
    EXPECT_TRUE(setup_parser(&parser, source, &memory));
    
    // Test accessing empty object
    fld_object* empty_obj;
    EXPECT_TRUE(fld_get_object(parser.root, "empty", &empty_obj));
    EXPECT_TRUE(empty_obj == NULL);
    
    // Test path traversal through string (should fail)
    bool val;
    EXPECT_FALSE(fld_get_bool(parser.root, "str.anything", &val));
    
    // Test null/empty path
    EXPECT_FALSE(fld_get_bool(parser.root, "", &val));
    EXPECT_FALSE(fld_get_bool(parser.root, NULL, &val));
    
    cleanup_parser(memory);
    return true;
}

int main(void) {
    RUN_ALL_TESTS();
    return 0;
}

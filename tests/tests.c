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

// TODO: Rename main name of tests from Parser to something better
// for better grouping.
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

TEST(Parser, VectorTypes) {
    const char* source = 
        "vec2_val = vec2(1.0, 2.0);\n"
        "vec3_val = vec3(-1.5, 0.0, 3.14);\n"
        "vec4_val = vec4(1.0, 2.0, 3.0, 4.0);\n"
        "// Test integer to float conversion\n"
        "vec2_ints = vec2(1, 2);\n"
        "vec3_mixed = vec3(1, 2.5, 3);\n"
        "nested = {\n"
        "    position = vec3(10.0, 20.0, 30.0);\n"
        "};\n";
    
    fld_parser parser = {0};
    void* memory = NULL;
    
    EXPECT_TRUE(setup_parser(&parser, source, &memory));
    
    // Test vec2 retrieval
    float x2, y2;
    EXPECT_TRUE(fld_get_vec2(parser.root, "vec2_val", &x2, &y2));
    EXPECT_EQ_FLOAT(x2, 1.0f);
    EXPECT_EQ_FLOAT(y2, 2.0f);
    
    // Test vec3 retrieval
    float x3, y3, z3;
    EXPECT_TRUE(fld_get_vec3(parser.root, "vec3_val", &x3, &y3, &z3));
    EXPECT_EQ_FLOAT(x3, -1.5f);
    EXPECT_EQ_FLOAT(y3, 0.0f);
    EXPECT_EQ_FLOAT(z3, 3.14f);
    
    // Test vec4 retrieval
    float x4, y4, z4, w4;
    EXPECT_TRUE(fld_get_vec4(parser.root, "vec4_val", &x4, &y4, &z4, &w4));
    EXPECT_EQ_FLOAT(x4, 1.0f);
    EXPECT_EQ_FLOAT(y4, 2.0f);
    EXPECT_EQ_FLOAT(z4, 3.0f);
    EXPECT_EQ_FLOAT(w4, 4.0f);
    
    // Test integer to float conversion in vec2
    EXPECT_TRUE(fld_get_vec2(parser.root, "vec2_ints", &x2, &y2));
    EXPECT_EQ_FLOAT(x2, 1.0f);
    EXPECT_EQ_FLOAT(y2, 2.0f);
    
    // Test mixed integer/float in vec3
    EXPECT_TRUE(fld_get_vec3(parser.root, "vec3_mixed", &x3, &y3, &z3));
    EXPECT_EQ_FLOAT(x3, 1.0f);
    EXPECT_EQ_FLOAT(y3, 2.5f);
    EXPECT_EQ_FLOAT(z3, 3.0f);
    
    // Test nested path access
    EXPECT_TRUE(fld_get_vec3(parser.root, "nested.position", &x3, &y3, &z3));
    EXPECT_EQ_FLOAT(x3, 10.0f);
    EXPECT_EQ_FLOAT(y3, 20.0f);
    EXPECT_EQ_FLOAT(z3, 30.0f);
    
    cleanup_parser(memory);
    return true;
}

TEST(Parser, VectorErrors) {
    const char* source = 
        "// Invalid vector sizes\n"
        "vec1 = vec1(1.0);\n"
        "vec5 = vec5(1,2,3,4,5);\n"
        "\n"
        "// Wrong number of components\n"
        "vec2_missing = vec2(1.0);\n"
        "vec2_extra = vec2(1.0, 2.0, 3.0);\n"
        "vec3_missing = vec3(1.0, 2.0);\n"
        "vec4_extra = vec4(1,2,3,4,5);\n"
        "\n"
        "// Invalid component types\n"
        "vec2_invalid = vec2(\"string\", 1.0);\n"
        "vec3_invalid = vec3(true, 1.0, 2.0);\n"
        "\n"
        "// Valid vector for type checking\n"
        "valid_vec3 = vec3(1,2,3);\n";

    fld_parser parser = {0};
    void* memory = NULL;
    
    // Parser should fail with invalid vector syntax
    EXPECT_FALSE(setup_parser(&parser, source, &memory));
    
    // Test separate error cases with individual vectors
    const char* valid_source = "valid_vec3 = vec3(1,2,3);";
    EXPECT_TRUE(setup_parser(&parser, valid_source, &memory));
    
    // Test type mismatches
    float x2, y2;
    EXPECT_FALSE(fld_get_vec2(parser.root, "valid_vec3", &x2, &y2));
    
    float x4, y4, z4, w4;
    EXPECT_FALSE(fld_get_vec4(parser.root, "valid_vec3", &x4, &y4, &z4, &w4));
    
    cleanup_parser(memory);
    return true;
}

TEST(Parser, VectorComponents) {
    const char* source = 
        "pos2 = vec2(1.5, -2.0);\n"
        "pos3 = vec3(1.0, 0.0, -1.0);\n"
        "pos4 = vec4(-1.5, 2.5, 0.0, 1.0);\n";
    
    fld_parser parser = {0};
    void* memory = NULL;
    
    EXPECT_TRUE(setup_parser(&parser, source, &memory));

    // Test getting raw components
    float components[4];
    size_t count;
    
    EXPECT_TRUE(fld_get_vec_components(parser.root, "pos2", components, &count));
    EXPECT_EQ(count, 2);
    EXPECT_EQ_FLOAT(components[0], 1.5f);
    EXPECT_EQ_FLOAT(components[1], -2.0f);
    
    EXPECT_TRUE(fld_get_vec_components(parser.root, "pos3", components, &count));
    EXPECT_EQ(count, 3);
    EXPECT_EQ_FLOAT(components[0], 1.0f);
    EXPECT_EQ_FLOAT(components[1], 0.0f);
    EXPECT_EQ_FLOAT(components[2], -1.0f);
    
    EXPECT_TRUE(fld_get_vec_components(parser.root, "pos4", components, &count));
    EXPECT_EQ(count, 4);
    EXPECT_EQ_FLOAT(components[0], -1.5f);
    EXPECT_EQ_FLOAT(components[1], 2.5f);
    EXPECT_EQ_FLOAT(components[2], 0.0f);
    EXPECT_EQ_FLOAT(components[3], 1.0f);
    
    // Test type checking
    EXPECT_EQ(fld_get_type(parser.root, "pos2"), FLD_VALUE_VEC2);
    EXPECT_EQ(fld_get_type(parser.root, "pos3"), FLD_VALUE_VEC3);
    EXPECT_EQ(fld_get_type(parser.root, "pos4"), FLD_VALUE_VEC4);
    
    cleanup_parser(memory);
    return true;
}

TEST(Parser, VectorEdgeCases) {
    const char* source = 
        "// Test whitespace handling\n"
        "vec2_spaces = vec2 ( 1.0 , 2.0 );\n"
        "vec3_newlines = vec3(\n"
        "    1.0,\n"
        "    2.0,\n"
        "    3.0\n"
        ");\n"
        "// Test zero values\n"
        "vec2_zeros = vec2(0.0, 0.0);\n"
        "// Test large values\n"
        "vec2_large = vec2(999999.0, -999999.0);\n";
    
    fld_parser parser = {0};
    void* memory = NULL;
    
    EXPECT_TRUE(setup_parser(&parser, source, &memory));
    
    float x, y, z;
    
    // Test whitespace handling
    EXPECT_TRUE(fld_get_vec2(parser.root, "vec2_spaces", &x, &y));
    EXPECT_EQ_FLOAT(x, 1.0f);
    EXPECT_EQ_FLOAT(y, 2.0f);
    
    // Test multi-line formatting
    EXPECT_TRUE(fld_get_vec3(parser.root, "vec3_newlines", &x, &y, &z));
    EXPECT_EQ_FLOAT(x, 1.0f);
    EXPECT_EQ_FLOAT(y, 2.0f);
    EXPECT_EQ_FLOAT(z, 3.0f);
    
    // Test zero values
    EXPECT_TRUE(fld_get_vec2(parser.root, "vec2_zeros", &x, &y));
    EXPECT_EQ_FLOAT(x, 0.0f);
    EXPECT_EQ_FLOAT(y, 0.0f);
    
    // Test large values
    EXPECT_TRUE(fld_get_vec2(parser.root, "vec2_large", &x, &y));
    EXPECT_EQ_FLOAT(x, 999999.0f);
    EXPECT_EQ_FLOAT(y, -999999.0f);
    
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

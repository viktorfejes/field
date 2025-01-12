/*
*   vf_test - v0.3
*   Header-only tiny test library for C/C++.
*
*   RECENT CHANGES:
*       0.3     (2025-01-11)    Added more macros;
*                               Implemented cleanup for tests that gets called after tests are run;
*       0.21    (2024-06-24)    ;
*       0.2     (2024-06-21)    Added suite to TEST;
*       0.1     (2024-06-20)    Finalized the implementation;
*
*   LICENSE: MIT License
*       Copyright (c) 2024 Viktor Fejes
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
*       - [ ] Add a timer to time the global and local parts
*       - [ ] Potentially, add ASSERT_ macros for stopping upon failure,
*             while EXPECT_ macros will let the program continue...
*       - [ ] Timer based failure implementation.
*       - [ ] Setup and Teardown sections.
*       - [ ] Add SKIP.
*
*/

#ifndef VF_TEST_H
#define VF_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    #define VF_PLATFORM_WINDOWS 1
#elif defined(__linux__) || defined(__gnu_linux__) || defined(__unix__)
    #define VF_PLATFORM_UNIX 1
#else
    #error "Unsupported platform."
#endif

typedef bool (*PFN_test)(void);

// Test structure
typedef struct vf_test_case {
    const char* suite;
    const char* name;
    PFN_test func;
    struct vf_test_case* next;
} vf_test_case;

#define VF_MAX_LOGS 100
#define VF_MAX_FAIL_MSG_BUFFER 256

typedef struct vf_test_log {
    vf_test_case* test;
    int line;
    const char* file;
    char message[VF_MAX_FAIL_MSG_BUFFER];
} vf_test_log;

typedef struct vf_test_state {
    int test_count;

    // Test case variables
    vf_test_case* current_test;
    vf_test_case* test_head;
    vf_test_case* test_tail;

    // Logs
    int current_test_id;
    vf_test_log logs[VF_MAX_LOGS];
    int log_count;

    // Style variables
    int max_test_name_length;
} vf_test_state;

// Timer struct
typedef struct vf_test_timer {
    double start;
    double elapsed;
} vf_test_timer;

// Main TEST
extern void _vf_register_test(bool (*PFN_test)(void), const char* suite, const char* name);
extern void _vf_run_all_tests(const char* file);
extern void _vf_log_failure(const char* file, size_t line, const char* message);

// Construct on first use initialize state...
static vf_test_state* vf_test_get_state(void) {
    static vf_test_state state = {0};
    static int vfsi = 0;

    if (!vfsi) {
        state.test_count = 0;
        state.current_test = NULL;
        state.test_head = NULL;
        state.test_tail = NULL;
        state.current_test_id = 0;
        state.log_count = 0;
        state.max_test_name_length = 0;

        vfsi = 1;
    }

    return &state;
}

// Macros...
#if defined(__GNUC__) || defined(__clang__)
    #define VF_TEST_REGISTER __attribute__((constructor))
    #define TEST(suite, name)                                                          \
        bool test_func_##suite##name(void);                                            \
        static void test_register_##suite##name(void) VF_TEST_REGISTER;                \
        static void test_register_##suite##name(void) {                                \
            _vf_register_test(test_func_##suite##name, #suite, #name);                         \
        }                                                                       \
        bool test_func_##suite##name(void)
#elif defined(_MSC_VER)
    #define VF_TEST_REGISTER_SECTION __pragma(section(".CRT$XCU", read))
    #define VF_TEST_REGISTER __declspec(allocate(".CRT$XCU"))
    #define TEST(name)                                                          \
        bool test_func_##name(void);                                            \
        static int test_register_##name(void) {                                 \
            _vf_register_test(test_func_##name, #name);                         \
            return 0;                                                           \
        }                                                                       \
        VF_TEST_REGISTER_SECTION VF_TEST_REGISTER int (*test_register_##name##_ptr)(void) = test_register_##name; \
        bool test_func_##name(void)
#else
    #error "Unsupported compiler"
#endif

// #define RUN_TEST()
#define RUN_ALL_TESTS() do {                    \
        _vf_run_all_tests(__FILE__);                    \
    } while(0)

#define EXPECT_TRUE(condition) do {             \
        if(!(condition)) {                      \
            char message[] = "Expected condition to be true, but got false.\0"; \
            _vf_log_failure(__FILE__, __LINE__, message); \
            return false;                       \
        }                                       \
    } while (0)

#define EXPECT_FALSE(condition) \
    do { \
        if((condition)) { \
            char message[] = "Expected condition to be false, but got true.\0"; \
            _vf_log_failure(__FILE__, __LINE__, message); \
            return false; \
        } \
    } while (0)

#define EXPECT_EQ(val1, val2) do { \
        if ((val1) != (val2)) { \
            char message[VF_MAX_FAIL_MSG_BUFFER]; \
            snprintf(message, sizeof(message), \
            "Expected values to be equal. (expected: %s, actual: %s)", \
            #val1, #val2); \
            _vf_log_failure(__FILE__, __LINE__, message); \
            return false; \
        } \
    } while (0)

#define EXPECT_NE(val1, val2) do { \
        if ((val1) == (val2)) { \
            char message[VF_MAX_FAIL_MSG_BUFFER]; \
            snprintf(message, sizeof(message), \
            "Expected values to be NOT equal. (expected: %s, actual: %s)", \
            #val1, #val2); \
            _vf_log_failure(__FILE__, __LINE__, message); \
            return false; \
        } \
    } while (0)

#define EXPECT_GT(val1, val2) do { \
        if ((val1) <= (val2)) { \
            char message[VF_MAX_FAIL_MSG_BUFFER]; \
            snprintf(message, sizeof(message), \
            "Expected value 1 to be GREATER THAN value 2. (expected: %s, actual: %s)", \
            #val1, #val2); \
            _vf_log_failure(__FILE__, __LINE__, message); \
            return false; \
        } \
    } while (0)

#define EXPECT_EQ_INT(expected, actual) do { \
        if ((expected) != (actual)) { \
            char message[VF_MAX_FAIL_MSG_BUFFER]; \
            snprintf(message, sizeof(message), \
            "Expected integer values to be equal. (expected: %s (%d), actual: %s (%d))", \
            #expected, (expected), #actual, (actual)); \
            _vf_log_failure(__FILE__, __LINE__, message); \
            return false; \
        } \
    } while (0)
    
#define EXPECT_NE_INT(expected, actual) do { \
        if ((expected) == (actual)) { \
            char message[VF_MAX_FAIL_MSG_BUFFER]; \
            snprintf(message, sizeof(message), \
            "Expected integer values to be NOT equal. (expected: %s (%d), actual: %s (%d))", \
            #expected, (expected), #actual, (actual)); \
            _vf_log_failure(__FILE__, __LINE__, message); \
            return false; \
        } \
    } while (0)

#define EXPECT_EQ_FLOAT(expected, actual, epsilon) do { \
        float diff = (expected) - (actual); \
        if (diff > epsilon || diff < -epsilon) { \
            char message[VF_MAX_FAIL_MSG_BUFFER]; \
            snprintf(message, sizeof(message), \
            "Expected float values to be equal within epsilon %f. (expected: %s (%f), actual: %s (%f))", \
            epsilon, #expected, (float)(expected), #actual, (float)(actual)); \
            _vf_log_failure(__FILE__, __LINE__, message); \
            return false; \
        } \
    } while (0)

#define EXPECT_NE_FLOAT(expected, actual, epsilon) do { \
        float diff = (expected) - (actual); \
        if (diff <= epsilon && diff >= -epsilon) { \
            char message[VF_MAX_FAIL_MSG_BUFFER]; \
            snprintf(message, sizeof(message), \
            "Expected float values to NOT be equal within epsilon %f. (expected: %s (%f), actual: %s (%f))", \
            epsilon, #expected, (float)(expected), #actual, (float)(actual)); \
            _vf_log_failure(__FILE__, __LINE__, message); \
            return false; \
        } \
    } while (0)

#define EXPECT_EQ_STR(expected, actual) do { \
        if (expected == NULL && actual == NULL) break; \
        if (expected == NULL || actual == NULL || strcmp(expected, actual) != 0) { \
            char message[VF_MAX_FAIL_MSG_BUFFER]; \
            snprintf(message, sizeof(message), \
            "Expected strings to be equal. (expected: %s \"%s\", actual: %s \"%s\")", \
            #expected, (expected ? expected : "NULL"), \
            #actual, (actual ? actual : "NULL")); \
            _vf_log_failure(__FILE__, __LINE__, message); \
            return false; \
        } \
    } while (0)

#define EXPECT_NE_STR(expected, actual) do { \
        if (expected == NULL && actual == NULL) { \
            char message[VF_MAX_FAIL_MSG_BUFFER]; \
            snprintf(message, sizeof(message), \
            "Expected strings to NOT be equal. Both are NULL"); \
            _vf_log_failure(__FILE__, __LINE__, message); \
            return false; \
        } \
        if ((expected != NULL && actual != NULL) && strcmp(expected, actual) == 0) { \
            char message[VF_MAX_FAIL_MSG_BUFFER]; \
            snprintf(message, sizeof(message), \
            "Expected strings to NOT be equal. (expected: %s \"%s\", actual: %s \"%s\")", \
            #expected, expected, #actual, actual); \
            _vf_log_failure(__FILE__, __LINE__, message); \
            return false; \
        } \
    } while (0)

#define EXPECT_NULL(ptr) do { \
        if ((ptr) != NULL) { \
            char message[VF_MAX_FAIL_MSG_BUFFER]; \
            snprintf(message, sizeof(message), \
            "Expected %s to be NULL, but it wasn't", #ptr); \
            _vf_log_failure(__FILE__, __LINE__, message); \
            return false; \
        } \
    } while (0)

#define EXPECT_NOT_NULL(ptr) do { \
        if ((ptr) == NULL) { \
            char message[VF_MAX_FAIL_MSG_BUFFER]; \
            snprintf(message, sizeof(message), \
            "Expected %s to NOT be NULL, but it was", #ptr); \
            _vf_log_failure(__FILE__, __LINE__, message); \
            return false; \
        } \
    } while (0)

#define FAIL(...)
#define SUCCEED(...)
#define SKIP(...)

#ifdef __cplusplus
}
#endif

// End of header...
#ifdef VF_TEST_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>

#if VF_PLATFORM_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif VF_PLATFORM_UNIX
    #include <time.h>
#else
    #error "Unsupported platform"
#endif

#if VF_PLATFORM_WINDOWS
static double timer_frequency;
#endif

static void _cleanup_tests(void);

static double vf_test_platform_get_abs_time(void) {
    #if VF_PLATFORM_WINDOWS
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return (double)now.QuadPart * timer_frequency;
    #elif VF_PLATFORM_UNIX
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        return now.tv_sec + now.tv_nsec * 1e-9;
    #endif
}

static void _timer_start(vf_test_timer* timer) {
    timer->start = vf_test_platform_get_abs_time();
}

static void _timer_stop(vf_test_timer* timer) {
    timer->start = 0;
}

static double _timer_get_elapsed(vf_test_timer* timer) {
    if (timer->start != 0) {
        timer->elapsed = vf_test_platform_get_abs_time() - timer->start;
    }
    return timer->elapsed;
}

static void _print_human_readable_time(double seconds) {
    if (seconds >= 60.0) {
        int minutes = (int)(seconds / 60);
        double remaining_seconds = seconds - (minutes * 60);
        printf("%02d:%06.3f", minutes, remaining_seconds);
    } else {
        printf("%.7fs", seconds);
    }
}

// Register tests in a linked list...
void _vf_register_test(bool (*func)(void), const char* suite, const char* name) {
    vf_test_case* test = (vf_test_case*)malloc(sizeof(vf_test_case));
    test->suite = suite;
    test->name = name;
    test->func = func;

    // Increment test count in state.
    vf_test_get_state()->test_count++;

    int suite_len = strlen(suite);
    int name_len = strlen(name);
    if (name_len + suite_len > vf_test_get_state()->max_test_name_length) {
        vf_test_get_state()->max_test_name_length = name_len + suite_len;
    }

    if (vf_test_get_state()->test_tail) {
        vf_test_get_state()->test_tail->next = test;
    } else {
        vf_test_get_state()->test_head = test;
    }
    vf_test_get_state()->test_tail = test;
    test->next = 0;
}

static void _print_dots(vf_test_case* test) {
    int suite_len = strlen(test->suite);
    int name_len = strlen(test->name);
    int diff = vf_test_get_state()->max_test_name_length - (name_len + suite_len);
    for (int i = 0; i < diff; ++i) {
        printf(".");
    }
}

void _vf_log_failure(const char* file, size_t line, const char* message) {
    // Boundary check
    if (vf_test_get_state()->log_count >= VF_MAX_LOGS) {
        return;
    }

    vf_test_case* current_test = vf_test_get_state()->current_test;

    vf_test_get_state()->logs[vf_test_get_state()->log_count].file = file;
    vf_test_get_state()->logs[vf_test_get_state()->log_count].line = line;
    vf_test_get_state()->logs[vf_test_get_state()->log_count].test = current_test;

    snprintf(vf_test_get_state()->logs[vf_test_get_state()->log_count].message, VF_MAX_FAIL_MSG_BUFFER, "%s", message);

    vf_test_get_state()->log_count++;
}

void _vf_run_all_tests(const char* file) {
    // Get frequency for timer on windows by initializing
    // our static frequency variable.
    #if VF_PLATFORM_WINDOWS
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        timer_frequency = 1.0 / (double)frequency.QuadPart;
    #endif

    unsigned int passed = 0;
    unsigned int failed = 0;
    int count = 0;

    const char* ascii_format[] = {
        "\033[0m",      // reset
        "\033[96m",     // cyan
        "\033[92m",     // green
        "\033[93m",     // yellow
        "\033[91m",     // red

        "\033[1m",      // bold
        "\033[22m",     // bold-reset
    };

    // Init main timer
    vf_test_timer main_timer;
    _timer_start(&main_timer);

    printf("Test project %s - running %d tests\n", file, vf_test_get_state()->test_count);

    vf_test_get_state()->current_test = vf_test_get_state()->test_head;
    while (vf_test_get_state()->current_test) {
        printf("  Test #%d: ", count + 1);

        bool result = vf_test_get_state()->current_test->func();

        if (result == true) {
            printf("%s/%s ", vf_test_get_state()->current_test->suite, vf_test_get_state()->current_test->name);
            _print_dots(vf_test_get_state()->current_test);
            printf("... Passed\n");
            passed++;
        } else {
            printf("%s/%s ", vf_test_get_state()->current_test->suite, vf_test_get_state()->current_test->name);
            _print_dots(vf_test_get_state()->current_test);
            printf("... %sFailed%s\n", ascii_format[4], ascii_format[0]);
            failed++;
        }

        count++;
        vf_test_get_state()->current_test_id++;
        vf_test_get_state()->current_test = vf_test_get_state()->current_test->next;
    }

    double main_elapsed = _timer_get_elapsed(&main_timer);
    _timer_stop(&main_timer); // this is not really needed...
    unsigned int percentage = passed / (float)count * 100.0f;

    if (failed <= 0) {
        printf("\n\033[4mResults:\033[24m %s%d%% tests passed%s, %d tests failed. (Total tests %d) ", ascii_format[2], percentage, ascii_format[0], failed, count);
        _print_human_readable_time(main_elapsed);
        printf("\n");
    } else {
        printf("\n\033[4mResults:\033[24m %d%% tests passed, %s%d tests failed%s. (Total tests %d) ", percentage, ascii_format[4], failed, ascii_format[0], count);
        _print_human_readable_time(main_elapsed);
        printf("\n");

        printf("\nFailure logs:\n");
        for (int i = 0; i < vf_test_get_state()->log_count; ++i) {
            printf("  %s%s.%s (%s:%d) - %s%s\n",
                ascii_format[4],
                vf_test_get_state()->logs[i].test->suite,
                vf_test_get_state()->logs[i].test->name,
                vf_test_get_state()->logs[i].file,
                vf_test_get_state()->logs[i].line,
                vf_test_get_state()->logs[i].message,
                ascii_format[0]);
        }
    }

    _cleanup_tests();
}

static void _cleanup_tests(void) {
    vf_test_case *current = vf_test_get_state()->test_head;
    while (current) {
        vf_test_case *next = current->next;
        free(current);
        current = next;
    }
    vf_test_get_state()->test_head = NULL;
    vf_test_get_state()->test_tail = NULL;
}

#endif // VF_TEST_IMPLEMENTATION
#endif // VF_TEST_H

#ifndef VJUDGE_VJUDGE_H
#define VJUDGE_VJUDGE_H

#include <libvcd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define VJUDGE_MAX_NAME_SIZE 128
#define VJUDGE_MAX_ASSERTIONS_NO 1024
#define VJUDGE_MAX_TESTS_NO 64
#define VJUDGE_MAX_SRC_FILES_NO 32

typedef enum {
    VJUDGE_NO_ERROR = 0,
    VJUDGE_ERROR_OPENING_VCD_FILE,
    VJUDGE_ERROR_COMPILING_VERILOG_FILE,
    VJUDGE_ERROR_ASSERTIONS_FILE_WRONG_FORMAT,
    VJUDGE_ERROR_ASSERTIONS_FILE_NOT_EXISTS,
    VJUDGE_ERROR_OPENING_TEST_DIRECTORY,
    VJUDGE_ERROR_OPENING_SRC_DIRECTORY,
    VJUDGE_ERROR_HANDLING_TEMP_DIRECTORY
} error_t;

typedef struct {
    char *signal_name;
    char *expected_value;
    timestamp_t timestamp;
} assertion_t;

typedef struct {
    assertion_t assertion;
    bool passed;
    char *actual_value;
} assertion_result_t;

typedef struct {
    char *name;
    bool passed;
    size_t passed_assertions_count;
    size_t assertions_count;
    assertion_result_t assertion_results[VJUDGE_MAX_ASSERTIONS_NO];
} test_t;

typedef struct {
    bool passed;
    size_t passed_tests_count;
    size_t tests_count;
    test_t tests[VJUDGE_MAX_TESTS_NO];
    error_t error;
} judge_result_t;

typedef struct {
    char *test_dir_path;
    char *src_dir_path;
} judge_input_t;

void run_judge(judge_input_t *judge_input, judge_result_t *judge_result);

#endif // VJUDGE_VJUDGE_H

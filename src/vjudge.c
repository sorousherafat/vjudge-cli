#define _DEFAULT_SOURCE

#include <dirent.h>
#include <getopt.h>
#include <libvcd.h>
#include <libvjudge.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *const usage =
    "Usage: vjudge [OPTIONS]...\n"
    "A command-line tool for automatic judge of verilog HDL code.\n"
    "\n"
    "Optional arguments:\n"
    "  -t, --test DIR        Path to the tests directory, containing testbenches and assertions (required).\n"
    "  -s, --src DIR         Path to the src directory, containing the src files to judge (required).\n"
    "  -h, --help            Print this help message and exit.\n";

static const struct option options[] = {{"test", required_argument, 0, 't'},
                                        {"src", required_argument, 0, 's'},
                                        {"help", no_argument, 0, 'h'},
                                        {0, 0, 0, 0}};

static char *test_dir_path = NULL;
static char *src_dir_path = NULL;

void parse_input(int argc, char *argv[]);

void check_args_provided(int argc);

void print_judge_result(judge_result_t *result);

char *get_judge_message(error_t error);

int main(int argc, char *argv[]) {
    parse_input(argc, argv);
    check_args_provided(argc);
    judge_input_t input = {.test_dir_path = test_dir_path, .src_dir_path = src_dir_path};
    judge_result_t result;
    run_judge(&input, &result);
    print_judge_result(&result);
}

void parse_input(int argc, char *argv[]) {
    int opt = 0;
    while ((opt = getopt_long(argc, argv, "t:s:h", options, NULL)) != -1) {
        switch (opt) {
        case 't':
            test_dir_path = optarg;
            break;
        case 's':
            src_dir_path = optarg;
            break;
        case 'h':
            printf("%s", usage);
            exit(EXIT_SUCCESS);
        case '?':
            fprintf(stderr, "Unknown option or missing argument: %s\n%s", argv[optind - 1], usage);
            exit(EXIT_FAILURE);
        case ':':
            fprintf(stderr, "Missing argument for option: %s\n%s", argv[optind - 1], usage);
            exit(EXIT_FAILURE);
        default:
            exit(EXIT_FAILURE);
        }
    }
}

void check_args_provided(int argc) {
    bool args_provided = true;

    if (test_dir_path == NULL) {
        fprintf(stderr, "Test directory not provided\n");
        args_provided = false;
    }
    if (src_dir_path == NULL) {
        fprintf(stderr, "Src directory not provided\n");
        args_provided = false;
    }

    if (!args_provided) {
        fprintf(stderr, "\n%s", usage);
        exit(EXIT_FAILURE);
    }
}

void print_judge_result(judge_result_t *result) {
    char *message = get_judge_message(result->error);
    printf("%s\n", message);

    bool passed = result->passed;
    printf("Passed: %s\n", passed ? "True" : "False");
    
    size_t passed_tests_count = result->passed_tests_count;
    size_t tests_count = result->tests_count;
    printf("Passed Tests: %zu/%zu\n", passed_tests_count, tests_count);
    
    test_t *tests = result->tests;
    printf("Tests:\n");
    for (size_t i = 0; i < tests_count; i++) {
        test_t test = tests[i];
        printf("  Passed: %s, Name: %s\n", test.passed ? "True " : "False", test.name);
    }
}

char *get_judge_message(error_t error) {
    switch (error) {
    case VJUDGE_NO_ERROR:
        return "Your code was judged successfully!";
    case VJUDGE_ERROR_COMPILING_VERILOG_FILE:
        return "An error occurred while compiling your Verilog files.";
    case VJUDGE_ERROR_OPENING_SRC_DIRECTORY:
        return "An error occurred while opening the src directory.";
    case VJUDGE_ERROR_OPENING_VCD_FILE:
        return "An error occurred while opening the .vcd file.";
    case VJUDGE_ERROR_OPENING_TEST_DIRECTORY:
        return "An error occurred while opening the test directory.";
    case VJUDGE_ERROR_HANDLING_TEMP_DIRECTORY:
        return "An error occurred while handling the temp directory.";
    case VJUDGE_ERROR_ASSERTIONS_FILE_WRONG_FORMAT:
        return "An error occurred while reading the assertion files.";
    case VJUDGE_ERROR_ASSERTIONS_FILE_NOT_EXISTS:
        return "An error occurred while opening an assertion file.";
    default:
        return "An unknown error occurred.";
    }
}
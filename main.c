#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <getopt.h>
#include "libvcd.h"

#define MAX_NAME_SIZE 128
#define MAX_ASSERTIONS_NO 1024
#define MAX_TESTS_NO 64
#define MAX_SIGNAL_NAME_SIZE 64

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
    char *test_name;
    size_t assertions_count;
    assertion_result_t assertion_results[MAX_ASSERTIONS_NO];
} test_t;

static const char *usage =
        "Usage: vjudge [OPTIONS]... FILE...\n"
        "A command-line tool for automatic judge of verilog HDL code.\n"
        "\n"
        "Positional arguments:\n"
        "  FILE...               Paths to source files.\n"
        "\n"
        "Optional arguments:\n"
        "  -t, --test DIR        Path to the tests directory, containing testbenches and assertions (required).\n"
        "  -v, --verbose         Print verbose output.\n"
        "  -h, --help            Print this help message and exit.\n";

static const struct option options[] = {
        {"test", required_argument, 0, 't'},
        {"verbose", no_argument,    0, 'v'},
        {"help", no_argument,       0, 'h'},
        {0, 0,                      0, 0}
};

static const char *out_file_name = ".tmp.o";
static const char *vcd_file_name = ".tmp.vcd";

static DIR *tests_dir;
static char *tests_dir_path;
static test_t tests[MAX_TESTS_NO];
static size_t tests_count = 0;

static bool check_args_provided(int argc, bool test_flag);

static bool check_files_existence_and_print_error(int argc, char *const *argv, const char *tests_dir_path);

static void print_input(int argc, char *const *argv);

static void parse_input(int argc, char *argv[], bool *test_flag, bool *verbose_flag);

static bool try_get_test_name(char *file_name, char **test_name);

static FILE *open_assertion_file(const struct dirent *tests_dirent, const char *assertion_file_path);

static void write_assertion_file_path(char *assertion_file_path, const char *test_name);

static void read_assertions(test_t *test, FILE *assertion_file);

static void write_assertion(test_t *test, timestamp_t timestamp, char *expected_value, char *signal_name);

int main(int argc, char *argv[]) {
    bool test_flag = false;
    bool verbose_flag = false;
    parse_input(argc, argv, &test_flag, &verbose_flag);

    bool args_provided = check_args_provided(argc, test_flag);
    if (!args_provided) {
        fprintf(stderr, "\n%s", usage);
        return 1;
    }

    bool files_exist = check_files_existence_and_print_error(argc, argv, tests_dir_path);
    if (!files_exist)
        return 1;

    if (verbose_flag) {
        print_input(argc, argv);
    }

    struct dirent *tests_dirent;
    while ((tests_dirent = readdir(tests_dir)) != NULL) {
        if (tests_dirent->d_type != DT_REG)
            continue;

        /* TODO: I don't know man. */
        char *test_name;
        bool is_test = try_get_test_name(tests_dirent->d_name, &test_name);
        if (!is_test)
            continue;

        test_t *test = &tests[tests_count];
        test->test_name = test_name;
        test->assertions_count = 0;

        char *assertion_file_path = (char *) malloc(PATH_MAX * sizeof(char));
        write_assertion_file_path(assertion_file_path, test_name);

        FILE *assertion_file = open_assertion_file(tests_dirent, assertion_file_path);
        read_assertions(test, assertion_file);
    }

//    for (int i = optind; i < argc; i++) {
//        char *src_file_path = argv[i];
//        sprintf(command, "%s %s/%s %s -o %s && ./%s", "iverilog", tests_dir_path, tests_dirent->d_name,
//                src_file_path, out_file_name, out_file_name);
//        system(command);
//    }
}

void read_assertions(test_t *test, FILE *assertion_file) {
    timestamp_t timestamp;
    char *expected_value;
    char *signal_name;
    int result;
    while ((result = fscanf(assertion_file, "%d %m[^=]=%m[^\n]\n", &timestamp, &signal_name, &expected_value)) != EOF) {
        if (result != 3) {
            fprintf(stderr, "An error occurred when trying to read assertion files");
            exit(EXIT_FAILURE);
        }

        write_assertion(test, timestamp, expected_value, signal_name);
    }
}

void write_assertion(test_t *test, timestamp_t timestamp, char *expected_value, char *signal_name) {
    assertion_result_t *assertion_result = &test->assertion_results[test->assertions_count];
    assertion_result->passed = false;
    assertion_t *assertion = &assertion_result->assertion;
    assertion->timestamp = timestamp;
    assertion->expected_value = expected_value;
    assertion->signal_name = signal_name;
    test->assertions_count += 1;
}

void write_assertion_file_path(char *assertion_file_path, const char *test_name) {
    snprintf(assertion_file_path, MAX_NAME_SIZE, "%s/%s-assertion.txt", tests_dir_path, test_name);
}

FILE *open_assertion_file(const struct dirent *tests_dirent, const char *assertion_file_path) {
    FILE *assertion_file;
    if ((assertion_file = fopen(assertion_file_path, "r")) == NULL) {
        fprintf(stderr, "Found '%s/%s' test file but could not open '%s' assertion file\n", tests_dir_path,
                tests_dirent->d_name, assertion_file_path);
        exit(EXIT_FAILURE);
    }
    return assertion_file;
}

void parse_input(int argc, char *argv[], bool *test_flag, bool *verbose_flag) {
    int opt;
    while ((opt = getopt_long(argc, argv, "t:vh", options, NULL)) != -1) {
        switch (opt) {
            case 't':
                (*test_flag) = true;
                tests_dir_path = optarg;
                break;
            case 'v':
                (*verbose_flag) = true;
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

void print_input(int argc, char *const *argv) {
    printf("Tests directory path:   %s\n", tests_dir_path);
    for (int i = optind; i < argc; i++)
        printf("Source file path:       %s\n", argv[i]);
}

bool check_files_existence_and_print_error(int argc, char *const *argv, const char *tests_dir_path) {
    bool files_exist = true;
    if ((tests_dir = opendir(tests_dir_path)) == NULL) {
        fprintf(stderr, "Could not open tests directory: %s\n", tests_dir_path);
        files_exist = false;
    }

    /* TODO: Add checks for files in `tests_dir`. */

    for (int i = optind; i < argc; i++)
        if (access(argv[i], R_OK) == -1) {
            fprintf(stderr, "Could not open source file: %s\n", argv[i]);
            files_exist = false;
        }
    return files_exist;
}

bool check_args_provided(int argc, bool test_flag) {
    bool args_provided = true;
    if (!test_flag) {
        fprintf(stderr, "Test directory not provided\n");
        args_provided = false;
    }

    if (optind == argc) {
        fprintf(stderr, "No source files were provided\n");
        args_provided = false;
    }
    return args_provided;
}

bool try_get_test_name(char *file_name, char **test_name) {
    int result = sscanf(file_name, "%m[^-]-test.v", test_name);
    if (result == 1 && *test_name[0] != '\0' && file_name[result + strlen("-test.v")] != '\0')
        return true;
    return false;
}

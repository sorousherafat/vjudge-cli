#include "libvcd.h"
#include <dirent.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_NAME_SIZE 128
#define MAX_ASSERTIONS_NO 1024
#define MAX_TESTS_NO 64

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
    {"test", required_argument, 0, 't'}, {"verbose", no_argument, 0, 'v'}, {"help", no_argument, 0, 'h'}, {0, 0, 0, 0}};

static const char *out_file_name = ".tmp.o";
static const char *vcd_file_name = ".tmp.vcd";
static bool test_flag = false;
static bool verbose_flag = false;
static DIR *tests_dir;
static char *tests_dir_path;
static test_t tests[MAX_TESTS_NO];
static size_t tests_count = 0;

void check_args_provided(int argc);

void check_files_existence(int argc, char *const *argv);

void print_input_if_verbose(int argc, char *const *argv);

void parse_input(int argc, char *argv[]);

bool try_get_test_name(char *file_name, char **test_name);

FILE *open_assertion_file(const struct dirent *tests_dirent, const char *assertion_file_path);

void write_assertion_file_path(char *assertion_file_path, const char *test_name);

void read_assertions(test_t *test, FILE *assertion_file);

void write_assertion(test_t *test, timestamp_t timestamp, char *expected_value, char *signal_name);

void load_tests();

void read_test_assertions(const struct dirent *tests_dirent, const char *test_name, test_t *test);

void check_input(int argc, char *argv[]);

bool test_exists(const char *test_name);

void test_src_files(int argc, char *const *argv);

void test_src_file(const char *src_file_path);

bool run_test(const char *src_file_path, test_t *test);

void test_clean_up();

void create_vcd_file(const char *src_file_path, const test_t *test);

bool check_assertions(const test_t *test, vcd_t *vcd);

void print_test_passed(const char *src_file_path, const test_t *test);

void print_test_failed(const char *src_file_path, const test_t *test);

void print_failed_assertions_if_verbose(const char *src_file_path, const test_t *test);

bool check_assertion(vcd_t *vcd, assertion_result_t *assertion_result, assertion_t *assertion);

void print_assertion_if_failed(const char *src_file_path, const test_t *test,
                               const assertion_result_t *assertion_result);

test_t *read_test(char *test_name);

void print_src_report(const char *src_file_path, size_t passed_tests_count);

int main(int argc, char *argv[]) {
    parse_input(argc, argv);
    check_input(argc, argv);
    print_input_if_verbose(argc, argv);

    load_tests();

    test_src_files(argc, argv);
}

void test_src_files(int argc, char *const *argv) {
    for (int src_file_index = optind; src_file_index < argc; ++src_file_index) {
        char *src_file_path = argv[src_file_index];
        test_src_file(src_file_path);
    }
}

void test_src_file(const char *src_file_path) {
    size_t passed_tests_count = 0;
    for (int test_index = 0; test_index < tests_count; ++test_index) {
        test_t *test = &tests[test_index];
        passed_tests_count += run_test(src_file_path, test);
        test_clean_up();
    }
    print_src_report(src_file_path, passed_tests_count);
}

void print_src_report(const char *src_file_path, size_t passed_tests_count) {
    printf("src file '%s': %zu/%zu tests passed\n\n", src_file_path, passed_tests_count, tests_count);
}

void test_clean_up() {
    remove(out_file_name);
    remove(vcd_file_name);
}

void create_vcd_file(const char *src_file_path, const test_t *test) {
    char command[PATH_MAX];
    sprintf(command, "%s %s/%s-test.v %s -o %s && ./%s > /dev/null", "iverilog", tests_dir_path, test->name,
            src_file_path, out_file_name, out_file_name);
    system(command);
}

bool check_assertion(vcd_t *vcd, assertion_result_t *assertion_result, assertion_t *assertion) {
    assertion_result->actual_value = get_value_from_vcd(vcd, assertion->signal_name, assertion->timestamp);
    if (strcmp(assertion_result->actual_value, assertion->expected_value) == 0) {
        assertion_result->passed = true;
        return true;
    }

    assertion_result->passed = false;
    return false;
}

bool check_assertions(const test_t *test, vcd_t *vcd) {
    bool test_passed = true;
    for (int assertion_index = 0; assertion_index < test->assertions_count; ++assertion_index) {
        assertion_result_t *assertion_result = (assertion_result_t *)&test->assertion_results[assertion_index];
        assertion_t *assertion = &assertion_result->assertion;
        test_passed &= check_assertion(vcd, assertion_result, assertion);
    }
    return test_passed;
}

bool run_test(const char *src_file_path, test_t *test) {
    create_vcd_file(src_file_path, test);

    vcd_t *vcd = open_vcd((char *)vcd_file_name);
    if (vcd == NULL) {
        fprintf(stderr, "An error occurred when opening .vcd file '%s'", vcd_file_name);
        exit(EXIT_FAILURE);
    }

    bool test_passed = check_assertions(test, vcd);

    if (test_passed) {
        print_test_passed(src_file_path, test);
    } else {
        print_test_failed(src_file_path, test);
        print_failed_assertions_if_verbose(src_file_path, test);
    }

    return test_passed;
}

void print_failed_assertions_if_verbose(const char *src_file_path, const test_t *test) {
    if (!verbose_flag)
        return;

    for (int assertion_index = 0; assertion_index < test->assertions_count; ++assertion_index) {
        const assertion_result_t *assertion_result = &test->assertion_results[assertion_index];
        print_assertion_if_failed(src_file_path, test, assertion_result);
    }
}

void print_assertion_if_failed(const char *src_file_path, const test_t *test,
                               const assertion_result_t *assertion_result) {
    if (!assertion_result->passed) {
        printf("Assertion failed on test '%s' for src file '%s': Expected '%s' but got '%s'\n", test->name,
               src_file_path, assertion_result->assertion.expected_value, assertion_result->actual_value);
    }
}

void print_test_failed(const char *src_file_path, const test_t *test) {
    printf("Test '%s' failed for src file '%s'\n", test->name, src_file_path);
}

void print_test_passed(const char *src_file_path, const test_t *test) {
    printf("Test '%s' passed for src file '%s'\n", test->name, src_file_path);
}

void check_input(int argc, char *argv[]) {
    check_args_provided(argc);
    check_files_existence(argc, argv);
}

void load_tests() {
    struct dirent *tests_dirent;
    while ((tests_dirent = readdir(tests_dir)) != NULL) {
        if (tests_dirent->d_type != DT_REG)
            continue;

        char *test_name;
        if (!try_get_test_name(tests_dirent->d_name, &test_name))
            continue;

        if (test_exists(test_name))
            continue;

        test_t *test = read_test(test_name);

        read_test_assertions(tests_dirent, test_name, test);
    }
}

test_t *read_test(char *test_name) {
    test_t *test = &tests[tests_count];
    test->name = test_name;
    test->assertions_count = 0;
    tests_count += 1;
    return test;
}

bool test_exists(const char *test_name) {
    for (int test_index = 0; test_index < tests_count; ++test_index) {
        test_t *test = &tests[test_index];
        if (strcmp(test->name, test_name) == 0)
            return true;
    }

    return false;
}

void read_test_assertions(const struct dirent *tests_dirent, const char *test_name, test_t *test) {
    char *assertion_file_path = (char *)malloc(PATH_MAX * sizeof(char));
    write_assertion_file_path(assertion_file_path, test_name);

    FILE *assertion_file = open_assertion_file(tests_dirent, assertion_file_path);
    read_assertions(test, assertion_file);
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

void parse_input(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt_long(argc, argv, "t:vh", options, NULL)) != -1) {
        switch (opt) {
        case 't':
            test_flag = true;
            tests_dir_path = optarg;
            break;
        case 'v':
            verbose_flag = true;
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

void print_input_if_verbose(int argc, char *const *argv) {
    if (!verbose_flag)
        return;

    printf("Tests directory path:   %s\n", tests_dir_path);
    for (int i = optind; i < argc; i++)
        printf("Source file path:       %s\n", argv[i]);
}

void check_files_existence(int argc, char *const *argv) {
    bool files_exist = true;
    if ((tests_dir = opendir(tests_dir_path)) == NULL) {
        fprintf(stderr, "Could not open tests directory: %s\n", tests_dir_path);
        files_exist = false;
    }

    for (int i = optind; i < argc; i++)
        if (access(argv[i], R_OK) == -1) {
            fprintf(stderr, "Could not open source file: %s\n", argv[i]);
            files_exist = false;
        }

    if (!files_exist) {
        fprintf(stderr, "\n%s", usage);
        exit(EXIT_FAILURE);
    }
}

void check_args_provided(int argc) {
    bool args_provided = true;
    if (!test_flag) {
        fprintf(stderr, "Test directory not provided\n");
        args_provided = false;
    }

    if (optind == argc) {
        fprintf(stderr, "No source files were provided\n");
        args_provided = false;
    }

    if (!args_provided) {
        fprintf(stderr, "\n%s", usage);
        exit(EXIT_FAILURE);
    }
}

bool try_get_test_name(char *file_name, char **test_name) {
    int result = sscanf(file_name, "%m[^-]-test.v", test_name);
    if (result == 1 && *test_name[0] != '\0' && file_name[result + strlen("-test.v")] != '\0')
        return true;
    return false;
}

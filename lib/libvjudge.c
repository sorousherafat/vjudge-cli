#define _DEFAULT_SOURCE

#include <dirent.h>
#include <libvcd.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <libvjudge.h>

static const char *out_file_name = ".tmp.o";
static const char *vcd_file_name = ".tmp.vcd";

void check_files_existence(judge_input_t *judge_input, DIR **src_dir, DIR **test_dir, judge_result_t *judge_result);

char *get_real_path(char *path, judge_result_t *judge_result, error_t error);

void load_tests(char *test_dir_path, DIR *test_dir, judge_result_t *judge_result);

bool try_get_test_name(char *file_name, char **test_name);

bool test_exists(const char *test_name, test_t *tests, size_t tests_count);

test_t *read_test(char *test_name, judge_result_t *judge_result);

void read_test_assertions(const char *test_dir_path, const struct dirent *tests_dirent, const char *test_name,
                          test_t *test, judge_result_t *judge_result);

void read_assertions(test_t *test, FILE *assertion_file, judge_result_t *judge_result);

void read_assertion(test_t *test, timestamp_t timestamp, char *expected_value, char *signal_name);

void write_assertion_file_path(char *assertion_file_path, const char *test_name, const char *test_dir_path);

FILE *open_assertion_file(const struct dirent *tests_dirent, const char *assertion_file_path,
                          judge_result_t *judge_result);

int load_srcs(const char *src_dir_path, DIR *src_dir, char *src_files_paths[], judge_result_t *judge_result);

bool is_verilog_file_path(const char *path);

void run_tests(const char *test_dir_path, size_t src_files_count, char *src_file_paths[], judge_result_t *judge_result);

char *create_temp_dir(judge_result_t *judge_result);

bool run_test(const char *test_dir_path, size_t src_files_count, char *src_file_paths[], const char *temp_dir_path,
              test_t *test, judge_result_t *judge_result);

void create_vcd_file(const char *test_dir_path, size_t src_files_count, char *src_file_paths[],
                     const char *temp_dir_path, const test_t *test, judge_result_t *judge_result);

vcd_t *read_vcd_file(const char *temp_dir_path, judge_result_t *judge_result);

bool check_assertion(vcd_t *vcd, assertion_result_t *assertion_result, assertion_t *assertion);

size_t count_passed_assertions(const test_t *test, vcd_t *vcd);

void test_clean_up(const char *temp_dir_path);

void set_judge_error(judge_result_t *judge_result, error_t error);

void strjoin(char *dest, char *src[], size_t length, char *delimiter, char *prefix, char *suffix);

void run_judge(judge_input_t *judge_input, judge_result_t *judge_result) {
    judge_result->error = VJUDGE_NO_ERROR;
    judge_result->tests_count = 0;
    judge_result->passed_tests_count = 0;

    DIR *src_dir, *test_dir;
    check_files_existence(judge_input, &src_dir, &test_dir, judge_result);
    if (judge_result->error != VJUDGE_NO_ERROR)
        return;

    char *src_dir_path = get_real_path(judge_input->src_dir_path, judge_result, VJUDGE_ERROR_OPENING_SRC_DIRECTORY);
    if (judge_result->error != VJUDGE_NO_ERROR)
        return;

    char *test_dir_path = get_real_path(judge_input->test_dir_path, judge_result, VJUDGE_ERROR_OPENING_TEST_DIRECTORY);
    if (judge_result->error != VJUDGE_NO_ERROR)
        return;

    load_tests(test_dir_path, test_dir, judge_result);
    if (judge_result->error != VJUDGE_NO_ERROR)
        return;

    int src_files_count;
    char *src_file_paths[VJUDGE_MAX_SRC_FILES_NO];
    src_files_count = load_srcs(src_dir_path, src_dir, src_file_paths, judge_result);
    if (judge_result->error != VJUDGE_NO_ERROR)
        return;

    run_tests(test_dir_path, src_files_count, src_file_paths, judge_result);
    if (judge_result->error != VJUDGE_NO_ERROR)
        return;
}

void check_files_existence(judge_input_t *judge_input, DIR **src_dir, DIR **test_dir, judge_result_t *judge_result) {
    if ((*src_dir = opendir(judge_input->src_dir_path)) == NULL) {
        set_judge_error(judge_result, VJUDGE_ERROR_OPENING_SRC_DIRECTORY);
        return;
    }

    if ((*test_dir = opendir(judge_input->test_dir_path)) == NULL) {
        set_judge_error(judge_result, VJUDGE_ERROR_OPENING_TEST_DIRECTORY);
        return;
    }
}

char *get_real_path(char *path, judge_result_t *judge_result, error_t error) {
    char *full_path = (char *)malloc(PATH_MAX * sizeof(char));
    if (realpath(path, full_path) == NULL) {
        set_judge_error(judge_result, error);
        return NULL;
    }

    return full_path;
}

void load_tests(char *test_dir_path, DIR *test_dir, judge_result_t *judge_result) {
    struct dirent *tests_dirent;
    while ((tests_dirent = readdir(test_dir)) != NULL) {
        char test_file_path[PATH_MAX];
        snprintf(test_file_path, sizeof(test_file_path), "%s/%s", test_dir_path, tests_dirent->d_name);

        struct stat test_stat;
        if (stat(test_file_path, &test_stat) < 0) {
            set_judge_error(judge_result, VJUDGE_ERROR_OPENING_TEST_DIRECTORY);
            return;
        }

        if (S_ISDIR(test_stat.st_mode))
            continue;

        char *test_name;
        if (!try_get_test_name(tests_dirent->d_name, &test_name))
            continue;

        if (test_exists(test_name, judge_result->tests, judge_result->tests_count))
            continue;

        test_t *test = read_test(test_name, judge_result);

        read_test_assertions(test_dir_path, tests_dirent, test_name, test, judge_result);
        if (judge_result->error != VJUDGE_NO_ERROR)
            return;
    }
}

bool try_get_test_name(char *file_name, char **test_name) {
    int result = sscanf(file_name, "%m[^-]-test.v", test_name);
    if (result == 1 && *test_name[0] != '\0' && file_name[result + strlen("-test.v")] != '\0')
        return true;
    return false;
}

bool test_exists(const char *test_name, test_t *tests, size_t tests_count) {
    for (int test_index = 0; test_index < tests_count; ++test_index) {
        test_t *test = &tests[test_index];
        if (strcmp(test->name, test_name) == 0)
            return true;
    }

    return false;
}

test_t *read_test(char *test_name, judge_result_t *judge_result) {
    test_t *test = &judge_result->tests[judge_result->tests_count];
    test->name = test_name;
    test->assertions_count = 0;
    judge_result->tests_count += 1;

    return test;
}

void read_test_assertions(const char *test_dir_path, const struct dirent *tests_dirent, const char *test_name,
                          test_t *test, judge_result_t *judge_result) {
    char *assertion_file_path = (char *)malloc(PATH_MAX * sizeof(char));
    write_assertion_file_path(assertion_file_path, test_name, test_dir_path);

    FILE *assertion_file = open_assertion_file(tests_dirent, assertion_file_path, judge_result);
    if (judge_result->error != VJUDGE_NO_ERROR)
        return;

    read_assertions(test, assertion_file, judge_result);
    if (judge_result->error != VJUDGE_NO_ERROR)
        return;
}

void write_assertion_file_path(char *assertion_file_path, const char *test_name, const char *test_dir_path) {
    snprintf(assertion_file_path, VJUDGE_MAX_NAME_SIZE, "%s/%s-assertion.txt", test_dir_path, test_name);
}

FILE *open_assertion_file(const struct dirent *tests_dirent, const char *assertion_file_path,
                          judge_result_t *judge_result) {
    FILE *assertion_file;
    if ((assertion_file = fopen(assertion_file_path, "r")) == NULL) {
        set_judge_error(judge_result, VJUDGE_ERROR_ASSERTIONS_FILE_NOT_EXISTS);
        return NULL;
    }

    return assertion_file;
}

void read_assertions(test_t *test, FILE *assertion_file, judge_result_t *judge_result) {
    timestamp_t timestamp;
    char *expected_value;
    char *signal_name;
    int result;
    while ((result = fscanf(assertion_file, "%d %m[^=]=%m[^\n]\n", &timestamp, &signal_name, &expected_value)) != EOF) {
        if (result != 3) {
            set_judge_error(judge_result, VJUDGE_ERROR_ASSERTIONS_FILE_WRONG_FORMAT);
            return;
        }

        read_assertion(test, timestamp, expected_value, signal_name);
    }
}

void read_assertion(test_t *test, timestamp_t timestamp, char *expected_value, char *signal_name) {
    assertion_result_t *assertion_result = &test->assertion_results[test->assertions_count];
    assertion_result->passed = false;
    assertion_t *assertion = &assertion_result->assertion;
    assertion->timestamp = timestamp;
    assertion->expected_value = expected_value;
    assertion->signal_name = signal_name;
    test->assertions_count += 1;
}

int load_srcs(const char *src_dir_path, DIR *src_dir, char *src_files_paths[], judge_result_t *judge_result) {
    struct dirent *src_dirent;
    size_t src_files_count = 0;
    while ((src_dirent = readdir(src_dir)) != NULL) {
        char src_file_path[PATH_MAX];
        snprintf(src_file_path, sizeof(src_file_path), "%s/%s", src_dir_path, src_dirent->d_name);

        if (!is_verilog_file_path(src_file_path))
            continue;        

        struct stat src_stat;
        if (stat(src_file_path, &src_stat) < 0) {
            set_judge_error(judge_result, VJUDGE_ERROR_OPENING_SRC_DIRECTORY);
            return -1;
        }

        if (S_ISDIR(src_stat.st_mode))
            continue;

        int src_file_path_length = strlen(src_file_path) + 1;
        if (src_file_path_length < 0) {
            set_judge_error(judge_result, VJUDGE_ERROR_OPENING_SRC_DIRECTORY);
            return -1;
        }
        src_files_paths[src_files_count] = malloc(src_file_path_length * sizeof(char));
        snprintf(src_files_paths[src_files_count], src_file_path_length, "%s", src_file_path);
        src_files_count += 1;
    }
    return src_files_count;
}

bool is_verilog_file_path(const char *path) {
    char *dot = strrchr(path, '.');
    return dot && !strcmp(dot, ".v");
}

void run_tests(const char *test_dir_path, size_t src_files_count, char *src_file_paths[],
               judge_result_t *judge_result) {
    judge_result->passed_tests_count = 0;
    size_t tests_count = judge_result->tests_count;

    char *temp_dir_path = create_temp_dir(judge_result);
    if (judge_result->error != VJUDGE_NO_ERROR)
        return;

    for (int test_index = 0; test_index < tests_count; ++test_index) {
        test_t *test = &judge_result->tests[test_index];
        judge_result->passed_tests_count +=
            run_test(test_dir_path, src_files_count, src_file_paths, temp_dir_path, test, judge_result);
        test_clean_up(temp_dir_path);
        if (judge_result->error != VJUDGE_NO_ERROR) {
            rmdir(temp_dir_path);
            return;
        }
    }

    judge_result->passed = judge_result->passed_tests_count == judge_result->tests_count;

    if (rmdir(temp_dir_path) != 0) {
        set_judge_error(judge_result, VJUDGE_ERROR_HANDLING_TEMP_DIRECTORY);
        return;
    }
}

char *create_temp_dir(judge_result_t *judge_result) {
    char template[] = "/tmp/tmpdir.vjudge-server.XXXXXX";
    char *temp_dir_path = mkdtemp(template);
    if (temp_dir_path == NULL) {
        set_judge_error(judge_result, VJUDGE_ERROR_HANDLING_TEMP_DIRECTORY);
        return NULL;
    }

    char *result = (char *)malloc((strlen(temp_dir_path) + 1) * sizeof(char));
    strcpy(result, temp_dir_path);

    return result;
}

bool run_test(const char *test_dir_path, size_t src_files_count, char *src_file_paths[], const char *temp_dir_path,
              test_t *test, judge_result_t *judge_result) {
    create_vcd_file(test_dir_path, src_files_count, src_file_paths, temp_dir_path, test, judge_result);
    if (judge_result->error != VJUDGE_NO_ERROR)
        return false;

    vcd_t *vcd = read_vcd_file(temp_dir_path, judge_result);
    if (judge_result->error != VJUDGE_NO_ERROR)
        return false;

    test->passed_assertions_count = count_passed_assertions(test, vcd);
    test->passed = test->passed_assertions_count == test->assertions_count;
    return test->passed;
}

void create_vcd_file(const char *test_dir_path, size_t src_files_count, char *src_file_paths[],
                     const char *temp_dir_path, const test_t *test, judge_result_t *judge_result) {
    char *command = malloc(PATH_MAX * (src_files_count + 2) * sizeof(char));
    /* Join all src file paths to a single string & spaces between each path*/
    char *src_files_arg = malloc(PATH_MAX * src_files_count * sizeof(char));
    strjoin(src_files_arg, src_file_paths, src_files_count, "' '", "'", "'");

    sprintf(command, "cd '%s' && %s '%s/%s-test.v' %s -o %s && ./%s > /dev/null", temp_dir_path, "iverilog",
            test_dir_path, test->name, src_files_arg, out_file_name, out_file_name);

    int compilation_exit_code = system(command);
    free(command);
    free(src_files_arg);
    if (compilation_exit_code != 0) {
        set_judge_error(judge_result, VJUDGE_ERROR_COMPILING_VERILOG_FILE);
        return;
    }
}

vcd_t *read_vcd_file(const char *temp_dir_path, judge_result_t *judge_result) {
    char vcd_file_path[PATH_MAX];
    snprintf(vcd_file_path, PATH_MAX, "%s/%s", temp_dir_path, vcd_file_name);
    vcd_t *vcd = libvcd_read_vcd_from_path(vcd_file_path);
    if (vcd == NULL) {
        set_judge_error(judge_result, VJUDGE_ERROR_OPENING_VCD_FILE);
        return false;
    }

    return vcd;
}

bool check_assertion(vcd_t *vcd, assertion_result_t *assertion_result, assertion_t *assertion) {
    assertion_result->actual_value = libvcd_get_signal_value(vcd, assertion->signal_name, assertion->timestamp);
    if (strcmp(assertion_result->actual_value, assertion->expected_value) == 0) {
        assertion_result->passed = true;
        return true;
    }

    assertion_result->passed = false;
    return false;
}

size_t count_passed_assertions(const test_t *test, vcd_t *vcd) {
    size_t passed_assertions = 0;
    for (int assertion_index = 0; assertion_index < test->assertions_count; ++assertion_index) {
        assertion_result_t *assertion_result = (assertion_result_t *)&test->assertion_results[assertion_index];
        assertion_t *assertion = &assertion_result->assertion;
        assertion_result->passed = check_assertion(vcd, assertion_result, assertion);
        passed_assertions += assertion_result->passed;
    }
    return passed_assertions;
}

void test_clean_up(const char *temp_dir_path) {
    char file_path[PATH_MAX];
    snprintf(file_path, PATH_MAX, "%s/%s", temp_dir_path, out_file_name);
    remove(file_path);
    snprintf(file_path, PATH_MAX, "%s/%s", temp_dir_path, vcd_file_name);
    remove(file_path);
}

void set_judge_error(judge_result_t *judge_result, error_t error) {
    judge_result->error = error;
    judge_result->passed = false;
}

void strjoin(char *dest, char *src[], size_t length, char *delimiter, char *prefix, char *suffix) {
    if (length <= 0)
        return;

    strcpy(dest, prefix);
    strcat(dest, src[0]);

    for (size_t i = 1; i < length; i++) {
        strcat(dest, delimiter);
        strcat(dest, src[i]);
    }

    strcat(dest, suffix);
}

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <getopt.h>
#include "libvcd.h"

#define MAX_FILE_NAME_SIZE 128
#define MAX_PATH_SIZE 256
#define MAX_ASSERTIONS_NO 1024
#define MAX_SIGNAL_NAME_SIZE 64


typedef struct {
    char signal_name[MAX_SIGNAL_NAME_SIZE];
    char expected_value;
    timestamp_t timestamp;
} assertion_t;

static DIR *tests_dir;
static assertion_t *assertions;
static uint32_t assertions_count;

static const char* usage =
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
        {"test",    required_argument, 0, 't'},
        {"verbose", no_argument,       0, 'v'},
        {"help",    no_argument,       0, 'h'},
        {0,         0,                 0,   0}
};

static const char *out_file_name = ".tmp.o";
static const char *vcd_file_name = ".tmp.vcd";

int main(int argc, char *argv[])
{
    assertions = (assertion_t *) malloc(MAX_ASSERTIONS_NO * sizeof(assertion_t));
    bool test_flag = false;
    bool verbose_flag = false;
    char *tests_dir_path;

    int opt;
    while ((opt = getopt_long(argc, argv, "t:vh", options, NULL)) != -1)
    {
        switch (opt)
        {
            case 't':
                test_flag = true;
                tests_dir_path = optarg;
                break;
            case 'v':
                verbose_flag = true;
                break;
            case 'h':
                printf("%s", usage);
                return 0;
            case '?':
                fprintf(stderr, "Unknown option or missing argument: %s\n%s", argv[optind - 1], usage);
                return 1;
            case ':':
                fprintf(stderr, "Missing argument for option: %s\n%s", argv[optind - 1], usage);
                return 1;
            default:
                return 1;
        }
    }

    bool args_provided = true;

    if (!test_flag)
    {
        fprintf(stderr, "Test directory not provided\n");
        args_provided = false;
    }

    if (optind == argc)
    {
        fprintf(stderr, "No source files were provided\n");
        args_provided = false;
    }

    if (!args_provided)
    {
        fprintf(stderr, "\n%s", usage);
        return 1;
    }

    /* Check if the given files exist. */
    bool files_exist = true;

    if ((tests_dir = opendir(tests_dir_path)) == NULL)
    {
        fprintf(stderr, "Could not open tests directory: %s\n", tests_dir_path);
        files_exist = false;
    }

    /* TODO: Add checks for files in `tests_dir`. */

    for (int i = optind; i < argc; i++)
        if (access(argv[i], R_OK) == -1)
        {
            fprintf(stderr, "Could not open source file: %s\n", argv[i]);
            files_exist = false;
        }

    /* Return if any of the given files don't exist. */
    if (!files_exist)
        return 1;

    if (verbose_flag)
    {
        printf("Tests directory path:   %s\n", tests_dir_path);
        for (int i = optind; i < argc; i++)
            printf("Source file path:       %s\n", argv[i]);
    }

    assertions_count = 0;
    FILE *assertion_file;
    struct dirent *tests_dirent;
    char *test_name = (char *) malloc(MAX_PATH_SIZE * sizeof(char));
    char *assertion_file_path = (char *) malloc(MAX_PATH_SIZE * sizeof(char));
    char command[3 * MAX_FILE_NAME_SIZE + 10];
    while ((tests_dirent = readdir(tests_dir)) != NULL)
    {
        if (tests_dirent->d_type != DT_REG)
            continue;

        /* TODO: I don't know man. */
        int result = sscanf(tests_dirent->d_name, "%[^-]-test.v", test_name);
        if (result == 1 && test_name[0] != '\0' && tests_dirent->d_name[result + strlen("-test.v")] != '\0')
            continue;

        snprintf(assertion_file_path, MAX_FILE_NAME_SIZE, "%s/%s-assertion.txt", tests_dir_path, test_name);
        if ((assertion_file = fopen(assertion_file_path, "r")) == NULL)
        {
            fprintf(stderr, "Found '%s/%s' test file but could not open '%s' assertion file\n", tests_dir_path, tests_dirent->d_name, assertion_file_path);
            return 1;
        }

        assertions_count = 0;

        timestamp_t timestamp;
        char expected_value;
        char *signal_name = (char *) malloc(MAX_SIGNAL_NAME_SIZE);
        while (fscanf(assertion_file, "%ld %[^=]=%c\n", &timestamp, signal_name, &expected_value) != EOF)
        {
            assertion_t *assertion = &assertions[assertions_count];
            assertion->timestamp = timestamp;
            assertion->expected_value = expected_value;
            strncpy(assertion->signal_name, signal_name, MAX_SIGNAL_NAME_SIZE);
            assertions_count += 1;
        }

        for (int i = optind; i < argc; i++)
        {
            char *src_file_path = argv[i];
            sprintf(command, "%s %s/%s %s -o %s && ./%s", "iverilog", tests_dir_path, tests_dirent->d_name, src_file_path, out_file_name, out_file_name);
            system(command);
        }
    }
}

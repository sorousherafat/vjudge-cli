#include <dirent.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef uint64_t timestamp_t;

typedef struct {
    char *signal_name;
    char signal_value;
    timestamp_t timestamp;
} assertion_t;

static DIR *tests_dir;
static assertion_t *assertions;

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

struct option options[] = {
    {"test",    required_argument, 0, 't'},
    {"verbose", no_argument,       0, 'v'},
    {"help",    no_argument,       0, 'h'},
    {0,         0,                 0,   0}
};


int main(int argc, char *argv[])
{    
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
}


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

static char *test_file_path;
static char *assert_file_path;

static const char* usage =
    "Usage: vjudge [OPTIONS]... FILE...\n"
    "A command-line tool for automatic judge of verilog HDL code.\n\n"
    "Positional arguments:\n"
    "  FILE...               Paths to source files.\n\n"
    "Optional arguments:\n"
    "  -t, --test FILE       Path to the test file (required).\n"
    "  -a, --assert FILE     Path to the assert file (required).\n"
    "  -v, --verbose         Print verbose output.\n"
    "  -h, --help            Print this help message and exit.\n";

struct option options[] = {
    {"test", required_argument, 0, 't'},
    {"assert", required_argument, 0, 'a'},
    {"verbose", no_argument, 0, 'v'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};


int main(int argc, char *argv[])
{    
    bool test_flag = false;
    bool assert_flag = false;
    bool verbose_flag = false;

    int opt;
    while ((opt = getopt_long(argc, argv, "t:a:vh", options, NULL)) != -1)
    {
        switch (opt)
        {
            case 't':
                test_flag = true;
                test_file_path = optarg;
                break;
            case 'a':
                assert_flag = true;
                assert_file_path = optarg;
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
        fprintf(stderr, "Test file not provided\n");
        args_provided = false;
    }

    if (!assert_flag)
    {
        fprintf(stderr, "Assert file not provided\n");
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

    if (access(test_file_path, F_OK) == -1)
    {
        fprintf(stderr, "Test file does not exist: %s\n", test_file_path);
        files_exist = false;
    }
    
    if (access(assert_file_path, F_OK) == -1)
    {
        fprintf(stderr, "Assert file does not exist: %s\n", assert_file_path);
        files_exist = false;
    }

    for (int i = optind; i < argc; i++)
        if (access(argv[i], F_OK) == -1)
        {
            fprintf(stderr, "Source file does not exist: %s\n", argv[i]);
            files_exist = false;
        }

    /* Return if any of the given files don't exist. */
    if (!files_exist)
        return 1;

    if (verbose_flag)
    {
        printf("\n");
        printf("Test file path:   %s\n", test_file_path);
        printf("Assert file path: %s\n", assert_file_path);
        for (int i = optind; i < argc; i++)
            printf("Source file path: %s\n", argv[i]);
        printf("\n");
    }
}

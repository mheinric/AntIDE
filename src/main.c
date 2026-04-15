#include "utils.h"
#include "parser.h"

void print_usage()
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "antide check <filename>\n");
}

int 
main(int argc, char** argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "antide: Wrong number of arguments\n");
        print_usage(); 
        return 1;
    }

    if (strcmp(argv[1], "check") != 0)
    {
        fprintf(stderr, "antide: Unexpected command\n");
        print_usage(); 
        return 1;
    }
    ParseResult result = parse_program_from_file(argv[2]);
    size_t nb_errors = parse_result_nb_errors(&result);
    int status = 0;
    if (nb_errors > 0)
    {
        printf("Errors:\n");
        parse_result_print_errors(&result);
        status = 1;
    }
    else
    {
        printf("File OK\n");
    }
    parse_result_cleanup(&result);
    return status;
}
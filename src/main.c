#include "utils.h"
#include "parser.h"
#include "lsp.h"
#include "debugger.h"
#include "simulation.h"

#include <getopt.h>

void 
print_usage(void)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "antide check <filename>\n");
    fprintf(stderr, "antide run [--map <map_file>] <filename>\n");
    fprintf(stderr, "antide lsp [--stdio]\n");
    fprintf(stderr, "antide dbg\n");
}

typedef enum {
    CMD_CHECK,
    CMD_RUN,
    CMD_LSP,
    CMD_DBG,
    CMD_NO_COMMAND,
} CmdSubCommand;

typedef struct {
    CmdSubCommand type;
    union {
        struct { const char* file_name; } check_args;
        struct { const char* file_name; const char* map_name; MapType map_type; } run_args;
        struct { } lsp_args;
        struct { } dbg_args;
        struct { bool print_version; bool print_help; } no_command_args;
    };
} CmdArgs; 

/// @brief Parse the command line arguments for the 'check' command
/// @return true if the arguments were sucessfully parsed, and false otherwise
bool 
parse_cmd_check_args(int argc, char**argv, CmdArgs* parsed_args)
{
    parsed_args->type = CMD_CHECK;
    if (argc != 3)
    {
        fprintf(stderr, "antide: Too many arguments\n");
        return false;
    }
    parsed_args->check_args.file_name = argv[2];
    return true;
}

/// @brief Parse the command line arguments for the 'run' command
/// @return true if the arguments were sucessfully parsed, and false otherwise
bool 
parse_cmd_run_args(int argc, char**argv, CmdArgs* parsed_args)
{
    parsed_args->type = CMD_RUN;
    static struct option long_options[] = {
        {"map", required_argument, 0, 'm'},
        {"map-type", required_argument, 0, 't'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;
    parsed_args->run_args.file_name = NULL;
    parsed_args->run_args.map_name = NULL;
    parsed_args->run_args.map_type = MAP_TYPE_DEFAULT;
    while ((opt = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'm':
            {
                parsed_args->run_args.file_name = optarg;
                break;
            }
            case 't':
            {
                if (!map_type_deserialization(optarg, &parsed_args->run_args.map_type))
                {
                    fprintf(stderr, "antide: Unexpected map type: %s\n", optarg);
                    return false;
                }
                break;
            }
            default: 
            {
                return false;
            }
        }
    }

    //Two arguments should be left here: run and <program>
    if (optind + 1 > argc - 1) {
        fprintf(stderr, "antide: Missing <program> argument\n");
        return false;
    }
    if (optind + 1 < argc - 1) {
        fprintf(stderr, "antide: Error, too many arguments\n");
        return false;
    }
    parsed_args->run_args.file_name = argv[optind + 1];
    return true;
}

/// @brief Parse the command line arguments for the 'lsp' command
/// @return true if the arguments were sucessfully parsed, and false otherwise
bool
parse_cmd_lsp_args(int argc, char** argv, CmdArgs* parsed_args)
{
    parsed_args->type = CMD_LSP;
    if (argc > 3)
    {
        fprintf(stderr, "antide: Too many arguments\n");
        return false;
    }
    if (argc == 3 && strcmp(argv[2], "--stdio") != 0)
    {
        fprintf(stderr, "antide: Unexpected option %s\n", argv[2]);
        return false;
    }
    return true;
}

/// @brief Parse the command line arguments for the 'dbg' command
/// @return true if the arguments were sucessfully parsed, and false otherwise
bool
parse_cmd_dbg_args(int argc, char** /*argv*/, CmdArgs* parsed_args)
{
    parsed_args->type = CMD_DBG;
    if (argc > 3)
    {
        fprintf(stderr, "antide: Too many arguments\n");
        return false;
    }
    return true;
}

/// @brief Parse the command line arguments when no command is specified (to support the --help or --version arguments)
/// @return true if the arguments were sucessfully parsed, and false otherwise
bool
parse_cmd_no_command_args(int argc, char** argv, CmdArgs* parsed_args)
{
    parsed_args->type = CMD_NO_COMMAND;

    static struct option long_options[] = {
        {"version", no_argument, 0, 'v'},
        {"help"   , no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;
    parsed_args->no_command_args.print_version = false;
    parsed_args->no_command_args.print_help = false;
    while ((opt = getopt_long(argc, argv, "h", long_options, &option_index)) != -1) {
        switch (opt)
        {
            case 'v':
            {
                parsed_args->no_command_args.print_version = true;
                break;
            }
            case 'h':
            {
                parsed_args->no_command_args.print_help = true;
                break;
            }
            case '?':
            {
                return false;
            }
            default:
                break;
        }
    }

    if (optind < argc) 
    {
        fprintf(stderr, "antide: Unexpected argument %s\n", argv[optind]);
        return false;
    }

    return true;
}

bool
parse_cmd_args(int argc, char**argv, CmdArgs* parsed_args)
{
    if (argc < 2)
    {
        fprintf(stderr, "antide: Expected a 'command' argument (one of check, run, lsp, dbg)\n");
        return false;
    }
    if (strcmp(argv[1], "check") == 0)
    {
        return parse_cmd_check_args(argc, argv, parsed_args);
    }
    else if (strcmp(argv[1], "run") == 0)
    {
        return parse_cmd_run_args(argc, argv, parsed_args);
    }
    else if (strcmp(argv[1], "lsp") == 0)
    {
        return parse_cmd_lsp_args(argc, argv, parsed_args);
    }
    else if (strcmp(argv[1], "dbg") == 0)
    {
        return parse_cmd_dbg_args(argc, argv, parsed_args);
    }
    else if (strcmp(argv[1], "inline") == 0)
    {
        return parse_cmd_inline_args(argc, argv, parsed_args);
    }
    else 
    {
        return parse_cmd_no_command_args(argc, argv, parsed_args);
    }
}


int 
main(int argc, char** argv)
{
    CmdArgs parsed_args; 
    if (!parse_cmd_args(argc, argv, &parsed_args))
    {
        print_usage();
        return 1; 
    }

    switch(parsed_args.type)
    {
        case CMD_NO_COMMAND:
        {
            if (parsed_args.no_command_args.print_help)
            {
                print_usage();
            }
            else if (parsed_args.no_command_args.print_version)
            {
                printf("%s\n", ANTIDE_VERSION);
            }
            return 0;
        }
        case CMD_LSP:
        {
            run_lsp(); 
            return 0;
        }
        case CMD_DBG:
        {
            run_debugger(); 
            return 0;
        }
        case CMD_CHECK:
        case CMD_RUN:
        {
            ParseResult result = parse_program_from_file(
                parsed_args.type == CMD_CHECK ? parsed_args.check_args.file_name : 
                parsed_args.run_args.file_name);
            size_t nb_errors = parse_result_nb_errors(&result);
            int status = 0;
            if (nb_errors > 0)
            {
                printf("antide: Errors when reading assembly file:\n");
                parse_result_print_errors(&result);
                status = 1;
                goto main_run_end;
            }
            if (parsed_args.type == CMD_CHECK)
            {
                printf("File OK\n");
                goto main_run_end;
            }
            Program prog; 
            program_init_move(&prog, &result.program);
            GridMap map;
            if (parsed_args.run_args.map_name == NULL)
            {
                const MapSettings map_settings = map_settings_create_default(42);
                grid_map_init(&map, map_settings);
            }
            else
            {
                if (!grid_map_init_from_file(&map, parsed_args.run_args.map_name))
                {
                    printf("antide: Failed to read map file\n");
                    status = 1; 
                    grid_map_cleanup(&map);
                    goto main_run_end;
                }
            }

            SimulationSettings settings = simulation_settings_create_default(42);
            Simulation* sim = simulation_create(settings, prog, map);

            for (int i = 0; i < 2000; i++)
            {
                simulation_run_step(sim);
            }
            printf("Score: %zu/%zu\n", simulation_get_score(sim), simulation_get_max_score(sim));
            simulation_delete(sim);
main_run_end:
            parse_result_cleanup(&result);
            return status;
        }
    }
}
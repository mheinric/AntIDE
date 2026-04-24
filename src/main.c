#include "utils.h"
#include "parser.h"
#include "simulation_private.h" //Necessary for RandomGenerator
#include "lsp.h"
#include "debugger.h"

void 
print_usage(void)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "antide check <filename>\n");
    fprintf(stderr, "antide run <filename>\n");
    fprintf(stderr, "antide lsp\n");
}

int 
main(int argc, char** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "antide: Not enough arguments\n");
        print_usage(); 
        return 1;
    }

    bool is_check = strcmp(argv[1], "check") == 0;
    bool is_run = strcmp(argv[1], "run") == 0;
    bool is_lsp = strcmp(argv[1], "lsp") == 0;
    bool is_dbg = strcmp(argv[1], "dbg") == 0;

    if (is_lsp)
    {
        run_lsp(); 
        return 0;
    }

    if (is_dbg)
    {
        run_debugger(); 
        return 0;
    }

    // For the check and run commands
    if (argc != 3)
    {
        fprintf(stderr, "antide: Not enough arguments\n");
        print_usage(); 
        return 1;
    }

    if (!is_check && !is_run)
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
        if (is_run)
        {
            Program prog; 
            program_init_move(&prog, &result.program);
            const MapSettings map_settings = map_settings_create_default(42);
            GridMap map;
            grid_map_init(&map, map_settings);

            SimulationSettings settings = simulation_settings_create_default(42);
            Simulation* sim = simulation_create(settings, prog, map);

            size_t total_food_amount = 0; 
            for (size_t x = 0; x < map.width; x++)
            {
                for (size_t y = 0; y < map.height; y++)
                {
                    total_food_amount += simulation_get_cell(sim, (Position) { .x = x, .y = y })->food_amount;
                }
            }

            for (int i = 0; i < 2000; i++)
            {
                simulation_run_step(sim);
            }
            printf("Score: %zd/%zd\n", simulation_get_score(sim), total_food_amount);
        }
        else
        {
            printf("File OK\n");
        }
    }
    parse_result_cleanup(&result);
    return status;
}
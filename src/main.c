#include "utils.h"
#include "parser.h"
#include "simulation_private.h" //Necessary for RandomGenerator
#include "lsp.h"

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

    if (is_lsp)
    {
        run_lsp(); 
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
            SimulationSettings settings = simulation_settings_create_default(42);
            Simulation* sim = simulation_create(settings, prog);
            Position start_pos = simulation_get_ant(sim, 0)->position;
            for (int i = -1; i < 2; i++)
            {
                for (int j = -1; j < 2; j++)
                {
                    Position nest_pos = { .x = start_pos.x + i, .y = start_pos.y + j };
                    simulation_get_cell(sim, nest_pos)->type = CELL_TYPE_NEST;
                }
            }

            RandomGenerator rand;
            random_generator_init(&rand, 42); 
            size_t total_food_amount = 0;
            for (int i = 0; i < 1000; i++)
            {
                Position pos = { 
                    .x = random_generator_generate(&rand, settings.width), 
                    .y = random_generator_generate(&rand, settings.height)
                };
                Cell *cell = simulation_get_cell(sim, pos);
                if (cell->type == CELL_TYPE_EMPTY && cell->food_amount < 8)
                {
                    cell->food_amount++; 
                    total_food_amount++;
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
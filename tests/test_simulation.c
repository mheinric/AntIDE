#include "simulation.h"
#include "parser.h"
#include "ast.h"
#include <unity/unity.h>

void 
test_simulation_init(void)
{
    Program prog; 
    program_init(&prog);
    GridMap map; 
    grid_map_init(&map, map_settings_create_test());
    Simulation* sim = simulation_create(simulation_settings_create_test(), prog, map);
    simulation_run_step(sim);
    TEST_ASSERT_EQUAL_UINT64(1, simulation_get_step_number(sim));
    TEST_ASSERT_EQUAL_UINT64(0, simulation_get_ant(sim, 0)->id);
    TEST_ASSERT_EQUAL_INT32(0, simulation_get_ant(sim, 0)->pc);
    TEST_ASSERT_FALSE(simulation_get_ant(sim, 0)->carrying_food);
    for (unsigned i = 0; i < 8; i++)
    {
        TEST_ASSERT_EQUAL_UINT64(0, simulation_get_ant(sim, 0)->registers[i]);
    }
    simulation_delete(sim);
}

static
Program
parse_program(const char* program)
{
    ParseResult parse_result = parse_program_from_string(program);
    if (parse_result_nb_errors(&parse_result) > 0)
    {
        printf("Failed to parse the program below:\n%s\n", program); 
        parse_result_print_errors(&parse_result);
        TEST_FAIL();
    }
    //Moving the program out of parse_result.
    Program prog; 
    program_init_move(&prog, &parse_result.program);
    parse_result_cleanup(&parse_result);
    return prog;
}


Simulation*
create_test_sim(const char* program)
{
    Program prog = parse_program(program);
    GridMap map; 
    grid_map_init(&map, map_settings_create_test());
    return simulation_create(simulation_settings_create_test(), prog, map);
}

void 
test_simulation_move(void)
{
    // Ants can move but not through walls
    Simulation* sim = create_test_sim(
        "MOVE NORTH\n" // Ant will move here
        "MOVE NORTH\n" // Ant will be blocked by wall here
    );
    Position start_pos = simulation_get_ant(sim, 0)->position;
    Position wall_pos = start_pos; 
    wall_pos.y += 2;
    simulation_get_cell(sim, wall_pos)->type = CELL_TYPE_WALL;
    simulation_run_step(sim);
    TEST_ASSERT_EQUAL_INT32(1, simulation_get_ant(sim, 0)->pc);
    TEST_ASSERT_EQUAL_UINT64(start_pos.x, simulation_get_ant(sim, 0)->position.x);
    TEST_ASSERT_EQUAL_UINT64(start_pos.y + 1, simulation_get_ant(sim, 0)->position.y);
    simulation_run_step(sim);
    TEST_ASSERT_EQUAL_INT32(2, simulation_get_ant(sim, 0)->pc);
    TEST_ASSERT_EQUAL_UINT64(start_pos.x, simulation_get_ant(sim, 0)->position.x);
    TEST_ASSERT_EQUAL_UINT64(start_pos.y + 1, simulation_get_ant(sim, 0)->position.y); // Ant has not moved because of wall
    simulation_delete(sim);
}

void 
test_simulation_pickup_drop(void)
{
    Simulation* sim = create_test_sim(
        "PICKUP\n"
        "DROP\n"
    );
    Position pos = simulation_get_ant(sim, 0)->position;
    Cell* cell = simulation_get_cell(sim, pos);
    cell->food_amount = 1;
    simulation_run_step(sim); 
    uint8_t food_amount_step1 = cell->food_amount;
    bool carrying_step1 = simulation_get_ant(sim, 0)->carrying_food; 
    simulation_run_step(sim); 
    uint8_t food_amount_step2 = cell->food_amount;
    bool carrying_step2 = simulation_get_ant(sim, 0)->carrying_food;

    TEST_ASSERT_EQUAL(0, food_amount_step1);
    TEST_ASSERT_TRUE(carrying_step1);
    TEST_ASSERT_EQUAL(1, food_amount_step2);
    TEST_ASSERT_FALSE(carrying_step2);
    simulation_delete(sim);
}

void 
test_simulation_drop_overflow(void)
{
    //There cannot be more than 8 food on a cell. 
    //Any additional amount dropped on it is lost.
    Simulation* sim = create_test_sim("DROP");
    Position pos = simulation_get_ant(sim, 0)->position;
    Cell* cell = simulation_get_cell(sim, pos);
    cell->food_amount = 8;
    simulation_get_ant(sim, 0)->carrying_food = true; 
    simulation_run_step(sim);
    TEST_ASSERT_EQUAL(8, cell->food_amount);
    TEST_ASSERT_FALSE(simulation_get_ant(sim, 0)->carrying_food);
    simulation_delete(sim);
}

void 
test_simulation_drop_nest(void)
{
    // When dropping food on a nest cell, the amount of food on the cell does not increase, but 
    // instead the score is increased by 1.
    Program prog = parse_program(        
        "MOVE NORTH\n"
        "PICKUP\n"
        "MOVE SOUTH\n"
        "DROP\n"
    );

    GridMap map;
    grid_map_init(&map, map_settings_create_test());
    grid_map_get_cell(&map, map.starting_pos)->type = CELL_TYPE_NEST;
    Position north_pos = map.starting_pos;
    north_pos.y += 1;
    grid_map_get_cell(&map, north_pos)->food_amount = 5;

    Simulation* sim = simulation_create(simulation_settings_create_test(), prog, map);
    for (int i = 0; i < 4; i++)
    {
        simulation_run_step(sim);
    }
    TEST_ASSERT_EQUAL(1, simulation_get_score(sim));
    TEST_ASSERT_EQUAL(5, simulation_get_max_score(sim));
    TEST_ASSERT_FALSE(simulation_get_ant(sim, 0)->carrying_food);
    simulation_delete(sim);
}

void
test_simulation_arithmetic(void)
{
    typedef struct { 
        const char* prog; 
        int32_t init_register_values[8]; 
        int32_t expected_register_values[8]; 
    } TestData; 

    const TestData test_data[] = {
        {
            .prog = "SET r5 55", 
            .init_register_values     = { 0, 0, 0, 0, 0, 0, 0, 0 }, 
            .expected_register_values = { 0, 0, 0, 0, 0, 55, 0, 0}
        },
        {
            .prog = "SET r5 r4", 
            .init_register_values     = { 0, 0, 0, 0, 32, 0, 0, 0 }, 
            .expected_register_values = { 0, 0, 0, 0, 32, 32, 0, 0}
        },
        {
            .prog = "ADD r3 10", 
            .init_register_values     = { 0, 0, 0, 10, 0, 0, 0, 0 }, 
            .expected_register_values = { 0, 0, 0, 20, 0, 0, 0, 0 }
        },
        {
            .prog = "SUB r2 50", 
            .init_register_values     = { 0, 0,  25, 0, 0, 0, 0, 0 }, 
            .expected_register_values = { 0, 0, -25, 0, 0, 0, 0, 0 }
        },
        {
            .prog = "MOD r1 10", 
            .init_register_values     = { 0, 13, 0, 0, 0, 0, 0, 0 }, 
            .expected_register_values = { 0,  3, 0, 0, 0, 0, 0, 0 }
        },
        {
            .prog = "MOD r1 0",  //Should be a no-op
            .init_register_values     = { 0, 13, 0, 0, 0, 0, 0, 0 }, 
            .expected_register_values = { 0, 13, 0, 0, 0, 0, 0, 0 }
        },
        {
            .prog = "MOD r1 10", // Result should be > 0 even if r1 is < 0
            .init_register_values     = { 0, -13, 0, 0, 0, 0, 0, 0 }, 
            .expected_register_values = { 0,   7, 0, 0, 0, 0, 0, 0 }
        },
        {
            .prog = "MUL r1 10",
            .init_register_values     = { 0,  -13, 0, 0, 0, 0, 0, 0 }, 
            .expected_register_values = { 0, -130, 0, 0, 0, 0, 0, 0 }
        },
        {
            .prog = "DIV r1 10",
            .init_register_values     = { 0, 13, 0, 0, 0, 0, 0, 0 }, 
            .expected_register_values = { 0,  1, 0, 0, 0, 0, 0, 0 }
        },
        {
            .prog = "DIV r1 0", // Division by zero is no-op
            .init_register_values     = { 0, 13, 0, 0, 0, 0, 0, 0 }, 
            .expected_register_values = { 0, 13, 0, 0, 0, 0, 0, 0 }
        },
        {
            .prog = "DIV r1 10", // Division rounds towards 0
            .init_register_values     = { 0, -19, 0, 0, 0, 0, 0, 0 }, 
            .expected_register_values = { 0,  -1, 0, 0, 0, 0, 0, 0 }
        },
        {
            .prog = "AND r0 r1",
            .init_register_values     = { 0b101010, 0b110011, 0, 0, 0, 0, 0, 0 }, 
            .expected_register_values = { 0b100010, 0b110011, 0, 0, 0, 0, 0, 0 }
        },
        {
            .prog = "OR r0 r1",
            .init_register_values     = { 0b101010, 0b110011, 0, 0, 0, 0, 0, 0 }, 
            .expected_register_values = { 0b111011, 0b110011, 0, 0, 0, 0, 0, 0 }
        },
        {
            .prog = "XOR r0 r1",
            .init_register_values     = { 0b101010, 0b110011, 0, 0, 0, 0, 0, 0 }, 
            .expected_register_values = { 0b011001, 0b110011, 0, 0, 0, 0, 0, 0 }
        },
        {
            .prog = "LSHIFT r0 2",
            .init_register_values     = { 0b00101010, 0, 0, 0, 0, 0, 0, 0 }, 
            .expected_register_values = { 0b10101000, 0, 0, 0, 0, 0, 0, 0 }
        },
        {
            .prog = "RSHIFT r0 2",
            .init_register_values     = { 0b101010, 0, 0, 0, 0, 0, 0, 0 }, 
            .expected_register_values = { 0b001010, 0, 0, 0, 0, 0, 0, 0 }
        },
        {
            .prog = "RSHIFT r0 1", //RSHIFT preserves the sign
            .init_register_values     = { ~0b11, 0, 0, 0, 0, 0, 0, 0 }, 
            .expected_register_values = { ~0b01, 0, 0, 0, 0, 0, 0, 0 }
        },
    };
    const int NB_TEST = sizeof(test_data) / sizeof(test_data[0]);

    for (int i = 0; i < NB_TEST; i++)
    {
        const char* prog = test_data[i].prog;
        const int32_t *init_register_values = test_data[i].init_register_values;
        const int32_t *expected_register_values = test_data[i].expected_register_values;
        Simulation* sim = create_test_sim(test_data[i].prog);
        for (int reg_index = 0; reg_index < 8; reg_index++)
        {
            simulation_get_ant(sim, 0)->registers[reg_index] = init_register_values[reg_index];
        }
        simulation_run_step(sim);
        for (int reg_index = 0; reg_index < 8; reg_index++)
        {
            if (simulation_get_ant(sim, 0)->registers[reg_index] != expected_register_values[reg_index])
            {
                printf("Program: %s\nInvalid register value for r%d expected %d got %d\n",
                    prog, reg_index, expected_register_values[reg_index], 
                    simulation_get_ant(sim, 0)->registers[reg_index]);
            }
        }
        TEST_ASSERT_EQUAL_INT32(1, simulation_get_ant(sim, 0)->pc);
        simulation_delete(sim);
    }
}

void
test_simulation_random_instruction(void)
{
    Simulation* sim = create_test_sim(
        "main:\n"
        "RANDOM r0 10\n"
        "RANDOM r1 -10\n"
        "PICKUP\n"
        "JMP main\n"
    );
    for (int i = 0; i < 1000; i++)
    {
        simulation_run_step(sim);
        TEST_ASSERT_GREATER_OR_EQUAL(0, simulation_get_ant(sim, 0)->registers[0]);
        TEST_ASSERT_LESS_OR_EQUAL(9, simulation_get_ant(sim, 0)->registers[0]);
        TEST_ASSERT_LESS_OR_EQUAL(0, simulation_get_ant(sim, 0)->registers[1]);
        TEST_ASSERT_GREATER_OR_EQUAL(-9, simulation_get_ant(sim, 0)->registers[1]);
    }
    simulation_delete(sim);
}

void
test_simulation_loop(void)
{
    const char* program = 
        "main:\n"
        "ADD r0 1\n"
        "PICKUP\n"
        "JMP main\n"
    ;
    Simulation* sim = create_test_sim(program);
    for (int i = 0; i < 10; i++)
    {
        simulation_run_step(sim);
        TEST_ASSERT_EQUAL(2, simulation_get_ant(sim, 0)->pc);
        TEST_ASSERT_EQUAL(i + 1, simulation_get_ant(sim, 0)->registers[0]);
    }
    simulation_delete(sim);
}

void
test_simulation_conditional_jumps(void)
{
    const char* program = 
        "main:\n"
        "ADD r0 1\n"
        "JEQ r0 10 end\n"
        "JMP main\n"
        "end:\n"
    ;
    Simulation* sim = create_test_sim(program);
    simulation_run_step(sim);
    TEST_ASSERT_EQUAL(3, simulation_get_ant(sim, 0)->pc);
    TEST_ASSERT_EQUAL(10, simulation_get_ant(sim, 0)->registers[0]);
    simulation_delete(sim);
}

void
test_simulation_call(void)
{
    const char* program = 
        "main:\n"
        "CALL r0 end\n"
        "JMP main\n"
        "end:\n"
    ;
    Simulation* sim = create_test_sim(program);
    simulation_run_step(sim);
    TEST_ASSERT_EQUAL(2, simulation_get_ant(sim, 0)->pc);
    TEST_ASSERT_EQUAL(1, simulation_get_ant(sim, 0)->registers[0]);
    simulation_delete(sim);
}


void 
test_simulation_id(void)
{
    Program prog; 
    program_init(&prog);
    program_push_instruction(&prog, instruction_create_info(INST_ID, 0), 0);
    SimulationSettings settings = simulation_settings_create_test();
    settings.nb_ants = 10;
    GridMap map; 
    grid_map_init(&map, map_settings_create_test());
    Simulation* sim = simulation_create(settings, prog, map);
    simulation_run_step(sim);
    for (size_t i = 0; i < settings.nb_ants; i++)
    {
        TEST_ASSERT_EQUAL_INT32(1, simulation_get_ant(sim, i)->pc);
        TEST_ASSERT_EQUAL_INT32((int32_t) i, simulation_get_ant(sim, i)->registers[0]);
    }
    simulation_delete(sim);
}

void 
test_simulation_carrying(void)
{
    const char* program = "CARRYING r0";
    Simulation* sim = create_test_sim(program);
    simulation_get_ant(sim, 0)->carrying_food = true;
    simulation_run_step(sim);
    TEST_ASSERT_EQUAL(1, simulation_get_ant(sim, 0)->pc);
    TEST_ASSERT_EQUAL(1, simulation_get_ant(sim, 0)->registers[0]); 
    simulation_delete(sim);
}

void 
test_simulation_mark(void)
{
    const char* program = 
        "main:\n"
        "MARK CH_RED 100\n"
        "PICKUP\n"
        "JMP main\n"
    ;
    Simulation* sim = create_test_sim(program);
    simulation_get_ant(sim, 0)->carrying_food = true;
    Position pos = simulation_get_ant(sim, 0)->position;
    const Cell* cell= simulation_get_cell(sim, pos);
    simulation_run_step(sim);
    TEST_ASSERT_EQUAL(2, simulation_get_ant(sim, 0)->pc);
    TEST_ASSERT_EQUAL_UINT8(100, cell->pheromones[CH_RED]);
    simulation_run_step(sim);
    TEST_ASSERT_EQUAL(2, simulation_get_ant(sim, 0)->pc);
    TEST_ASSERT_EQUAL_UINT8(199, cell->pheromones[CH_RED]); // The -1 comes from the decay at each step
    simulation_run_step(sim);
    TEST_ASSERT_EQUAL(2, simulation_get_ant(sim, 0)->pc);
    TEST_ASSERT_EQUAL_UINT8(255, cell->pheromones[CH_RED]);
    simulation_delete(sim);
}

void 
test_simulation_sniff_smell(void)
{
    const char* program = 
        "SNIFF CH_RED NORTH r0\n"
        "SMELL CH_RED r2\n"
        "MOVE NORTH\n"
        "SNIFF CH_RED HERE r1\n"
        "SMELL CH_BLUE r3\n"
    ;
    Simulation* sim = create_test_sim(program);
    Position pos = simulation_get_ant(sim, 0)->position;
    Cell* north_cell = simulation_get_neighbor_cell(sim, pos, DIR_NORTH);
    north_cell->pheromones[CH_RED] = 200;
    Cell* east_cell = simulation_get_neighbor_cell(sim, pos, DIR_EAST);
    east_cell->pheromones[CH_RED] = 220;
    simulation_run_step(sim);
    simulation_run_step(sim);
    TEST_ASSERT_EQUAL_INT32(199, simulation_get_ant(sim, 0)->registers[0]);
    TEST_ASSERT_EQUAL_INT32(198, simulation_get_ant(sim, 0)->registers[1]);
    TEST_ASSERT_EQUAL_INT32(DIR_EAST, simulation_get_ant(sim, 0)->registers[2]);
    TEST_ASSERT_EQUAL_INT32(0, simulation_get_ant(sim, 0)->registers[3]);
    simulation_delete(sim);
}

void 
test_simulation_probe(void)
{
    const char* program = 
        "PROBE NORTH r0\n"
        "PROBE EAST r1\n"
        "PROBE SOUTH r2\n"
        "PROBE WEST r3\n"
    ;
    Simulation* sim = create_test_sim(program);
    Position pos = simulation_get_ant(sim, 0)->position;
    simulation_get_neighbor_cell(sim, pos, DIR_NORTH)->food_amount = 1;
    simulation_get_neighbor_cell(sim, pos, DIR_EAST)->type = CELL_TYPE_WALL;
    simulation_get_neighbor_cell(sim, pos, DIR_SOUTH)->type = CELL_TYPE_NEST;
    simulation_run_step(sim);
    TEST_ASSERT_EQUAL_UINT32(ENT_FOOD, simulation_get_ant(sim, 0)->registers[0]);
    TEST_ASSERT_EQUAL_UINT32(ENT_WALL, simulation_get_ant(sim, 0)->registers[1]);
    TEST_ASSERT_EQUAL_UINT32(ENT_NEST, simulation_get_ant(sim, 0)->registers[2]);
    TEST_ASSERT_EQUAL_UINT32(ENT_EMPTY, simulation_get_ant(sim, 0)->registers[3]);
    simulation_delete(sim);
}

void 
test_simulation_sense(void)
{
    const char* program = 
        "SENSE FOOD r0\n"
        "SENSE WALL r1\n"
        "SENSE NEST r2\n"
    ;
    Simulation* sim = create_test_sim(program);
    Position pos = simulation_get_ant(sim, 0)->position;
    simulation_get_neighbor_cell(sim, pos, DIR_NORTH)->food_amount = 1;
    simulation_get_neighbor_cell(sim, pos, DIR_EAST)->type = CELL_TYPE_WALL;
    simulation_get_neighbor_cell(sim, pos, DIR_SOUTH)->type = CELL_TYPE_NEST;
    simulation_run_step(sim);
    TEST_ASSERT_EQUAL_UINT32(DIR_NORTH, simulation_get_ant(sim, 0)->registers[0]);
    TEST_ASSERT_EQUAL_UINT32(DIR_EAST, simulation_get_ant(sim, 0)->registers[1]);
    TEST_ASSERT_EQUAL_UINT32(DIR_SOUTH, simulation_get_ant(sim, 0)->registers[2]);
    simulation_delete(sim);
}

void 
test_simulation_sense_ants(void)
{
    Program prog; 
    program_init(&prog);
    program_push_instruction(&prog, instruction_create_sense(argument_create_value(ENT_ANT), 0), 0);
    SimulationSettings settings = simulation_settings_create_test();
    settings.nb_ants = 2;
    GridMap map; 
    grid_map_init(&map, map_settings_create_test());
    Simulation* sim = simulation_create(settings, prog, map);
    Position north_pos = simulation_get_ant(sim, 0)->position;
    north_pos.y += 1;
    simulation_set_ant_position(sim, simulation_get_ant(sim, 1), north_pos);
    simulation_run_step(sim);
    TEST_ASSERT_EQUAL(DIR_NORTH, simulation_get_ant(sim, 0)->registers[0]);
    TEST_ASSERT_EQUAL(DIR_SOUTH, simulation_get_ant(sim, 1)->registers[0]);
    simulation_delete(sim);
}

void 
test_simulation_tags(void)
{
    const char* program = 
        "main:\n"
        "TAG 5\n"
        "ADD r0 1\n"
        "JMP main\n"
    ;
    Simulation* sim = create_test_sim(program);
    simulation_run_step(sim);
    TEST_ASSERT_EQUAL_UINT8(5, simulation_get_ant(sim, 0)->tag);
    // since TAG has no cost, each loop contains 2 'real' instructions, and runs 32 times per step.
    TEST_ASSERT_EQUAL_INT32(32, simulation_get_ant(sim, 0)->registers[0]);
    simulation_delete(sim);
}

void 
test_simulation_run_single_instruction(void)
{
    const char* program = 
        "main:\n"
        "ADD r0 1\n"
        "MOVE r0\n"
        "JMP main\n"
    ;
    Program prog = parse_program(program); 

    SimulationSettings settings = simulation_settings_create_test();
    settings.nb_ants = 2;
    GridMap map; 
    grid_map_init(&map, map_settings_create_test());
    Simulation* sim = simulation_create(settings, prog, map);

    Ant* ant0 = simulation_get_ant(sim, 0);
    Ant* ant1 = simulation_get_ant(sim, 1);

    //ant0 stepping through its instructions
    simulation_run_single_instruction(sim);
    TEST_ASSERT_EQUAL(1, ant0->pc);
    TEST_ASSERT_EQUAL(0, ant1->pc);
    simulation_run_single_instruction(sim);
    TEST_ASSERT_EQUAL(2, ant0->pc);
    TEST_ASSERT_EQUAL(0, ant1->pc);

    //ant1 stepping through its instructions
    simulation_run_single_instruction(sim);
    TEST_ASSERT_EQUAL(2, ant0->pc);
    TEST_ASSERT_EQUAL(1, ant1->pc);    simulation_run_single_instruction(sim);
    TEST_ASSERT_EQUAL(2, ant0->pc);
    TEST_ASSERT_EQUAL(2, ant1->pc);    
    
    //Next simulation step
    TEST_ASSERT_EQUAL(1, simulation_get_step_number(sim));
    //ant0 stepping through its instructions again
    simulation_run_single_instruction(sim);
    TEST_ASSERT_EQUAL(0, ant0->pc);
    TEST_ASSERT_EQUAL(2, ant1->pc);    simulation_run_single_instruction(sim);
    TEST_ASSERT_EQUAL(1, ant0->pc);
    TEST_ASSERT_EQUAL(2, ant1->pc);

    simulation_delete(sim);
}

void 
test_simulation_breakpoint(void)
{
    const char* program = 
        "main:\n"
        "ADD r0 1\n"
        "MOVE r0\n"
        "JMP main\n"
    ;
    Simulation* sim = create_test_sim(program);
    Program* prog = simulation_get_program(sim);
    program_set_breakpoint(prog, 1); 
    Ant* ant = simulation_get_ant(sim, 0);

    //Simulation step must stop on breakpoint
    simulation_run_step(sim);
    TEST_ASSERT_EQUAL(1, ant->pc);
    TEST_ASSERT_TRUE(simulation_stopped_on_breakpoint(sim));
    //Running the simulation step again continues the simulation and skips the breakpoint.
    simulation_run_step(sim);
    TEST_ASSERT_EQUAL(2, ant->pc);
    TEST_ASSERT_FALSE(simulation_stopped_on_breakpoint(sim));
    //This will break again on the same breakpoint
    simulation_run_step(sim);
    TEST_ASSERT_EQUAL(1, ant->pc);
    TEST_ASSERT_TRUE(simulation_stopped_on_breakpoint(sim));

    program_clear_breakpoints(prog);
    //There is no more breakpoints
    simulation_run_step(sim);
    TEST_ASSERT_EQUAL(2, ant->pc);
    TEST_ASSERT_FALSE(simulation_stopped_on_breakpoint(sim));
    simulation_run_step(sim);
    TEST_ASSERT_EQUAL(2, ant->pc);
    TEST_ASSERT_FALSE(simulation_stopped_on_breakpoint(sim));
    simulation_run_step(sim);
    TEST_ASSERT_EQUAL(2, ant->pc);
    TEST_ASSERT_FALSE(simulation_stopped_on_breakpoint(sim));
    simulation_delete(sim);
}

int 
run_all_simulation_tests(void) 
{
    UNITY_BEGIN();
    RUN_TEST(test_simulation_init);
    RUN_TEST(test_simulation_move);
    RUN_TEST(test_simulation_pickup_drop);
    RUN_TEST(test_simulation_drop_overflow);
    RUN_TEST(test_simulation_drop_nest);
    RUN_TEST(test_simulation_arithmetic);
    RUN_TEST(test_simulation_random_instruction);
    RUN_TEST(test_simulation_loop);
    RUN_TEST(test_simulation_conditional_jumps);
    RUN_TEST(test_simulation_call);
    RUN_TEST(test_simulation_id);
    RUN_TEST(test_simulation_carrying);
    RUN_TEST(test_simulation_mark);
    RUN_TEST(test_simulation_sniff_smell);
    RUN_TEST(test_simulation_probe);
    RUN_TEST(test_simulation_sense);
    RUN_TEST(test_simulation_sense_ants);
    RUN_TEST(test_simulation_tags);
    RUN_TEST(test_simulation_run_single_instruction);
    RUN_TEST(test_simulation_breakpoint);
    return UNITY_END();
}
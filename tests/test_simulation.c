#include "simulation.h"
#include "parser.h"
#include "ast.h"
#include <unity/unity.h>

void 
test_simulation_init(void)
{
    Program prog; 
    program_init(&prog);
    Simulation sim; 
    simulation_init(&sim, simulation_settings_create_test(), prog);
    simulation_run_step(&sim);
    TEST_ASSERT_EQUAL_UINT64(1, sim.step_number);
    TEST_ASSERT_EQUAL_UINT64(0, sim.ants[0].id);
    TEST_ASSERT_EQUAL_INT32(0, sim.ants[0].pc);
    TEST_ASSERT_FALSE(sim.ants[0].carrying_food);
    for (unsigned i = 0; i < 8; i++)
    {
        TEST_ASSERT_EQUAL_UINT64(0, sim.ants[0].registers[i]);
    }
}

void 
test_simulation_single_step_move(void)
{
    Program prog; 
    program_init(&prog);
    // MOVE NORTH
    program_push_instruction(&prog, instruction_create_move(argument_create_value(1)));
    Simulation sim; 
    simulation_init(&sim, simulation_settings_create_test(), prog);
    Position start_pos = sim.ants[0].position;
    simulation_run_step(&sim);
    Position end_pos = sim.ants[0].position;
    TEST_ASSERT_EQUAL_INT32(1, sim.ants[0].pc);
    TEST_ASSERT_EQUAL_UINT64(start_pos.x, end_pos.x);
    TEST_ASSERT_EQUAL_UINT64(start_pos.y + 1, end_pos.y);
}

Simulation
create_test_sim(const char* program)
{
    ParseResult parse_result = parse_program_from_string(program);
    if (vector_parse_error_size(&parse_result.errors) > 0)
    {
        printf("Failed to parse the program below:\n%s\n", program); 
        parse_result_print_errors(&parse_result);
        TEST_FAIL();
    }
    vector_parse_error_cleanup(&parse_result.errors);
    Simulation sim; 
    simulation_init(&sim, simulation_settings_create_test(), parse_result.program);
    return sim;
}

void 
test_simulation_pickup_drop(void)
{
    Simulation sim = create_test_sim(
        "PICKUP\n"
        "DROP\n"
    );
    Position pos = sim.ants[0].position;
    Cell* cell = simulation_get_cell(&sim, pos);
    cell->type = CELL_TYPE_FOOD;
    simulation_run_step(&sim); 
    CellType cell_type_step1 = cell->type;
    bool carrying_step1 = sim.ants[0].carrying_food; 
    simulation_run_step(&sim); 
    CellType cell_type_step2 = cell->type;
    bool carrying_step2 = sim.ants[0].carrying_food;

    TEST_ASSERT_EQUAL(CELL_TYPE_EMPTY, cell_type_step1);
    TEST_ASSERT_TRUE(carrying_step1);
    TEST_ASSERT_EQUAL(CELL_TYPE_FOOD, cell_type_step2);
    TEST_ASSERT_FALSE(carrying_step2);
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
        Simulation sim = create_test_sim(test_data[i].prog);
        for (int reg_index = 0; reg_index < 8; reg_index++)
        {
            sim.ants[0].registers[reg_index] = init_register_values[reg_index];
        }
        simulation_run_step(&sim);
        for (int reg_index = 0; reg_index < 8; reg_index++)
        {
            if (sim.ants[0].registers[reg_index] != expected_register_values[reg_index])
            {
                printf("Program: %s\nInvalid register value for r%d expected %d got %d\n",
                    prog, reg_index, expected_register_values[reg_index], 
                    sim.ants[0].registers[reg_index]);
            }
        }
        TEST_ASSERT_EQUAL_INT32(1, sim.ants[0].pc);
    }
}

void
test_simulation_random_instruction(void)
{
    RandomGenerator r; 
    random_generator_init(&r, 42); 
    int32_t rand_value = random_generator_generate(&r, 10);
    Simulation sim = create_test_sim("RANDOM r0 10");
    simulation_run_step(&sim);
    TEST_ASSERT_EQUAL_INT32(rand_value, sim.ants[0].registers[0]);
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
    Simulation sim = create_test_sim(program);
    for (int i = 0; i < 10; i++)
    {
        simulation_run_step(&sim);
        TEST_ASSERT_EQUAL(2, sim.ants[0].pc);
        TEST_ASSERT_EQUAL(i + 1, sim.ants[0].registers[0]);
    }
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
    Simulation sim = create_test_sim(program);
    simulation_run_step(&sim);
    TEST_ASSERT_EQUAL(3, sim.ants[0].pc);
    TEST_ASSERT_EQUAL(10, sim.ants[0].registers[0]);
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
    Simulation sim = create_test_sim(program);
    simulation_run_step(&sim);
    TEST_ASSERT_EQUAL(2, sim.ants[0].pc);
    TEST_ASSERT_EQUAL(1, sim.ants[0].registers[0]);
}


void 
test_simulation_id(void)
{
    Program prog; 
    program_init(&prog);
    program_push_instruction(&prog, instruction_create_info(INST_ID, 0));
    SimulationSettings settings = simulation_settings_create_test();
    settings.nb_ants = 10;
    Simulation sim; 
    simulation_init(&sim, settings, prog);
    simulation_run_step(&sim);
    for (size_t i = 0; i < settings.nb_ants; i++)
    {
        TEST_ASSERT_EQUAL_INT32(1, sim.ants[i].pc);
        TEST_ASSERT_EQUAL_INT32((int32_t) i, sim.ants[i].registers[0]);
    }
}

void 
test_simulation_carrying(void)
{
    const char* program = "CARRYING r0";
    Simulation sim = create_test_sim(program);
    sim.ants[0].carrying_food = true;
    simulation_run_step(&sim);
    TEST_ASSERT_EQUAL(1, sim.ants[0].pc);
    TEST_ASSERT_EQUAL(1, sim.ants[0].registers[0]); 
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
    Simulation sim = create_test_sim(program);
    sim.ants[0].carrying_food = true;
    Position pos = sim.ants[0].position;
    const Cell* cell= simulation_get_cell(&sim, pos);
    simulation_run_step(&sim);
    TEST_ASSERT_EQUAL(2, sim.ants[0].pc);
    TEST_ASSERT_EQUAL_UINT8(100, cell->pheromones[CH_RED]);
    simulation_run_step(&sim);
    TEST_ASSERT_EQUAL(2, sim.ants[0].pc);
    TEST_ASSERT_EQUAL_UINT8(199, cell->pheromones[CH_RED]); // The -1 comes from the decay at each step
    simulation_run_step(&sim);
    TEST_ASSERT_EQUAL(2, sim.ants[0].pc);
    TEST_ASSERT_EQUAL_UINT8(255, cell->pheromones[CH_RED]);
}

void 
test_simulation_sniff_smell(void)
{
    const char* program = 
        "SNIFF CH_RED NORTH r0\n"
        "SMELL CH_RED r2\n"
        "MOVE NORTH\n"
        "SNIFF CH_RED HERE r1\n"
    ;
    Simulation sim = create_test_sim(program);
    Position pos = sim.ants[CH_RED].position;
    Cell* north_cell = simulation_get_neighbor_cell(&sim, pos, DIR_NORTH);
    north_cell->pheromones[CH_RED] = 200;
    Cell* east_cell = simulation_get_neighbor_cell(&sim, pos, DIR_EAST);
    east_cell->pheromones[CH_RED] = 220;
    simulation_run_step(&sim);
    simulation_run_step(&sim);
    TEST_ASSERT_EQUAL_UINT32(199, sim.ants[0].registers[0]);
    TEST_ASSERT_EQUAL_UINT32(198, sim.ants[0].registers[1]);
    TEST_ASSERT_EQUAL_UINT32(DIR_EAST, sim.ants[0].registers[2]);
}

int 
run_all_simulation_tests(void) 
{
    UNITY_BEGIN();
    RUN_TEST(test_simulation_init);
    RUN_TEST(test_simulation_single_step_move);
    RUN_TEST(test_simulation_pickup_drop);
    RUN_TEST(test_simulation_arithmetic);
    RUN_TEST(test_simulation_random_instruction);
    RUN_TEST(test_simulation_loop);
    RUN_TEST(test_simulation_conditional_jumps);
    RUN_TEST(test_simulation_call);
    RUN_TEST(test_simulation_id);
    RUN_TEST(test_simulation_carrying);
    RUN_TEST(test_simulation_mark);
    RUN_TEST(test_simulation_sniff_smell);
    return UNITY_END();
}
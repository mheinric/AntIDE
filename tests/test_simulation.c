#include "unity/unity.h"
#include "simulation.h"
#include "parser.h"
#include "ast.h"

void 
test_simulation_init(void)
{
    Program prog; 
    Program_init(&prog);
    Simulation sim; 
    simulation_init(&sim, prog);
    simulation_run_single_step(&sim);
    TEST_ASSERT_EQUAL_UINT64(1, sim.step_number);
    TEST_ASSERT_EQUAL_UINT64(0, sim.ants[0].id);
    TEST_ASSERT_EQUAL_UINT64(0, sim.ants[0].pc);
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
    Program_init(&prog);
    // MOVE NORTH
    Program_push_instruction(&prog, instruction_create_move(argument_create_value(1)));
    Simulation sim; 
    simulation_init(&sim, prog);
    Position start_pos = sim.ants[0].position;
    simulation_run_single_step(&sim);
    Position end_pos = sim.ants[0].position;
    TEST_ASSERT_EQUAL_UINT64(1, sim.ants[0].pc);
    TEST_ASSERT_EQUAL_UINT64(start_pos.x, end_pos.x);
    TEST_ASSERT_EQUAL_UINT64(start_pos.y + 1, end_pos.y);
}

Simulation
create_test_sim(const char* program)
{
    ParseResult parse_result = parse_program_from_string(program);
    TEST_ASSERT_EQUAL_UINT64(0, vector_parse_error_size(&parse_result.errors));
    vector_parse_error_clear(&parse_result.errors);
    Simulation sim; 
    simulation_init(&sim, parse_result.program);
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
    simulation_run_single_step(&sim); 
    CellType cell_type_step1 = cell->type;
    bool carrying_step1 = sim.ants[0].carrying_food; 
    simulation_run_single_step(&sim); 
    CellType cell_type_step2 = cell->type;
    bool carrying_step2 = sim.ants[0].carrying_food;

    TEST_ASSERT_EQUAL(CELL_TYPE_EMPTY, cell_type_step1);
    TEST_ASSERT_TRUE(carrying_step1);
    TEST_ASSERT_EQUAL(CELL_TYPE_FOOD, cell_type_step2);
    TEST_ASSERT_FALSE(carrying_step2);
}

int 
run_all_simulation_tests(void) 
{
    UNITY_BEGIN();
    RUN_TEST(test_simulation_init);
    RUN_TEST(test_simulation_single_step_move);
    RUN_TEST(test_simulation_pickup_drop);
    return UNITY_END();
}
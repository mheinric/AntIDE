#pragma once
#include "ast.h"
#include "utils.h"

typedef struct {
    size_t x; 
    size_t y;
} Position;

typedef enum {
    CELL_TYPE_EMPTY = 0,
    CELL_TYPE_FOOD,
} CellType;

typedef struct {
    CellType type;
    Position position;
} Cell; 

typedef struct {
    size_t id;
    int32_t pc;
    int32_t registers[8];
    Position position;
    bool carrying_food;
    uint8_t instruction_budget;
} Ant;

int32_t 
ant_get_arg_value(Ant *ant, Argument arg);

typedef struct {
    uint64_t state;
} RandomGenerator;

void
random_generator_init(RandomGenerator *rand, uint64_t seed);

int32_t 
random_generator_generate(RandomGenerator *rand, int32_t max_bound);


typedef struct {
    size_t step_number;
    Ant* ants;
    Cell* cells;
    Program program;
    size_t width; 
    size_t height;
    RandomGenerator random_generator;
} Simulation;

void
simulation_init(Simulation *sim, Program prog);

void
simulation_run_step(Simulation *sim);

Cell* 
simulation_get_cell(Simulation* sim, Position pos);
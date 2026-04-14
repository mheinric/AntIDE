#pragma once
#include "ast.h"
#include "utils.h"

typedef struct {
    size_t x; 
    size_t y;
} Position;

typedef enum {
    CELL_TYPE_EMPTY = 0,
    CELL_TYPE_WALL,
    CELL_TYPE_NEST,
} CellType;

typedef struct {
    CellType type;
    Position position;
    uint8_t pheromones[4];
    uint8_t food_amount;
    size_t nb_ants;
} Cell; 

bool
cell_matches_entity(Cell *cell, EntityType entity_type);

typedef struct {
    int32_t id;
    int32_t pc;
    int32_t registers[8];
    Position position;
    bool carrying_food;
    uint8_t instruction_budget;
    uint8_t tag;
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
    size_t random_seed; 
    size_t width; 
    size_t height;
    size_t nb_ants;
} SimulationSettings;

SimulationSettings
simulation_settings_create_default(size_t random_seed);

SimulationSettings
simulation_settings_create_test();

typedef struct {
    SimulationSettings settings;
    size_t step_number;
    Ant* ants;
    Cell* cells;
    Program program;
    size_t width; 
    size_t height;
    size_t score;
    RandomGenerator random_generator;
} Simulation;

void
simulation_init(Simulation *sim, SimulationSettings settings, Program prog);

void
simulation_run_step(Simulation *sim);

Cell* 
simulation_get_cell(Simulation* sim, Position pos);

Cell* 
simulation_get_neighbor_cell(Simulation* sim, Position pos, Direction dir);

void
simulation_set_ant_position(Simulation *sim, Ant *ant, Position new_pos);
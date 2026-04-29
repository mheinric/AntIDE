#pragma once
#include "ast.h"
#include "map.h"
#include "utils.h"
#include <cJSON/cJSON.h>

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
    size_t random_seed; 
    size_t nb_ants;
} SimulationSettings;

SimulationSettings
simulation_settings_create_default(size_t random_seed);

SimulationSettings
simulation_settings_create_test();

struct Simulation; 
typedef struct Simulation Simulation;

Simulation*
simulation_create(SimulationSettings settings, Program prog, GridMap map);

void 
simulation_delete(Simulation *sim);

cJSON*
simulation_to_json(Simulation *sim);

void
simulation_run_step(Simulation *sim);

void
simulation_run_single_instruction(Simulation* sim);

// Returns the id of the ant that will execute the next
// instruction in the simulation
size_t
simulation_get_next_running_ant(Simulation* sim);

Program*
simulation_get_program(Simulation* sim);

size_t 
simulation_get_step_number(Simulation* sim);

size_t 
simulation_get_score(Simulation* sim);

Cell* 
simulation_get_cell(Simulation* sim, Position pos);

Cell* 
simulation_get_neighbor_cell(Simulation* sim, Position pos, Direction dir);

size_t
simulation_get_nb_ants(Simulation *sim);

Ant* 
simulation_get_ant(Simulation *sim, int32_t id);

const char*
simulation_get_tag_name(Simulation *sim, uint8_t tag_id);

void
simulation_set_ant_position(Simulation *sim, Ant *ant, Position new_pos);

bool
simulation_stopped_on_breakpoint(Simulation* sim);
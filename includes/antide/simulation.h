#pragma once
#include "ast.h"
#include "map.h"
#include "utils.h"
/// Datastructures and methods for creating, running and inspecting ant colony simulations.

bool
cell_matches_entity(Cell *cell, EntityType entity_type);

typedef struct {
    int32_t id;
    int32_t pc;
    int32_t registers[8];
    Position position;
    bool carrying_food;
    uint8_t instruction_budget; // How many instructions the can can still run for the current step
    uint8_t tag;
} Ant;

int32_t 
ant_get_arg_value(const Ant *ant, Argument arg);

cJSON*
ant_to_json(const Ant* ant);

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
simulation_to_json(const Simulation *sim);

void
simulation_run_step(Simulation *sim);

void
simulation_run_single_instruction(Simulation* sim);

/// @brief Returns the id of the ant that will execute the next
/// instruction in the simulation
size_t
simulation_get_next_running_ant(const Simulation* sim);

Program*
simulation_get_program(Simulation* sim);
 
size_t 
simulation_get_step_number(const Simulation* sim);

size_t 
simulation_get_score(const Simulation* sim);

/// @brief Returns the total amount of food that was present on the map initially 
size_t 
simulation_get_max_score(const Simulation* sim);

Cell* 
simulation_get_cell(Simulation* sim, Position pos);

Cell* 
simulation_get_neighbor_cell(Simulation* sim, Position pos, Direction dir);

size_t
simulation_get_nb_ants(const Simulation *sim);

Ant* 
simulation_get_ant(Simulation *sim, int32_t id);

const char*
simulation_get_tag_name(const Simulation *sim, uint8_t tag_id);

void
simulation_set_ant_position(Simulation *sim, Ant *ant, Position new_pos);

/// @brief Returns true if the last call to simulation_run_step/simulation_run_single_instruction
/// was stopped by a breakpoint set on the program.
/// In the case of simulation_run_single_instruction, this means that no instruction was actually
/// exectuted, you have to run the function again if you want to just skip the breakpoint.
bool
simulation_stopped_on_breakpoint(const Simulation* sim);
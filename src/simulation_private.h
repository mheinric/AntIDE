#include "simulation.h"
#include "internals/rng.h"

struct Simulation {
    SimulationSettings settings;
    size_t step_number;
    size_t next_ant_id; 
    bool step_started;
    Ant* ants;
    Program program;
    size_t score;
    RandomGenerator random_generator;
    GridMap map;
};

void 
simulation_ant_run_single_instruction(Simulation *sim, Ant *ant, Instruction inst);

void
simulation_ant_run_step(Simulation *sim, Ant *ant);

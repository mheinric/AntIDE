#include "simulation.h"
#include "internals/rng.h"

struct Simulation {
    SimulationSettings settings;
    size_t step_number;
    size_t next_ant_id; 
    bool step_started;
    bool skip_next_breakpoint;
    Ant* ants;
    Program program;
    size_t max_score;
    size_t score;
    RandomGenerator random_generator;
    GridMap map;
};

void 
simulation_ant_run_single_instruction(Simulation *sim, Ant *ant, Instruction inst);

void
simulation_ant_run_step(Simulation *sim, Ant *ant);

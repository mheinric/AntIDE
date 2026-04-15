#include "simulation.h"

typedef struct {
    uint64_t state;
} RandomGenerator;

void
random_generator_init(RandomGenerator *rand, uint64_t seed);

void
random_generator_cleanup(RandomGenerator *rand);

int32_t 
random_generator_generate(RandomGenerator *rand, int32_t max_bound);

struct Simulation {
    SimulationSettings settings;
    size_t step_number;
    Ant* ants;
    Cell* cells;
    Program program;
    size_t width; 
    size_t height;
    size_t score;
    RandomGenerator random_generator;
};

void 
simulation_ant_run_single_instruction(Simulation *sim, Ant *ant, Instruction inst);

void
simulation_ant_run_step(Simulation *sim, Ant *ant);

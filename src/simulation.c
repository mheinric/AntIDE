#include "simulation.h"

int32_t ant_get_arg_value(Ant *ant, Argument arg)
{
    if (arg.is_register)
    {
        return ant->registers[arg.register_index];
    }
    else 
    {
        return arg.value;
    }
}

void random_generator_init(RandomGenerator *rand, uint64_t seed)
{
    rand->state = seed;
}

int32_t random_generator_generate(RandomGenerator *rand, int32_t max_bound)
{
    // Values taken from https://en.wikipedia.org/wiki/Linear_congruential_generator from the MUSL implementation
    rand->state = rand->state * 6364136223846793005 + 1;
    // we return the upper bits
    return (rand->state >> 32) % max_bound; //Note when max_bound is negative we get negative values.
}

SimulationSettings simulation_settings_create_default(size_t random_seed)
{
    return (SimulationSettings) {
        .random_seed = random_seed,
        .width = 128, 
        .height = 128,
        .nb_ants = 200,
    };
}

SimulationSettings simulation_settings_create_test()
{
    return (SimulationSettings) {
        .random_seed = 42,
        .width = 10, 
        .height = 10,
        .nb_ants = 1,
    };
}

void simulation_init(Simulation *sim, SimulationSettings settings, Program prog)
{
    sim->settings = settings;
    sim->step_number = 0;
    sim->ants = calloc(settings.nb_ants, sizeof(Ant));
    for (size_t i = 0; i < settings.nb_ants; i++)
    {
        sim->ants[i].id = (int32_t) i;
        sim->ants[i].position.x = (settings.width - 1) / 2; 
        sim->ants[i].position.y = (settings.height -1) / 2; 
    }
    sim->program = prog;
    sim->width = settings.width;
    sim->height = settings.height;
    sim->cells = calloc(sim->width * sim->height, sizeof(Cell));
    for (size_t y = 0; y < sim->height; y++)
    {
        for (size_t x = 0; x < sim->width; x++)
        {
            sim->cells[sim->width * y + x].position.x = x;
            sim->cells[sim->width * y + x].position.y = x;
        }
    }
    random_generator_init(&sim->random_generator, settings.random_seed);
}

void simulation_ant_run_single_instruction(Simulation *sim, Ant *ant, Instruction inst)
{
    ant->pc += 1;
    switch(inst.type) 
    {
        case INST_MOVE: 
        {
            int32_t dir = ant_get_arg_value(ant, inst.move_args.dir);
            ant->position.x += dir == 2 ? 1 : dir == 4 ? -1 : 0;
            ant->position.y += dir == 1 ? 1 : dir == 3 ? -1 : 0;
            break;
        }
        case INST_PICKUP: 
        {
            Cell* cell = simulation_get_cell(sim, ant->position);
            if (ant->carrying_food || cell->food_amount == 0)
            {
                //Nothing to do
                break;
            }
            cell->food_amount--; 
            ant->carrying_food = true;
            break;
        }
        case INST_DROP:
        {
            if (!ant->carrying_food)
            {
                break;
            }
            Cell* cell = simulation_get_cell(sim, ant->position);
            cell->food_amount += 1;
            if (cell->food_amount > 8)
            {
                cell->food_amount = 8;
            }
            ant->carrying_food = false;
            break;
        }
        case INST_SET: 
        {
            ant->registers[inst.arith_args.target_register] = ant_get_arg_value(ant, inst.arith_args.arg);
            break;
        }
        case INST_ADD:
        {
            ant->registers[inst.arith_args.target_register] += ant_get_arg_value(ant, inst.arith_args.arg);
            break;
        }
        case INST_SUB:
        {
            ant->registers[inst.arith_args.target_register] -= ant_get_arg_value(ant, inst.arith_args.arg);
            break;
        }
        case INST_MOD:
        {
            int32_t mod_value = ant_get_arg_value(ant, inst.arith_args.arg);
            if (mod_value != 0)
            {
                ant->registers[inst.arith_args.target_register] %= mod_value;
                if (ant->registers[inst.arith_args.target_register] < 0)
                {
                    ant->registers[inst.arith_args.target_register] += abs(mod_value);
                }
            }
            break;
        }
        case INST_MUL:
        {
            ant->registers[inst.arith_args.target_register] *= ant_get_arg_value(ant, inst.arith_args.arg);
            break;
        }
        case INST_DIV:
        {
            int32_t divisor = ant_get_arg_value(ant, inst.arith_args.arg);
            if (divisor != 0)
            {
                ant->registers[inst.arith_args.target_register] /= divisor;
            } 
            break;
        }
        case INST_AND:
        {
            ant->registers[inst.arith_args.target_register] &= ant_get_arg_value(ant, inst.arith_args.arg);
            break;
        }
        case INST_OR:
        {
            ant->registers[inst.arith_args.target_register] |= ant_get_arg_value(ant, inst.arith_args.arg);
            break;
        }
        case INST_XOR:
        {
            ant->registers[inst.arith_args.target_register] ^= ant_get_arg_value(ant, inst.arith_args.arg);
            break;
        }
        case INST_LSHIFT:
        {
            ant->registers[inst.arith_args.target_register] <<= ant_get_arg_value(ant, inst.arith_args.arg);
            break;
        }
        case INST_RSHIFT:
        {
            ant->registers[inst.arith_args.target_register] >>= ant_get_arg_value(ant, inst.arith_args.arg);
            break;
        }
        case INST_RANDOM:
        {
            int32_t max_bound = ant_get_arg_value(ant, inst.arith_args.arg);
            if (max_bound != 0)
            {
                ant->registers[inst.arith_args.target_register] = random_generator_generate(&sim->random_generator, max_bound);
            }
            break;
        }
        case INST_JMP: 
        {
            int32_t target = ant_get_arg_value(ant, inst.jmp_arg.target);
            ant->pc = target;
            break;
        }
        case INST_JEQ:
        {
            int32_t cond_value1 = ant_get_arg_value(ant, inst.cond_jmp_arg.cond_value1); 
            int32_t cond_value2 = ant_get_arg_value(ant, inst.cond_jmp_arg.cond_value2);
            if (cond_value1 == cond_value2)
            {
                ant->pc = ant_get_arg_value(ant, inst.cond_jmp_arg.target);
            }
            break;
        }
        case INST_JNE: 
        {
            int32_t cond_value1 = ant_get_arg_value(ant, inst.cond_jmp_arg.cond_value1); 
            int32_t cond_value2 = ant_get_arg_value(ant, inst.cond_jmp_arg.cond_value2);
            if (cond_value1 != cond_value2)
            {
                ant->pc = ant_get_arg_value(ant, inst.cond_jmp_arg.target);
            }
            break;
        }
        case INST_JGT: 
        {
            int32_t cond_value1 = ant_get_arg_value(ant, inst.cond_jmp_arg.cond_value1); 
            int32_t cond_value2 = ant_get_arg_value(ant, inst.cond_jmp_arg.cond_value2);
            if (cond_value1 > cond_value2)
            {
                ant->pc = ant_get_arg_value(ant, inst.cond_jmp_arg.target);
            }
            break;
        }
        case INST_JLT: 
        {
            int32_t cond_value1 = ant_get_arg_value(ant, inst.cond_jmp_arg.cond_value1); 
            int32_t cond_value2 = ant_get_arg_value(ant, inst.cond_jmp_arg.cond_value2);
            if (cond_value1 < cond_value2)
            {
                ant->pc = ant_get_arg_value(ant, inst.cond_jmp_arg.target);
            }
            break;
        }
        case INST_CALL: 
        {
            int32_t target_address = ant_get_arg_value(ant, inst.call_arg.target); 
            ant->registers[inst.call_arg.return_register] = ant->pc; // ant->pc already points to the next instruction at this point 
            ant->pc = target_address;
            break;
        }
        case INST_ID: 
        {
            ant->registers[inst.info_arg.target_register] = ant->id;
            break;
        }
        case INST_CARRY: 
        {
            ant->registers[inst.info_arg.target_register] = ant->carrying_food;
            break;
        }
        case INST_MARK:
        {
            int32_t channel = ant_get_arg_value(ant, inst.mark_args.channel); 
            int32_t amount = ant_get_arg_value(ant, inst.mark_args.amount);
            Cell* cell = simulation_get_cell(sim, ant->position);
            if (amount > 0 && channel >= 0 && channel < 4)
            {
                bool overflow = (amount > 255) || (amount + cell->pheromones[channel] > 255); // The first condition is to avoid an int32_t overflow
                if (overflow)
                {
                    cell->pheromones[channel] = 255;
                }
                else
                {
                    cell->pheromones[channel] += amount;
                }
            }
            break;
        }
        case INST_SNIFF:
        {
            int32_t channel = ant_get_arg_value(ant, inst.sniff_args.channel);
            int32_t direction = ant_get_arg_value(ant, inst.sniff_args.direction);
            if (channel >= 0 && channel < 4 && direction_is_valid(direction))
            {
                Cell* cell = simulation_get_neighbor_cell(sim, ant->position, (Direction) direction);
                ant->registers[inst.sniff_args.target_reg] = cell->pheromones[channel];
            }
            break;
        }
        case INST_SMELL:
        {
            int32_t channel = ant_get_arg_value(ant, inst.smell_args.channel);
            uint8_t target_register = inst.smell_args.target_reg;
            if (channel < 0 || channel >= 4)
            {
                //Invalid channel, nothing to do
                break;
            }
            int32_t found_directions[4]; 
            uint8_t nb_found = 0;
            uint8_t max_found = 0;
            for (int dir = DIR_NORTH; dir <= DIR_WEST; dir++)
            {
                Cell* cell = simulation_get_neighbor_cell(sim, ant->position, (Direction) dir);
                if (cell->pheromones[channel] > max_found)
                {
                    found_directions[0] = dir; 
                    nb_found = 1;
                    max_found = cell->pheromones[channel];
                }
                else if (cell->pheromones[channel] == max_found)
                {
                    found_directions[nb_found] = dir; 
                    nb_found++;
                }
            }
            if (nb_found == 0)
            {
                ant->registers[target_register] = 0;
            }
            else if (nb_found == 1)
            {
                ant->registers[target_register] = found_directions[0];
            }
            else 
            {
                ant->registers[target_register] = found_directions[random_generator_generate(&sim->random_generator, nb_found)];
            }
            break;
        }
    }
    if (inst.type == INST_MOVE || inst.type == INST_PICKUP || inst.type == INST_DROP)
    {
        ant->instruction_budget = 0;
    }
    else
    {
        ant->instruction_budget--;
    }
}

void
simulation_ant_run_step(Simulation *sim, Ant *ant)
{
    while (ant->instruction_budget > 0 && ant->pc >= 0 && (uint32_t) ant->pc < program_size(&sim->program))
    {
        Instruction inst = sim->program.instructions.begin[ant->pc];
        simulation_ant_run_single_instruction(sim, ant, inst);
    }
}


void 
simulation_run_step(Simulation *sim)
{
    sim->step_number++;
    // Decay the pheromones
    for (Cell* it = sim->cells; it != sim->cells + sim->width * sim->height; it++)
    {
        for (int i = 0; i < 4; i++)
        {
            if (it->pheromones[i] > 0)
            {
                it->pheromones[i]--;
            }
        }
    }

    // Simulate the ants
    for (size_t i = 0; i < sim->settings.nb_ants; i++)
    {
        sim->ants[i].instruction_budget = 64;
        simulation_ant_run_step(sim, &sim->ants[i]);
    }
}

Cell*
simulation_get_cell(Simulation *sim, Position pos)
{
    return &sim->cells[sim->width * pos.y + pos.x];
}

Cell*
simulation_get_neighbor_cell(Simulation *sim, Position pos, Direction dir)
{
    switch(dir)
    {
        case DIR_HERE: break; 
        case DIR_NORTH: pos.y += 1; break;
        case DIR_EAST: pos.x += 1; break;
        case DIR_SOUTH: pos.y -= 1; break;
        case DIR_WEST: pos.x -= 1; break; 
        case DIR_RANDOM: return simulation_get_neighbor_cell(sim, pos, (Direction) (1 + random_generator_generate(&sim->random_generator, 4)));
    }
    return simulation_get_cell(sim, pos);
}

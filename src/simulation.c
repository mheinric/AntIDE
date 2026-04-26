#include "simulation_private.h"
#include "simulation.h"

bool 
cell_matches_entity(Cell *cell, EntityType entity_type)
{
    switch (entity_type)
    {
        case ENT_EMPTY: return cell->type == CELL_TYPE_EMPTY && cell->food_amount == 0;
        case ENT_WALL: return cell->type == CELL_TYPE_WALL;
        case ENT_FOOD: return cell->type == CELL_TYPE_EMPTY && cell->food_amount >0;
        case ENT_NEST: return cell->type == CELL_TYPE_NEST;
        case ENT_ANT: return cell->nb_ants > 0;
    }
    return false;
}

int32_t ant_get_arg_value(Ant *ant, Argument arg)
{
    if (arg.is_register)
    {
        assert(arg.register_index < (sizeof(ant->registers) / sizeof(ant->registers[0])));
        return ant->registers[arg.register_index];
    }
    else 
    {
        return arg.value;
    }
}

SimulationSettings simulation_settings_create_default(size_t random_seed)
{
    return (SimulationSettings) {
        .random_seed = random_seed,
        .nb_ants = 200,
    };
}

SimulationSettings simulation_settings_create_test()
{
    return (SimulationSettings) {
        .random_seed = 42,
        .nb_ants = 1,
    };
}

Simulation* 
simulation_create(SimulationSettings settings, Program prog, GridMap map)
{
    Simulation* sim = calloc(1, sizeof(Simulation));
    sim->settings = settings;
    sim->step_number = 0;
    sim->ants = calloc(settings.nb_ants, sizeof(Ant));

    for (size_t i = 0; i < settings.nb_ants; i++)
    {
        sim->ants[i].id = (int32_t) i;
        sim->ants[i].position = map.starting_pos; 
    }
    sim->program = prog;
    sim->map = map;
    simulation_get_cell(sim, map.starting_pos)->nb_ants = settings.nb_ants;
    sim->score = 0;
    random_generator_init(&sim->random_generator, settings.random_seed);
    return sim;
}

void simulation_delete(Simulation *sim)
{
    assert(sim != NULL);
    program_cleanup(&sim->program);
    random_generator_cleanup(&sim->random_generator);
    grid_map_cleanup(&sim->map);
    free(sim->ants);
    free(sim);
}

cJSON *simulation_to_json(Simulation *sim)
{
    cJSON* sim_json = cJSON_CreateObject(); 
    cJSON_AddItemToObject(sim_json, "map", grid_map_to_json(&sim->map));
    cJSON_AddNumberToObject(sim_json, "step_number", sim->step_number);
    return sim_json;
}

void simulation_ant_run_single_instruction(Simulation *sim, Ant *ant, Instruction inst)
{
    ant->pc += 1;
    switch(inst.type) 
    {
        case INST_MOVE: 
        {
            int32_t dir = ant_get_arg_value(ant, inst.move_args.dir);
            Position new_pos = ant->position;
            new_pos.x += dir == 2 ? 1 : dir == 4 ? -1 : 0;
            new_pos.y += dir == 1 ? 1 : dir == 3 ? -1 : 0;
            if (simulation_get_cell(sim, new_pos)->type != CELL_TYPE_WALL)
            {
                simulation_set_ant_position(sim, ant, new_pos);
            }
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
            if (cell->type == CELL_TYPE_EMPTY)
            {
                cell->food_amount += 1;
                if (cell->food_amount > 8)
                {
                    cell->food_amount = 8;
                }
            }
            else if (cell->type == CELL_TYPE_NEST)
            {
                sim->score += 1;
            }
            else
            {
                assert(false);
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
        case INST_PROBE:
        {
            int32_t dir = ant_get_arg_value(ant, inst.probe_args.direction);
            if (!direction_is_valid(dir))
            {
                break;
            }
            Cell* cell = simulation_get_neighbor_cell(sim, ant->position, (Direction) dir);
            int32_t result = 0; 
            switch (cell->type)
            {
                case CELL_TYPE_EMPTY: result = cell->food_amount > 0 ? ENT_FOOD : ENT_EMPTY; break;
                case CELL_TYPE_NEST: result = ENT_NEST; break;
                case CELL_TYPE_WALL: result = ENT_WALL; break;
            }
            ant->registers[inst.probe_args.target_reg] = result;
            break;
        }
        case INST_SENSE:
        {
            int32_t entity_type = ant_get_arg_value(ant, inst.sense_args.type);
            if (!entity_type_is_valid(entity_type))
            {
                break;
            }
            int32_t found_directions[4];
            int32_t nb_found = 0;
            for (Direction dir = DIR_NORTH; dir <= DIR_WEST; dir++)
            {
                Cell *cell = simulation_get_neighbor_cell(sim, ant->position, dir);
                if (cell_matches_entity(cell, (EntityType) entity_type))
                {
                    found_directions[nb_found] = dir;
                    nb_found++;
                }
            }
            if (nb_found == 0)
            {
                ant->registers[inst.sense_args.target_reg] = 0;
            }
            else if (nb_found == 1)
            {
                ant->registers[inst.sense_args.target_reg] = found_directions[0];
            }
            else
            {
                size_t random_index = random_generator_generate(&sim->random_generator, nb_found);
                ant->registers[inst.sense_args.target_reg] = found_directions[random_index];
            }
            break;
        }
        case INST_TAG:
        {
            int32_t tag_value = ant_get_arg_value(ant, inst.tag_args.value);
            if (tag_value >= 0 && tag_value < 8)
            {
                ant->tag = tag_value;
            }
            break;
        }
    }
    if (inst.type == INST_MOVE || inst.type == INST_PICKUP || inst.type == INST_DROP)
    {
        ant->instruction_budget = 0;
    }
    else if(inst.type != INST_TAG) // The tag instruction does not count towards the instruction budget
    {
        ant->instruction_budget--;
    }
}

void
simulation_ant_run_step(Simulation *sim, Ant *ant)
{
    while (ant->instruction_budget > 0 && ant->pc >= 0 && (size_t) ant->pc < program_size(&sim->program))
    {
        Instruction inst = program_get_instruction(&sim->program, (size_t) ant->pc);
        simulation_ant_run_single_instruction(sim, ant, inst);
    }
}

void simulation_run_step(Simulation *sim)
{
    sim->step_number++;
    // Decay the pheromones
    for (Cell* it = sim->map.cells; it != sim->map.cells + sim->map.width * sim->map.height; it++)
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

size_t 
simulation_get_step_number(Simulation *sim)
{
    return sim->step_number;
}

size_t simulation_get_score(Simulation *sim)
{
    return sim->score;
}

Cell*
simulation_get_cell(Simulation *sim, Position pos)
{
    return grid_map_get_cell(&sim->map, pos);
}

Cell*
simulation_get_neighbor_cell(Simulation *sim, Position pos, Direction dir)
{
    assert(pos.x > 0 && pos.x < sim->map.width - 1); // No support for fetching neighbors of border cells
    assert(pos.y > 0 && pos.y < sim->map.height - 1);
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

size_t 
simulation_get_nb_ants(Simulation *sim)
{
    return sim->settings.nb_ants;
}

Ant*
simulation_get_ant(Simulation *sim, int32_t id)
{
    return &sim->ants[id];
}

const char*
simulation_get_tag_name(Simulation *sim, uint8_t tag_id)
{
    return program_get_tag_name(&sim->program, tag_id);
}

void 
simulation_set_ant_position(Simulation *sim, Ant *ant, Position new_pos)
{
    Cell *old_cell = simulation_get_cell(sim, ant->position);
    assert(old_cell->nb_ants > 0);
    old_cell->nb_ants--;
    ant->position = new_pos;
    simulation_get_cell(sim, new_pos)->nb_ants++;
}

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

void simulation_init(Simulation *sim, Program prog)
{
    sim->step_number = 0;
    sim->ants = calloc(1, sizeof(Ant));
    sim->ants[0].position.x = 63; 
    sim->ants[0].position.y = 63; 
    sim->program = prog;
    sim->width = 128;
    sim->height = 128;
    sim->cells = calloc(sim->width * sim->height, sizeof(Cell));
    for (size_t y = 0; y < sim->height; y++)
    {
        for (size_t x = 0; x < sim->width; x++)
        {
            sim->cells[sim->width * y + x].position.x = x;
            sim->cells[sim->width * y + x].position.y = x;
        }
    }
}

void simulation_ant_run_step(Simulation *sim, Ant *ant)
{
    if (ant->pc >= 0 && (uint32_t) ant->pc < Program_size(&sim->program))
    {
        Instruction inst = sim->program.instructions.begin[ant->pc];
        switch(inst.type) 
        {
            case INST_MOVE: 
            {
                int32_t dir = ant_get_arg_value(ant, inst.move_args.dir);
                ant->position.x += dir == 2 ? 1 : dir == 4 ? -1 : 0;
                ant->position.y += dir == 1 ? 1 : dir == 3 ? -1 : 0;
                ant->pc += 1;
                break;
            }
            case INST_PICKUP: 
            {
                Cell* cell = simulation_get_cell(sim, ant->position);
                if (ant->carrying_food || cell->type != CELL_TYPE_FOOD)
                {
                    //Nothing to do
                    break;
                }
                cell->type = CELL_TYPE_EMPTY; 
                ant->carrying_food = true;
                ant->pc += 1;
                break;
            }
            case INST_DROP:
            {
                if (!ant->carrying_food)
                {
                    break;
                }
                Cell* cell = simulation_get_cell(sim, ant->position);
                cell->type = CELL_TYPE_FOOD;
                ant->carrying_food = false;
                ant->pc += 1;
                break;
            }
        }
    }
}


void simulation_run_single_step(Simulation *sim)
{
    sim->step_number++;
    simulation_ant_run_step(sim, sim->ants);
}

Cell *simulation_get_cell(Simulation *sim, Position pos)
{
    return &sim->cells[sim->width * pos.y + pos.x];
}

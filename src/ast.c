#include "ast.h"

void 
program2_init(Program2 *program)
{
    program->begin = NULL;
    program->end = NULL;
    program->capacity = 0;
}

void program2_move(Program2 *target, Program2 *source)
{
    *target = *source; 
    source->begin = NULL; 
    source->end = NULL; 
    source->capacity = 0;
}

void program2_clear(Program2 *program)
{
    if (program->begin != NULL) 
    {
        free(program->begin);
    }
}

uint64_t
program2_size(Program2 * program)
{
    return program->end - program->begin;
}

void
program2_push_instruction(Program2 *program, Instruction instruction)
{
    const uint64_t size = program2_size(program);
    if (size >= program->capacity) 
    {
        //Reallocation needed.
        //Allocate new memory
        const uint64_t new_capacity = (program->capacity + 1) * 2;
        Instruction* new_content = calloc(new_capacity, sizeof(Instruction));
        //Copy data to the new memory
        memcpy(new_content, program->begin, program->capacity * sizeof(Instruction));
        //Free old memory
        if (program->begin != NULL) {
            free(program->begin);
        }
        //Update struct
        program->capacity = new_capacity; 
        program->begin = new_content; 
        program->end = program->begin + size;
    }
    *program->end = instruction;
    program->end++;
}

/*
Instruction create_move(Argument arg)
{
    Instruction result; 
    result.type = INST_MOVE;
    result.move_args.dir = arg;
    return result;
}

Instruction create_pickup()
{
    Instruction result; 
    result.type = INST_PICKUP;
    result.pickup_args = {};
    return result;
}

Instruction create_drop()
{
    Instruction result; 
    result.type = INST_DROP;
    result.pickup_args = {};
    return result;
}
*/

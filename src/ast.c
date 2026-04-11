#include "ast.h"

void 
program2_init(Program2 *program)
{
    vector_instruction_init(&program->instructions);
}

void program2_clear(Program2 *program)
{
    vector_instruction_clear(&program->instructions);
}

uint64_t
program2_size(Program2 * program)
{
    return vector_instruction_size(&program->instructions);
}

void
program2_push_instruction(Program2 *program, Instruction instruction)
{
    vector_instruction_push(&program->instructions, instruction);
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

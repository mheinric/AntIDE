#include "ast.h"

Argument argument_create_register(uint8_t reg_index)
{
    Argument result; 
    result.is_register = true; 
    result.register_index = reg_index;
    return result;
}

Argument argument_create_value(int32_t value)
{
    Argument result; 
    result.is_register = false; 
    result.value = value;
    return result;
}

bool argument_equal(Argument first, Argument second)
{
    if (first.is_register != second.is_register)
    {
        return false;
    }
    if (first.is_register)
    {
        return first.register_index == second.register_index;
    }
    else
    {
        return first.value == second.value;
    }
}

Instruction instruction_create_move(Argument dir)
{
    Instruction result; 
    result.type = INST_MOVE; 
    result.move_args.dir = dir;
    return result;
}

bool instruction_equal(Instruction first, Instruction second)
{
    if (first.type != second.type)
    {
        return false;
    }
    switch(first.type)
    {
        case INST_MOVE: return argument_equal(first.move_args.dir, second.move_args.dir);
        default: return true;
    }
}

void program2_init(Program2 *program)
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

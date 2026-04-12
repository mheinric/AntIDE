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

Instruction instruction_create_arithmetic(InstructionType type, uint8_t target_reg, Argument arg)
{
    Instruction result; 
    result.type = type; 
    result.arith_args.target_register = target_reg; 
    result.arith_args.arg = arg;
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
        case INST_PICKUP:
        case INST_DROP: 
            return true; 
        case INST_MOVE: 
            return argument_equal(first.move_args.dir, second.move_args.dir);
        case INST_SET: 
        case INST_ADD:
        case INST_SUB:
        case INST_MOD:
        case INST_MUL:
        case INST_DIV:
        case INST_AND:
        case INST_OR:
        case INST_XOR:
        case INST_LSHIFT:
        case INST_RSHIFT:
        case INST_RANDOM:
            return first.arith_args.target_register == second.arith_args.target_register && 
                argument_equal(first.arith_args.arg, second.arith_args.arg);
    }
    return false;
}

void program_init(Program *program)
{
    vector_instruction_init(&program->instructions);
}

void program_clear(Program *program)
{
    vector_instruction_clear(&program->instructions);
}

uint64_t
program_size(Program * program)
{
    return vector_instruction_size(&program->instructions);
}

void
program_push_instruction(Program *program, Instruction instruction)
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

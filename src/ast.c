#include "ast.h"

bool direction_is_valid(int32_t dir_value)
{
    return dir_value >= 0 && dir_value < 5;
}

bool channel_is_valid(int32_t channel_value)
{
    return channel_value >= 0 && channel_value < 4;
}

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
    return (Instruction) {
        .type = INST_MOVE, 
        .move_args = { .dir = dir }
    };
}

Instruction instruction_create_arithmetic(InstructionType type, uint8_t target_reg, Argument arg)
{
    return (Instruction) {
        .type = type, 
        .arith_args = { .target_register = target_reg, .arg = arg} 
    };
}

Instruction instruction_create_jump(Argument target)
{
    return (Instruction) {
        .type = INST_JMP, 
        .jmp_arg = { .target = target }
    };
}

Instruction instruction_create_conditional_jump(InstructionType type, Argument cond_value1, Argument cond_value2, Argument target)
{
    return (Instruction) {
        .type = type, 
        .cond_jmp_arg = { 
            .cond_value1 = cond_value1,
            .cond_value2 = cond_value2,
            .target = target 
        }
    };
}

Instruction instruction_create_call(uint8_t return_register, Argument target)
{
    return (Instruction) {
        .type = INST_CALL, 
        .call_arg = {
            .return_register = return_register, 
            .target = target
        }
    };
}

Instruction instruction_create_info(InstructionType type, uint8_t target_reg)
{
    return (Instruction) {
        .type = type, 
        .info_arg = { .target_register = target_reg }
    } ;
}

Instruction instruction_create_mark(Argument channel, Argument amount)
{
    return (Instruction) {
        .type = INST_MARK, 
        .mark_args = { .channel = channel, .amount = amount }
    };
}

Instruction instruction_create_sniff(Argument channel, Argument direction, uint8_t target_reg)
{
    return (Instruction){
        .type = INST_SNIFF,
        .sniff_args = { .channel = channel, .direction = direction, .target_reg = target_reg }
    };
}

Instruction instruction_create_smell(Argument channel, uint8_t target_reg)
{
    return (Instruction) {
        .type = INST_SMELL, 
        .smell_args = { .channel = channel, .target_reg = target_reg }
    };
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
        case INST_JMP:
            return argument_equal(first.jmp_arg.target, second.jmp_arg.target);
        case INST_JEQ:
        case INST_JNE:
        case INST_JGT:
        case INST_JLT:
            return argument_equal(first.cond_jmp_arg.cond_value1, second.cond_jmp_arg.cond_value1) && 
                argument_equal(first.cond_jmp_arg.cond_value2, second.cond_jmp_arg.cond_value2) &&
                argument_equal(first.cond_jmp_arg.target, second.cond_jmp_arg.target);
        case INST_CALL: 
            return first.call_arg.return_register == second.call_arg.return_register && 
                argument_equal(first.call_arg.target, second.call_arg.target);
        case INST_ID: 
        case INST_CARRY: 
            return first.info_arg.target_register == second.info_arg.target_register;
        case INST_MARK: 
            return argument_equal(first.mark_args.amount, second.mark_args.amount) &&
                argument_equal(first.mark_args.channel, second.mark_args.channel);
        case INST_SNIFF: 
            return first.sniff_args.target_reg == second.sniff_args.target_reg &&
                argument_equal(first.sniff_args.channel, second.sniff_args.channel) &&
                argument_equal(first.sniff_args.direction, second.sniff_args.direction);
        case INST_SMELL: 
            return first.smell_args.target_reg == second.smell_args.target_reg &&
                argument_equal(first.smell_args.channel, second.smell_args.channel);
    }
    return false;
}

void program_init(Program *program)
{
    vector_instruction_init(&program->instructions);
}

void program_init_move(Program *target, Program *source)
{
    vector_instruction_init_move(&target->instructions, &source->instructions);
}

void program_cleanup(Program *program)
{
    vector_instruction_cleanup(&program->instructions);
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
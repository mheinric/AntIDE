#include "ast_private.h"
#include "ast.h"

void tag_init(Tag *tag, uint8_t id, const char *name)
{
    tag->id = id; 
    tag->name = strdup(name);
}

void
tag_cleanup(Tag* tag)
{
    free(tag->name);
}

bool 
direction_is_valid(int32_t dir_value)
{
    return dir_value >= 0 && dir_value < 5;
}

bool 
channel_is_valid(int32_t channel_value)
{
    return channel_value >= 0 && channel_value < 4;
}

bool entity_type_is_valid(int32_t entity_type)
{
    return entity_type >= 0 && entity_type < 5;
}

Argument 
argument_create_register(uint8_t reg_index)
{
    Argument result; 
    result.is_register = true; 
    result.register_index = reg_index;
    return result;
}

Argument 
argument_create_value(int32_t value)
{
    Argument result; 
    result.is_register = false; 
    result.value = value;
    return result;
}

bool 
argument_equal(Argument first, Argument second)
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

Instruction 
instruction_create_move(Argument dir)
{
    return (Instruction) {
        .type = INST_MOVE, 
        .breakpoint = false,
        .move_args = { .dir = dir }
    };
}

Instruction 
instruction_create_arithmetic(InstructionType type, uint8_t target_reg, Argument arg)
{
    return (Instruction) {
        .type = type, 
        .breakpoint = false,
        .arith_args = { .target_register = target_reg, .arg = arg} 
    };
}

Instruction 
instruction_create_jump(Argument target)
{
    return (Instruction) {
        .type = INST_JMP, 
        .breakpoint = false,
        .jmp_arg = { .target = target }
    };
}

Instruction 
instruction_create_conditional_jump(InstructionType type, Argument cond_value1, Argument cond_value2, Argument target)
{
    return (Instruction) {
        .type = type, 
        .breakpoint = false,
        .cond_jmp_arg = { 
            .cond_value1 = cond_value1,
            .cond_value2 = cond_value2,
            .target = target 
        }
    };
}

Instruction 
instruction_create_call(uint8_t return_register, Argument target)
{
    return (Instruction) {
        .type = INST_CALL, 
        .breakpoint = false,
        .call_arg = {
            .return_register = return_register, 
            .target = target
        }
    };
}

Instruction 
instruction_create_info(InstructionType type, uint8_t target_reg)
{
    return (Instruction) {
        .type = type, 
        .breakpoint = false,
        .info_arg = { .target_register = target_reg }
    } ;
}

Instruction 
instruction_create_mark(Argument channel, Argument amount)
{
    return (Instruction) {
        .type = INST_MARK, 
        .breakpoint = false,
        .mark_args = { .channel = channel, .amount = amount }
    };
}

Instruction 
instruction_create_sniff(Argument channel, Argument direction, uint8_t target_reg)
{
    return (Instruction){
        .type = INST_SNIFF,
        .breakpoint = false,
        .sniff_args = { .channel = channel, .direction = direction, .target_reg = target_reg }
    };
}

Instruction 
instruction_create_smell(Argument channel, uint8_t target_reg)
{
    return (Instruction) {
        .type = INST_SMELL, 
        .breakpoint = false,
        .smell_args = { .channel = channel, .target_reg = target_reg }
    };
}


Instruction 
instruction_create_probe(Argument direction, uint8_t target_reg)
{
    return (Instruction) {
        .type = INST_PROBE, 
        .breakpoint = false,
        .probe_args = { .direction = direction, .target_reg = target_reg },
    };
}

Instruction 
instruction_create_sense(Argument sense_type, uint8_t target_reg)
{
    return (Instruction) {
        .type = INST_SENSE,
        .breakpoint = false,
        .sense_args = { .type = sense_type, .target_reg = target_reg }
    };
}

Instruction instruction_create_tag(Argument tag_value)
{
    return (Instruction) {
        .type = INST_TAG,
        .breakpoint = false,
        .tag_args = { .value = tag_value }
    };
}

bool 
instruction_equal(Instruction first, Instruction second)
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
        case INST_PROBE: 
            return first.probe_args.target_reg == second.probe_args.target_reg &&
                argument_equal(first.probe_args.direction, second.probe_args.direction);
        case INST_SENSE: 
            return first.sense_args.target_reg == second.sense_args.target_reg &&
                argument_equal(first.sense_args.type, second.sense_args.type);
        case INST_TAG: 
            return argument_equal(first.tag_args.value, second.tag_args.value);
    }
    return false;
}

void 
program_init(Program *program)
{
    program->instructions = calloc(1, sizeof(VectorInstruction));
    vector_instruction_init(program->instructions);
    program->tags = calloc(1, sizeof(VectorTag)); 
    vector_tag_init(program->tags);
    program->source_map = calloc(1, sizeof(VectorSourceMap));
    vector_source_map_init(program->source_map);
}

void 
program_init_move(Program *target, Program *source)
{
    target->instructions = calloc(1, sizeof(VectorInstruction));
    vector_instruction_init_move(target->instructions, source->instructions);
    target->tags = calloc(1, sizeof(VectorTag));
    vector_tag_init_move(target->tags, source->tags);
    target->source_map = calloc(1, sizeof(VectorSourceMap));
    vector_source_map_init_move(target->source_map, source->source_map);
}

void 
program_cleanup(Program *program)
{
    vector_instruction_cleanup(program->instructions);
    free(program->instructions);
    program->instructions = NULL;
    for (Tag* it = program->tags->begin; it != program->tags->end; it++)
    {
        tag_cleanup(it);
    }
    vector_tag_cleanup(program->tags);
    free(program->tags);
    program->tags = NULL;
    vector_source_map_cleanup(program->source_map);
    free(program->source_map);
    program->source_map = NULL;
}

uint64_t
program_size(const Program * program)
{
    return vector_instruction_size(program->instructions);
}

Instruction 
program_get_instruction(const Program *program, size_t index)
{
    return program->instructions->begin[index];
}

void 
program_push_instruction(Program *program, Instruction instruction, size_t line_nb)
{
    size_t inst_index = vector_instruction_size(program->instructions);
    vector_instruction_push(program->instructions, instruction);
    vector_source_map_push(program->source_map, (SourceMap) { .inst_index = inst_index, .line_nb = line_nb });
}

void program_add_tag(Program *program, uint8_t tag_id, const char *tag_name)
{
    Tag tag; 
    tag_init(&tag, tag_id, tag_name); 
    vector_tag_push(program->tags, tag);
}

const char*
program_get_tag_name(const Program *program, uint8_t tag_id)
{
    for (Tag* it = program->tags->begin; it != program->tags->end; it++)
    {
        if (it->id == tag_id)
        {
            return it->name;
        }
    }
    return "";
}

size_t 
program_get_source_line(const Program *program, size_t instruction_index)
{
    for (SourceMap* it = program->source_map->begin; it != program->source_map->end; it++)
    {
        if (it->inst_index == instruction_index)
        {
            return it->line_nb;
        }
    }
    return 0;
}

size_t 
program_get_instruction_index(const Program *program, size_t line_nb)
{
    size_t last_index = 0;
    for (SourceMap* it = program->source_map->begin; it != program->source_map->end; it++)
    {
        if (it->line_nb == line_nb)
        {
            return it->inst_index;
        }
        last_index = it->inst_index; 
        if (it->line_nb > line_nb)
        {
            break;
        }
    }
    return last_index;
}

void 
program_set_breakpoint(Program* program, size_t inst_index)
{
    assert(inst_index < program_size(program));
    program->instructions->begin[inst_index].breakpoint = true;
}

void 
program_clear_breakpoints(Program* program)
{
    for (Instruction* it = program->instructions->begin; it != program->instructions->end; it++)
    {
        it->breakpoint = false;
    }
}
#pragma once
#include "utils.h"

typedef enum {
    DIR_HERE   = 0, 
    DIR_NORTH  = 1,
    DIR_EAST   = 2,
    DIR_SOUTH  = 3,
    DIR_WEST   = 4, 
    DIR_RANDOM = 5,
} Direction;

bool
direction_is_valid(int32_t dir_value);

typedef enum {
    CH_RED    = 0, 
    CH_BLUE   = 1, 
    CH_GREEN  = 2, 
    CH_YELLOW = 3
} Channel;

bool
channel_is_valid(int32_t channel_value);

typedef enum {
    ENT_EMPTY = 0,
    ENT_WALL  = 1,
    ENT_FOOD  = 2,
    ENT_NEST  = 3,
    ENT_ANT   = 4,
} EntityType;

bool 
entity_type_is_valid(int32_t entity_type);

typedef enum {
    //Action instructions
    INST_PICKUP,
    INST_DROP,
    INST_MOVE,
    INST_MARK,

    //Jumps
    INST_JMP,
    INST_JEQ,
    INST_JNE,
    INST_JGT,
    INST_JLT,
    INST_CALL,

    //Arithmetic
    INST_SET,
    INST_ADD,
    INST_SUB,
    INST_MOD,
    INST_MUL,
    INST_DIV,
    INST_AND,
    INST_OR,
    INST_XOR,
    INST_LSHIFT,
    INST_RSHIFT,
    INST_RANDOM,

    //Probing the environment
    INST_ID,
    INST_CARRY,
    INST_SNIFF,
    INST_SMELL,
    INST_PROBE,
    INST_SENSE,

    //Debug
    INST_TAG,
} InstructionType;

typedef struct {
    bool is_register;
    union {
        int32_t value; 
        uint8_t register_index;
    };
} Argument;

Argument
argument_create_register(uint8_t reg_index);

Argument
argument_create_value(int32_t value);

bool 
argument_equal(Argument first, Argument second);

typedef struct {
    InstructionType type; 

    union {
        struct {} no_args;
        struct { Argument dir; } move_args;
        struct { uint8_t target_register; Argument arg; } arith_args;
        struct { Argument target; } jmp_arg;
        struct { Argument cond_value1; Argument cond_value2; Argument target; } cond_jmp_arg;
        struct { uint8_t return_register; Argument target; } call_arg;
        struct { uint8_t target_register; } info_arg;
        struct { Argument channel; Argument amount; } mark_args;
        struct { Argument channel; Argument direction; uint8_t target_reg; } sniff_args;
        struct { Argument channel; uint8_t target_reg; } smell_args;
        struct { Argument direction; uint8_t target_reg; } probe_args;
        struct { Argument type; uint8_t target_reg; } sense_args;
        struct { Argument value; } tag_args;
    };
} Instruction;

Instruction 
instruction_create_move(Argument dir);

Instruction 
instruction_create_arithmetic(InstructionType type, uint8_t target_reg, Argument arg);

Instruction 
instruction_create_jump(Argument arg);

Instruction 
instruction_create_conditional_jump(InstructionType type, Argument cond_value1, Argument cond_value2, Argument target);

Instruction
instruction_create_call(uint8_t return_register, Argument target);

Instruction 
instruction_create_info(InstructionType type, uint8_t target_reg);

Instruction
instruction_create_mark(Argument channel, Argument amount);

Instruction 
instruction_create_sniff(Argument channel, Argument direction, uint8_t target_reg);

Instruction
instruction_create_smell(Argument channel, uint8_t target_reg);

Instruction
instruction_create_probe(Argument direction, uint8_t target_reg);

Instruction 
instruction_create_sense(Argument sense_type, uint8_t target_reg);

Instruction 
instruction_create_tag(Argument tag_value);

bool 
instruction_equal(Instruction first, Instruction second);

#define VECTOR_ITEM_TYPE Instruction 
#define VECTOR_ITEM_NAME instruction
#include "internals/vector.h"

typedef struct {
    VectorInstruction instructions;
} Program;

void 
program_init(Program *program);

void
program_init_move(Program *target, Program *source);

void
program_cleanup(Program *program);

uint64_t
program_size(Program *program);

void
program_push_instruction(Program *program, Instruction instruction);
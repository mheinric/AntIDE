#pragma once
#include "utils.h"

typedef enum {
    //Action instructions
    INST_PICKUP,
    INST_DROP,
    INST_MOVE,

    //Jumps

    //Arithmetic
    INST_SET,
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
    };
} Instruction;

Instruction 
instruction_create_move(Argument dir);

Instruction 
instruction_create_arithmetic(InstructionType type, uint8_t target_reg, Argument arg);

bool 
instruction_equal(Instruction first, Instruction second);

#define VECTOR_ITEM_TYPE Instruction 
#define VECTOR_ITEM_NAME instruction
#include "vector.h"
#undef VECTOR_ITEM_TYPE
#undef VECTOR_ITEM_NAME

typedef struct {
    VectorInstruction instructions;
} Program;

void 
program_init(Program *program);

void
program_clear(Program *program);

uint64_t
program_size(Program *program);

void
program_push_instruction(Program *program, Instruction instruction);
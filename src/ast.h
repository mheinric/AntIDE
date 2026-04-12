#pragma once
#include "utils.h"

typedef enum {
    //Action instructions
    INST_PICKUP,
    INST_DROP,
    INST_MOVE,

    //Jumps

    //Arithmetic
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
    };
} Instruction;

Instruction 
instruction_create_move(Argument dir);

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
Program_init(Program *program);

void 
Program_move(Program *target, Program *source);

void
Program_clear(Program *program);

uint64_t
Program_size(Program *program);

void
Program_push_instruction(Program *program, Instruction instruction);
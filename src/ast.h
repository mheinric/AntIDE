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
} Program2;

void 
program2_init(Program2 *program);

void 
program2_move(Program2 *target, Program2 *source);

void
program2_clear(Program2 *program);

uint64_t
program2_size(Program2 *program);

void
program2_push_instruction(Program2 *program, Instruction instruction);

/*


typedef struct {
    uint8_t index;
} Register;

typedef struct {
    bool is_register; 
    union {
        int32_t int_value;
        Register reg;
    };
} Argument;

typedef struct {
    InstructionType type; 
    union {
        struct { Argument dir; } move_args;
        struct {} pickup_args;
        struct {} drop_args;
    };
} Instruction;

Instruction create_move(Argument arg);
Instruction create_pickup();
Instruction create_drop();
*/
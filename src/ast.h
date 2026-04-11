#pragma once
#include "utils.h"

typedef enum {
    //Action instructions
    INST_PICKUP,
    INST_DROP,

    //Jumps

    //Arithmetic
} InstructionType;


typedef struct {
    InstructionType type; 
} Instruction;

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
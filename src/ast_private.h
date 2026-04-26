#pragma once
#include "ast.h"

#define VECTOR_ITEM_TYPE Instruction 
#define VECTOR_ITEM_NAME instruction
#include "internals/vector.h"

typedef struct {
    uint8_t id; 
    char* name;
} Tag; 

void
tag_init(Tag* tag, uint8_t id, const char* name);

void
tag_cleanup(Tag* tag);

#define VECTOR_ITEM_TYPE Tag
#define VECTOR_ITEM_NAME tag
#include "internals/vector.h"
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

typedef struct {
    size_t line_nb; 
    size_t inst_index;
} SourceMap;

#define VECTOR_ITEM_TYPE SourceMap
#define VECTOR_ITEM_NAME source_map
#include "internals/vector.h"

typedef struct {
    uint8_t register_index; 
    char* alias;
} RegisterAlias;

#define VECTOR_ITEM_TYPE RegisterAlias
#define VECTOR_ITEM_NAME register_alias
#include "internals/vector.h"

void 
register_alias_init(RegisterAlias* reg_alias, uint8_t index, char* alias);

void 
register_alias_cleanup(RegisterAlias* reg_alias);
//No include guard here because we want to be able to include this file several times 
//with different values of VECTOR_ITEM_TYPE.
//Function should be marked as static inline in this file to avoid multiple definition issues.

#include "utils.h"

#ifndef VECTOR_ITEM_TYPE
#warning "vector.h: VECTOR_ITEM_TYPE was not defined before including this file, assuming int"
#define VECTOR_ITEM_TYPE int
#define VECTOR_ITEM_NAME int
#endif

#ifndef VECTOR_ITEM_NAME
#error "vector.h You must define VECTOR_ITEM_NAME before including this file"
#endif

// Need to define a concatenation macro to make sur all macros are expanded correctly.
#define _VECTOR_CAT(a, b) a ## b
#define VECTOR_CAT(a, b) _VECTOR_CAT(a, b)

#define VECTOR_TYPE VECTOR_CAT(Vector, VECTOR_ITEM_TYPE)
#define VECTOR_FUN(FUN_NAME) VECTOR_CAT(VECTOR_CAT(vector_, VECTOR_ITEM_NAME), VECTOR_CAT(_, FUN_NAME))

typedef struct {
    VECTOR_ITEM_TYPE *begin; 
    VECTOR_ITEM_TYPE *end;
    size_t capacity;
} VECTOR_TYPE;

static inline 
void 
VECTOR_FUN(init) (VECTOR_TYPE *vect) {
    vect->begin = NULL; 
    vect->end = NULL; 
    vect->capacity = 0;
}

static inline 
void 
VECTOR_FUN(init_move) (VECTOR_TYPE *target, VECTOR_TYPE *source) {
    target->begin = source->begin; 
    target->end = source->end; 
    target->capacity = source->capacity;
    memset(source, 0, sizeof(VECTOR_TYPE));
}

static inline 
void 
VECTOR_FUN(cleanup) (VECTOR_TYPE *vect) {
    if (vect->begin != NULL) 
    {
        free(vect->begin);
    }
    vect->begin = NULL; 
    vect->end = NULL; 
    vect->capacity = 0;
}

static inline 
size_t 
VECTOR_FUN(size) (const VECTOR_TYPE *vect) {
    return vect->end - vect->begin;
}

static inline 
void
VECTOR_FUN(push) (VECTOR_TYPE *vect, VECTOR_ITEM_TYPE item) {
    const size_t size = VECTOR_FUN(size)(vect);
    if (size >= vect->capacity) 
    {
        //Reallocation needed.
        //Allocate new memory
        const uint64_t new_capacity = (vect->capacity + 1) * 2;
        VECTOR_ITEM_TYPE* new_content = calloc(new_capacity, sizeof(VECTOR_ITEM_TYPE));
        //Copy data to the new memory
        memcpy(new_content, vect->begin, vect->capacity * sizeof(VECTOR_ITEM_TYPE));
        //Free old memory
        if (vect->begin != NULL) {
            free(vect->begin);
        }
        //Update struct
        vect->capacity = new_capacity; 
        vect->begin = new_content; 
        vect->end = vect->begin + size;
    }
    *vect->end = item;
    vect->end++;
}


#pragma once
#include "utils.h"

typedef struct {
    uint64_t state;
} RandomGenerator;

void
random_generator_init(RandomGenerator *rand, uint64_t seed);

void
random_generator_cleanup(RandomGenerator *rand);

int32_t 
random_generator_generate(RandomGenerator *rand, int32_t max_bound);
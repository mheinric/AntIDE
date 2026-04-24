#include "rng.h"

void random_generator_init(RandomGenerator *rand, uint64_t seed)
{
    rand->state = seed;
}

void random_generator_cleanup(RandomGenerator */*rand*/)
{
}

int32_t random_generator_generate(RandomGenerator *rand, int32_t max_bound)
{
    assert(max_bound != 0);
    // Values taken from https://en.wikipedia.org/wiki/Linear_congruential_generator from the MUSL implementation
    rand->state = rand->state * 6364136223846793005 + 1;
    // we return the upper bits
    int32_t result = (rand->state >> 32) % abs(max_bound);
    return max_bound < 0 ? -result : result; //Note when max_bound is negative we get negative values.
}
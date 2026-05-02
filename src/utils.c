#include "utils.h"

double 
timespec_diff(struct timespec first, struct timespec second)
{
    return (first.tv_sec - second.tv_sec) + (first.tv_nsec - second.tv_nsec) / 1e9;
}
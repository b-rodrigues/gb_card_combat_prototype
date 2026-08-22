#include "rng.h"

uint16_t g_rng_state = 1;

uint16_t rng_next(void)
{
    g_rng_state ^= g_rng_state << 7;
    g_rng_state ^= g_rng_state >> 9;
    g_rng_state ^= g_rng_state << 8;
    return g_rng_state;
}

void rng_set_seed(uint16_t seed)
{
    g_rng_state = seed ? seed : 1;
}

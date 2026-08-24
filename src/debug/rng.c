#include "rng.h"
#include "rpg/loot.h"

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
    /* Derive the isolated loot stream from the same seed so drops are
     * deterministic per scenario while never consuming the shared
     * stream (docs/loot.md §34.5). */
    g_loot_rng_state = (uint16_t)(g_rng_state ^ 0x1B3C);
}

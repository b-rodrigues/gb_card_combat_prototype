#pragma bank 2

#include "game.h"
#include "banked.h"

/* ── Banked render-cache reset (src/core/game.c dispatcher) ─────────
 * Pure WRAM writes over the staged Game pointer; no calls, no fixed-bank
 * references (AGENTS.md 52.11.1). */

void game_render_reset_banked(void)
{
    Game *g = (Game *)g_bk_ptr_a;
    uint8_t *p;
    uint16_t n;
    if (!g) return;
    p = (uint8_t *)&g->render_cache;
    n = sizeof(RenderCache);
    while (n--) *p++ = 0xFF;
    g->render_cache.valid = false;
    g->render_cache.prev_dialogue_active = false;
    g->render_cache.prev_screen = g->prev_screen;
    g->render_cache.prev_map_id = g->world.map_id;
    g->battle.dirty = BATTLE_DIRTY_ALL;
}

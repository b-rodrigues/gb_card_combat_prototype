#pragma bank 2

#include "scene.h"
#include "banked.h"

/* Banked body of scene_load_tiles() (see scene.c).  Lives in ROM bank 2 and
 * runs through the WRAM banked-call trampoline so the ~600-byte terrain
 * builder does not consume the fixed-bank _CODE budget.  Self-contained: the
 * scene tables (g_scenes / exits / terrain blocks) all live in this same
 * bank, so it reads them directly (no banked_copy, no fixed-bank calls).  It
 * reads the World pointer from g_bk_ptr_a and the map id from g_bk_byte_a. */

extern const SceneDefinition g_scenes[];

void scene_load_tiles_banked(void)
{
    World *w = (World *)g_bk_ptr_a;
    MapId map_id = (MapId)g_bk_byte_a;
    const SceneDefinition *def;
    uint8_t i, x, y;

    if (!w) return;
    if (map_id > MAP_CASTLE) return;
    def = &g_scenes[map_id];

    for (y = 0; y < w->height; y++) {
        uint8_t *row = w->map[y];
        for (x = 0; x < w->width; x++) {
            row[x] = (x == 0 || x == (uint8_t)(w->width - 1) || y == 0 || y == (uint8_t)(w->height - 1)) ? TILE_WALL : TILE_FLOOR;
        }
    }

    if (def->terrain_blocks) {
        for (i = 0; ; i++) {
            const SceneTerrainBlock *b = &def->terrain_blocks[i];
            uint8_t ex, ey;
            if (b->w == 0) break;
            ey = (uint8_t)(b->y + b->h);
            ex = (uint8_t)(b->x + b->w);
            if (ey > w->height) ey = w->height;
            if (ex > w->width) ex = w->width;
            for (y = b->y; y < ey; y++) {
                uint8_t *row = w->map[y];
                for (x = b->x; x < ex; x++) {
                    row[x] = b->tile;
                }
            }
        }
    }

    for (x = 0; x < def->exit_count; x++) {
        const SceneExit *e = &def->exits[x];
        w->map[e->gate_y][e->gate_x] = TILE_EXIT;
    }
}

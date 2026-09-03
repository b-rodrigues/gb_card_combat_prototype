#pragma bank 5

#include <stdint.h>

/* World background tiles extracted from assets/ by tools/png2gb.py (make gfx) */

const uint8_t g_tileset_forest[768] = {
    /* 48 forest tiles (768 bytes): All wall, floor, stump, tree, and exit tiles */
#include "gfx/rpg_forest_world_tiles.inc"
};

const uint8_t g_tileset_desolate[768] = {
    /* 48 desolate landscape tiles (768 bytes): All wall, floor, rock, tree, and prop tiles */
#include "gfx/rpg_desolate_world_tiles.inc"
};

const uint8_t g_tileset_castle[432] = {
    /* 27 castle tiles (432 bytes) */
#include "gfx/rpg_castle_tiles.inc"
};

const uint8_t g_intrepid_font_tiles[1536] = {
#include "gfx/intrepid_font_tiles.inc"
};
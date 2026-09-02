#pragma bank 5

#include <stdint.h>

/* World background tiles extracted from assets/ by tools/png2gb.py (make gfx) */
const uint8_t g_rpg_world_tiles[1024] = {
    /* 4 exterior tiles (64 bytes): Floor, Wall, Gate, Building */
#include "gfx/rpg_exterior_tiles.inc"

    /* 4 interior tiles (64 bytes): Floor, Wall, Door, Prop */
#include "gfx/rpg_interior_tiles.inc"

    /* 8 forest tiles (128 bytes): Floor, Tree, Gate, Stump TL, Stump TR, Stump BL, Stump BR, Mini Stump */
#include "gfx/rpg_forest_tiles.inc"

    /* 48 desolate landscape tiles (768 bytes): All wall, floor, rock, tree, and prop tiles */
#include "gfx/rpg_desolate_world_tiles.inc"
};

/* Font tiles extracted from assets/intrepid.png (96 tiles = 1536 bytes) */
const uint8_t g_intrepid_font_tiles[1536] = {
#include "gfx/intrepid_font_tiles.inc"
};

#pragma bank 5

#include <stdint.h>
#include <gb/gb.h>
#include <gb/cgb.h>
#include "tile_palette.h"
#include "world/world.h"
#include "ui/ui.h"
#include "banked.h"
#include "gfx/rpg_tile_lookup.h"

/* 8 CGB BG palettes, 4 colors each in RGB555 format. */
const palette_color_t cgb_bg_palettes[8][4] = {
    /* 0 gray */     { RGB8(255,255,255), RGB8(170,170,170), RGB8(85,85,85),  RGB8(0,0,0)      },
    /* 1 fire */     { RGB8(255,255,224), RGB8(255,140,40),  RGB8(220,50,20), RGB8(100,10,0)   },
    /* 2 iron/ice */ { RGB8(235,242,250), RGB8(140,180,214), RGB8(70,105,138), RGB8(27,43,58) },
    /* 3 field */    { RGB8(120,176,96),  RGB8(40,72,24),    RGB8(24,56,8),   RGB8(0,0,0)     },
    /* 4 poison */   { RGB8(240,255,240), RGB8(100,220,100), RGB8(30,140,50), RGB8(10,50,20) },
    /* 5 wood */     { RGB8(245,230,210), RGB8(196,138,72),  RGB8(138,82,34), RGB8(61,32,10)  },
    /* 6 gold */     { RGB8(255,252,224), RGB8(255,215,0),   RGB8(200,140,8), RGB8(90,58,0)   },
    /* 7 dim */      { RGB8(200,200,200), RGB8(150,150,150), RGB8(90,90,90),   RGB8(40,40,40)   }
};

/* Forest / Field overworld palette set: Palette 5 (wood) Color 0 is
 * harmonized to grass green (RGB8(120,176,96)), eliminating rectangular
 * beige seams around tree trunks and stumps while preserving wood details. */
const palette_color_t cgb_bg_palettes_forest[8][4] = {
    /* 0 gray */     { RGB8(255,255,255), RGB8(170,170,170), RGB8(85,85,85),  RGB8(0,0,0)      },
    /* 1 fire */     { RGB8(255,255,224), RGB8(255,140,40),  RGB8(220,50,20), RGB8(100,10,0)   },
    /* 2 iron/ice */ { RGB8(235,242,250), RGB8(140,180,214), RGB8(70,105,138), RGB8(27,43,58) },
    /* 3 field */    { RGB8(120,176,96),  RGB8(40,72,24),    RGB8(24,56,8),   RGB8(0,0,0)     },
    /* 4 poison */   { RGB8(240,255,240), RGB8(100,220,100), RGB8(30,140,50), RGB8(10,50,20) },
    /* 5 wood */     { RGB8(120,176,96),  RGB8(196,138,72),  RGB8(138,82,34), RGB8(61,32,10)  },
    /* 6 gold */     { RGB8(255,252,224), RGB8(255,215,0),   RGB8(200,140,8), RGB8(90,58,0)   },
    /* 7 dim */      { RGB8(200,200,200), RGB8(150,150,150), RGB8(90,90,90),   RGB8(40,40,40)   }
};

/* Desolate landscape palette set: Anchor color 0 is slate rock (#938da1
 * -> RGB8(147,141,161)) across outdoor nature palettes. */
const palette_color_t cgb_bg_palettes_desolate[8][4] = {
    /* 0 gray */     { RGB8(255,255,255), RGB8(170,170,170), RGB8(85,85,85),  RGB8(0,0,0)      },
    /* 1 campfire */ { RGB8(147,141,161), RGB8(237,194,20),  RGB8(215,80,20), RGB8(80,10,0)   },
    /* 2 iron/ice */ { RGB8(147,141,161), RGB8(140,180,214), RGB8(70,105,138), RGB8(27,43,58) },
    /* 3 flora */    { RGB8(147,141,161), RGB8(116,111,128), RGB8(63,58,74),  RGB8(38,35,46)  },
    /* 4 poison */   { RGB8(147,141,161), RGB8(100,220,100), RGB8(30,140,50), RGB8(10,50,20) },
    /* 5 deadwood */ { RGB8(147,141,161), RGB8(141,117,74),  RGB8(111,90,52), RGB8(38,35,46)  },
    /* 6 gold */     { RGB8(147,141,161), RGB8(215,167,38),  RGB8(141,117,74),RGB8(50,30,10)  },
    /* 7 slate rock*/{ RGB8(147,141,161), RGB8(131,123,150), RGB8(63,58,74),  RGB8(38,35,46)  }
};

/* Castle palette set: Anchor color 0 is light stone (#d7d7d7 ->
 * RGB8(215,215,215)) across indoor stone and furniture palettes. */
const palette_color_t cgb_bg_palettes_castle[8][4] = {
    /* 0 stone */    { RGB8(215,215,215), RGB8(179,176,176), RGB8(130,130,130),RGB8(46,46,46)  },
    /* 1 curtain */  { RGB8(215,215,215), RGB8(139,27,27),   RGB8(98,18,18),  RGB8(30,0,0)    },
    /* 2 iron */     { RGB8(215,215,215), RGB8(140,160,180), RGB8(70,90,110), RGB8(30,40,50)  },
    /* 3 moss/green*/{ RGB8(215,215,215), RGB8(90,140,80),   RGB8(40,80,30),  RGB8(10,30,10)  },
    /* 4 poison */   { RGB8(215,215,215), RGB8(120,200,120), RGB8(40,120,50), RGB8(10,50,20) },
    /* 5 wood furn */{ RGB8(215,215,215), RGB8(158,142,113), RGB8(111,90,52), RGB8(40,25,10)  },
    /* 6 gold */     { RGB8(215,215,215), RGB8(215,167,38),  RGB8(162,146,113),RGB8(60,40,10) },
    /* 7 dim shadow*/{ RGB8(215,215,215), RGB8(130,130,130), RGB8(86,86,86),  RGB8(35,35,35)  }
};

/* Backward compatibility alias */
#define cgb_bg_palettes_overworld cgb_bg_palettes_forest

void ui_load_cram_banked(void)
{
    uint8_t p, c;
    const uint8_t *pal_data;
    if (!g_is_cgb) return;
    switch (g_bk_byte_a) {
        case WORLD_TILESET_DESOLATE:
            pal_data = (const uint8_t *)cgb_bg_palettes_desolate;
            break;
        case WORLD_TILESET_CASTLE:
            pal_data = (const uint8_t *)cgb_bg_palettes_castle;
            break;
        case WORLD_TILESET_FOREST:
        case 1: /* Legacy overworld mode */
            pal_data = (const uint8_t *)cgb_bg_palettes_forest;
            break;
        case 0: /* Battle / UI mode */
        default:
            pal_data = (const uint8_t *)cgb_bg_palettes;
            break;
    }
    for (p = 0; p < 8; p++) {
        const uint8_t *ramp = pal_data + ((uint16_t)p << 3);
        BCPS_REG = (uint8_t)(0x80 | (p << 3));
        for (c = 0; c < 8; c++) {
            BCPD_REG = ramp[c];
        }
    }
}

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

void ui_load_tileset_banked(void)
{
    uint8_t tileset = g_bk_byte_a;
    const uint8_t *src;
    const uint8_t *pal_src;
    uint8_t tile_count;
    uint8_t i;
    uint16_t n;
    volatile uint8_t *dst;

    switch (tileset) {
        case WORLD_TILESET_FOREST:
            src = g_tileset_forest;
            pal_src = g_tile_pal_forest;
            tile_count = 48;
            break;
        case WORLD_TILESET_DESOLATE:
            src = g_tileset_desolate;
            pal_src = g_tile_pal_desolate;
            tile_count = 48;
            break;
        case WORLD_TILESET_CASTLE:
            src = g_tileset_castle;
            pal_src = g_tile_pal_castle;
            tile_count = 27;
            break;
        default:
            src = g_tileset_forest;
            pal_src = g_tile_pal_forest;
            tile_count = 48;
            break;
    }

    VBK_REG = 0;
    dst = (volatile uint8_t *)(0x8000u + ((uint16_t)RPG_TILE_BASE_WORLD << 4));
    n = (uint16_t)tile_count << 4;
    while (n--) {
        *dst++ = *src++;
    }
    for (i = 0; i < tile_count; i++) {
        g_active_tile_palette[i] = pal_src[i];
    }
    for (; i < 48; i++) {
        g_active_tile_palette[i] = 0;
    }
}
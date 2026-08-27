#pragma bank 3

#include <gb/gb.h>
#include <gb/cgb.h>
#include "ui.h"
#include "banked.h"

/* ── CGB effect-color support, bank-3 body (docs/loot.md §34) ─────────
 * The palette ramps and the VRAM bank-1 attribute writer live here so the
 * fixed bank stays under 0x8000 (AGENTS.md §55.5).  ui_init() banked_copies
 * the palette table into WRAM to program BCPS; the fixed-bank ui_color_span()
 * wrapper stages x/y/len/palette through g_bk_ptr_a and dispatches here.
 */

const palette_color_t cgb_bg_palettes[5][4] = {
    /* 0 gray */   { RGB8(255,255,255), RGB8(170,170,170), RGB8(85,85,85),  RGB8(0,0,0)      },
    /* 1 fire */   { RGB8(255,255,255), RGB8(255,192,192), RGB8(255,96,96), RGB8(192,0,0)   },
    /* 2 ice */    { RGB8(255,255,255), RGB8(192,192,255), RGB8(96,96,255), RGB8(0,0,192)   },
    /* 3 heal */   { RGB8(255,255,255), RGB8(192,255,192), RGB8(96,255,96), RGB8(0,160,0)   },
    /* 4 poison */ { RGB8(255,255,255), RGB8(224,192,255), RGB8(160,96,255), RGB8(96,0,192) }
};

/* Self-contained VRAM-sync write (banked code cannot call ui_vram_sync_write).
 * di/ei guards the PPU wait; unconditionally re-enabling IME via ei is safe
 * because interrupts are always active during normal play and stubbed under the harness. */
static void color_vram_sync_write(volatile uint8_t *dst, uint8_t v)
{
    if (LCDC_REG & 0x80) {
        __asm
            di
        __endasm;
        while (STAT_REG & 0x02);
        *dst = v;
        __asm
            ei
        __endasm;
    } else {
        *dst = v;
    }
}

/* Reads the staged x/y/len/palette from g_bk_ptr_a (see ui_color_span()). */
void ui_color_span_banked(void)
{
    uint8_t *a = (uint8_t *)g_bk_ptr_a;
    uint8_t x = a[0];
    uint8_t y = a[1];
    uint8_t len = a[2];
    uint8_t palette = a[3];
    uint8_t i;
    volatile uint8_t *dst;

    if (!g_is_cgb) return;
    if (y >= 18 || x >= 32) return;
    if ((uint8_t)(x + len) > 32) len = (uint8_t)(32 - x);

    dst = (volatile uint8_t *)(0x9800 + ((uint16_t)y << 5) + x);
    VBK_REG = 1;
    for (i = 0; i < len; i++) {
        color_vram_sync_write(&dst[i], (uint8_t)(palette & 0x07));
    }
    VBK_REG = 0;
}

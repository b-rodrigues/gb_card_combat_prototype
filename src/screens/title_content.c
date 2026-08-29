#pragma bank 2
#pragma disable_warning 110

#include "ui.h"
#include "banked.h"
#include "screen.h"
#include "game.h"
#include <gb/gb.h>

/* Bank-2 self-contained title/intro render bodies (AGENTS.md 52.11.1):
 * the fixed _CODE/_HOME area is at its size limit (see make memmap), so
 * the entire title logo + menu draw lives here, writing VRAM directly
 * with bank-local helpers (a banked body must never call fixed-bank
 * functions -- see ui_battle_content.c for the same pattern).
 *
 * Staged inputs (set by the fixed wrapper in title_screen.c):
 *   g_bk_byte_a = title_menu_showing (0 = PRESS START, 1 = menu)
 *   g_bk_byte_b = title_menu_index  (0 = NEW GAME, 1 = CONTINUE, 2 = SOUND)
 */

extern char g_ui_screen_buf[18][21];
extern uint8_t ui_font_tile_base;
extern uint8_t g_sound_enabled;

static void title_vram_sync_write(volatile uint8_t *dst, uint8_t tile)
{
    if (LCDC_REG & 0x80) {
        __asm
            di
        __endasm;
        while (STAT_REG & 0x02);
        *dst = tile;
        __asm
            ei
        __endasm;
    } else {
        *dst = tile;
    }
}

static void title_put_char(uint8_t x, uint8_t y, char ch)
{
    if (y < 18 && x < 20) {
        if (g_ui_screen_buf[y][x] != ch) {
            uint8_t tile = (uint8_t)(ui_font_tile_base + (uint8_t)(ch - ' '));
            volatile uint8_t *dst = (volatile uint8_t *)(0x9800 + ((uint16_t)y << 5) + x);
            VBK_REG = 0;
            title_vram_sync_write(dst, tile);
            g_ui_screen_buf[y][x] = ch;
        }
    }
}

static void title_draw_text(uint8_t x, uint8_t y, const char *text, uint8_t max_chars)
{
    uint8_t i;
    char ch;
    volatile uint8_t *dst;
    char *buf;
    uint8_t ended;

    if (y >= 18) return;
    if ((uint8_t)(x + max_chars) > 20) max_chars = (uint8_t)(20 - x);

    VBK_REG = 0;
    ended = 0;
    dst = (volatile uint8_t *)(0x9800 + ((uint16_t)y << 5) + x);
    buf = &g_ui_screen_buf[y][x];

    for (i = 0; i < max_chars; i++) {
        ch = text[i];
        if (ch == '\0') ended = 1;
        if (ended) ch = ' ';
        if (*buf != ch) {
            uint8_t tile = (uint8_t)(ui_font_tile_base + (uint8_t)(ch - ' '));
            title_vram_sync_write(dst, tile);
            *buf = ch;
        }
        dst++;
        buf++;
    }
}

/* ASCII logo block for the title screen. */
static const char s_logo[5][20] = {
    "   G I A U S A R",
    "------------------",
    "The Waking Whale",
    " and the Closed",
    "       Sky",
};

static void title_draw_logo(void)
{
    uint8_t i;
    for (i = 0; i < 5; i++) {
        title_draw_text(0, (uint8_t)(i + 1), s_logo[i], 19);
    }
}

static void title_draw_menu(uint8_t index)
{
    title_draw_text(3, 10, index == 0 ? "> NEW GAME" : "  NEW GAME", 11);
    title_draw_text(3, 12, index == 1 ? "> CONTINUE" : "  CONTINUE", 11);
    if (g_sound_enabled) {
        title_draw_text(3, 14, index == 2 ? "> SOUND: ON " : "  SOUND: ON ", 13);
    } else {
        title_draw_text(3, 14, index == 2 ? "> SOUND: OFF" : "  SOUND: OFF", 13);
    }
    title_draw_text(3, 16, index == 3 ? "> TUTORIAL" : "  TUTORIAL", 10);
}

void title_content_render(void)
{
    uint8_t showing = g_bk_byte_a;
    uint8_t index = g_bk_byte_b;

    title_draw_logo();

    if (!showing) {
        title_draw_text(5, 16, "PRESS START", 13);
    } else {
        title_draw_menu(index);
    }
}

/* Pure caret move for the title menu (AGENTS.md 36): computes the next
 * index from the direction (staged in g_bk_byte_b, 1 = DOWN) and the
 * current index, writes it back through the staged Game* (WRAM always
 * mapped), and repaints only the two '>' cells -- mirroring
 * g_ui_screen_buf so the render cache stays valid and no full redraw
 * flickers on navigation.  Rows are 10, 12, 14, 16 for indices 0..3. */
void title_menu_step_banked(void)
{
    Game *g = (Game *)g_bk_ptr_a;
    uint8_t index;
    uint8_t new_index;
    uint8_t old_row;
    uint8_t new_row;
    uint8_t max_idx = 3;

    if (!g) return;
    index = g->title_menu_index;
    if (g_bk_byte_b) {
        if (index == max_idx) new_index = 0;
        else new_index = (uint8_t)(index + 1);
    } else {
        if (index == 0) new_index = max_idx;
        else new_index = (uint8_t)(index - 1);
    }
    old_row = (uint8_t)(10 + (index << 1));
    new_row = (uint8_t)(10 + (new_index << 1));
    if (old_row != new_row) {
        title_put_char(3, old_row, ' ');
        title_put_char(3, new_row, '>');
    }
    g->title_menu_index = new_index;
}

/* ── Intro: three scripted ASCII slides ───────────────────────────── */

static const char s_intro_0[4][20] = {
    "The skies above",
    "Giausar grow dark.",
    "A whale stirs",
    "in the deep.",
};
static const char s_intro_1[4][20] = {
    "The sky closes,",
    "sealed against",
    "the waking whale.",
    "",
};
static const char s_intro_2[4][20] = {
    "Only the Lord of",
    "Slimes stands",
    "between all that",
    "lives and the end.",
};

void intro_content_render(void)
{
    uint8_t slide = g_bk_byte_a;
    const char (*lines)[20];
    uint8_t count = 4;
    uint8_t i;

    if (slide == 0) lines = s_intro_0;
    else if (slide == 1) lines = s_intro_1;
    else lines = s_intro_2;

    for (i = 0; i < count; i++) {
        title_draw_text(3, (uint8_t)(6 + i), lines[i], 18);
    }
    title_draw_text(5, 16, "[A] NEXT", 10);
}

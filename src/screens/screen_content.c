#pragma bank 2
#pragma disable_warning 110

#include <gb/gb.h>
#include <gb/cgb.h>
#include "game.h"
#include "ui.h"
#include "banked.h"
#include "battle/card.h"
#include "rpg/cards.h"
#include "rpg/status.h"
#include "shops.h"
#include "menu.h"
#include "rpg/save.h"

/* Bank-2 self-contained shop / save-load render bodies (AGENTS.md 52.11.1):
 * the fixed _CODE/_HOME area is at its size limit (see make memmap), so the
 * full shop and save-load content draws live here, writing VRAM directly with
 * bank-local helpers (a banked body must never call fixed-bank functions --
 * see ui_battle_content.c / title_content.c for the same pattern).
 *
 * The fixed wrapper (shop_screen.c / save_load_screen.c) runs menu_draw_frame
 * (fixed: centered title + separator + full clear) and THIS body draws only
 * the dynamic content below the frame, so no bank-local clear is needed.
 *
 * Staged input: g_bk_ptr_a = Game*.
 */

extern char g_ui_screen_buf[18][21];
extern uint8_t ui_font_tile_base;
extern uint8_t g_is_cgb;

extern const CardDefinition *g_card_defs;
extern uint8_t g_card_defs_count;
extern const ShopDefinition g_shops[];

/* Reuse the bank-2 decimal formatter that ui_format_int() dispatches:
 * we are in the same bank (2), so call the no-arg body directly after
 * staging value + out pointer. */
extern void ui_format_int_banked(void);

/* Shop message states (mirror shop_screen.c). */
#define SC_SHOP_MSG_NONE 0
#define SC_SHOP_MSG_BOUGHT 1
#define SC_SHOP_MSG_NO_GOLD 2
#define SC_SHOP_MSG_MAX_COPIES 3

static void sc_vram_sync_write(volatile uint8_t *dst, uint8_t tile)
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

static void sc_put_char(uint8_t x, uint8_t y, char ch)
{
    uint8_t tile;

    if (y >= 18 || x >= 20) return;
    if (g_ui_screen_buf[y][x] == ch) return;
    tile = (uint8_t)(ui_font_tile_base + (uint8_t)(ch - ' '));
    VBK_REG = 0;
    sc_vram_sync_write((volatile uint8_t *)(0x9800 + ((uint16_t)y << 5) + x), tile);
    g_ui_screen_buf[y][x] = ch;
}

static void sc_draw_text(uint8_t x, uint8_t y, const char *text, uint8_t max_chars)
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
            sc_vram_sync_write(dst, tile);
            *buf = ch;
        }
        dst++;
        buf++;
    }
}

/* CGB per-tile palette for a horizontal span (mirrors ui_color_span_banked,
 * which lives in bank 3 and cannot be called from here). */
static void sc_color_span(uint8_t x, uint8_t y, uint8_t len, uint8_t palette)
{
    uint8_t i;
    volatile uint8_t *dst;

    if (!g_is_cgb) return;
    if (y >= 18 || x >= 32) return;
    if ((uint8_t)(x + len) > 32) len = (uint8_t)(32 - x);

    dst = (volatile uint8_t *)(0x9800 + ((uint16_t)y << 5) + x);
    VBK_REG = 1;
    for (i = 0; i < len; i++) {
        sc_vram_sync_write(&dst[i], (uint8_t)(palette & 0x07));
    }
    VBK_REG = 0;
}

static uint8_t sc_card_color(uint8_t battle_type, uint8_t status_id, uint8_t is_heal)
{
    if (status_id == STATUS_BURN) return UI_COLOR_FIRE;
    if (status_id == STATUS_POISON) return UI_COLOR_POISON;
    if (status_id == STATUS_FREEZE) return UI_COLOR_ICE;
    if (battle_type == BATTLE_CARD_TYPE_SHIELD || is_heal) return UI_COLOR_WOOD;
    if (battle_type == BATTLE_CARD_TYPE_SWORD) return UI_COLOR_IRON;
    if (battle_type == BATTLE_CARD_TYPE_BOW) return UI_COLOR_GOLD;
    if (battle_type == BATTLE_CARD_TYPE_DAGGER) return UI_COLOR_POISON;
    return UI_COLOR_NONE;
}

/* Two-letter battle-type code + power digits (mirrors ui_card_code_str).
 * s_card_bt_codes = "SW\0SH\0BO\0RG\0DA" indexed by battle_type*3. */
static const char sc_bt_codes[] = "SW\0SH\0BO\0RG\0DA";

static void sc_card_code_str(uint8_t battle_type, uint8_t power, char *out)
{
    const char *bt;
    uint8_t tens = 0;
    uint8_t ones = power;

    bt = (battle_type <= BATTLE_CARD_TYPE_DAGGER) ?
        (sc_bt_codes + ((uint16_t)battle_type * 3)) : "??";
    while (ones >= 10) {
        ones -= 10;
        tens++;
    }
    out[0] = bt[0];
    out[1] = bt[1];
    if (power >= 10) {
        out[2] = (char)('0' + tens);
        out[3] = (char)('0' + ones);
        out[4] = '\0';
    } else {
        out[2] = (char)('0' + ones);
        out[3] = '\0';
    }
}

static void sc_format_int(int16_t value, char *out)
{
    if (!out) return;
    g_bk_byte_a = (uint8_t)(value & 0xFF);
    g_bk_byte_b = (uint8_t)((uint16_t)value >> 8);
    g_bk_ptr_a = (void *)out;
    ui_format_int_banked();
}

/* Direct bank-2 lookup of a regular (non-loot) card id.  Shop stock only
 * holds named game-card ids (shops_content.c), so the loot-synth branch is
 * unnecessary here.  Returns NULL if not found. */
static const CardDefinition *sc_card_get_def(CardId id)
{
    uint8_t i;
    if (!g_card_defs) return NULL;
    for (i = 0; i < g_card_defs_count; i++) {
        if (g_card_defs[i].id == id) return &g_card_defs[i];
    }
    return NULL;
}

/* Active shop stock, read directly from the bank-2 table (game_shop_for_id
 * uses banked_copy from the same bank). */
static const ShopDefinition *sc_shop_active(const Game *g)
{
    uint8_t id = (g && g->shop_id >= 1 && g->shop_id <= 2) ? g->shop_id : 1;
    return &g_shops[id - 1];
}

/* Cursor-only menu move (ITEM_DIRTY_CURSOR, AGENTS.md 36): called from the
 * ITEM and SHOP screens' update() on UP/DOWN.  Repaints just the `>` glyph:
 * two 1-tile writes, no frame/tab redraw and no LCD-off, so the music ISR
 * stays uninterrupted (AGENTS.md 35/36/52.18).  The wider body also owns the
 * window-shift decision: when the CARDS window must scroll, it moves the
 * scroll and flags the render cache so render() takes the rare full-redraw
 * path; a pure caret move leaves the render cache untouched.
 *
 * Banked so the row math and VRAM writes stay out of the fixed bank
 * (AGENTS.md 52.18).  Inputs via staging: g_bk_ptr_a = Game*, g_bk_byte_a =
 * old index, g_bk_byte_b = new index.  Writes game state through the staged
 * Game* (WRAM is always mapped).  The row layout is derived from
 * g->item_menu_tab for the ITEM screen; when g->screen == SCREEN_SHOP the
 * rows are linear 5 + i (shop_content_render).  ITEM_FIRST_CARD /
 * ITEM_VISIBLE_CARD mirror src/screens/item_screen.c (the CARDS window
 * layout: ROW_TOP 0, FIRST_CARD 1, VISIBLE_CARDS 5). */
#define ITEM_FIRST_CARD   1
#define ITEM_VISIBLE_CARD 5

static uint8_t bm_mode(const Game *g)
{
    return (g->screen == SCREEN_SHOP) ? 2 : (uint8_t)g->item_menu_tab;
}

static uint8_t bm_cursor_row(uint8_t mode, uint8_t index, uint8_t scroll)
{
    uint8_t pos;
    if (mode == 2)                /* SHOP: linear rows */
        return (uint8_t)(5u + index);
    if (mode == 1)                /* TAB_QUEST */
        return (uint8_t)(5 + (index << 1));
    if (index == 0)               /* ROW_TOP (FILTER/SORT row) */
        return 5;
    pos = (uint8_t)(index - ITEM_FIRST_CARD);
    if (scroll == 0)
        return (uint8_t)(6 + (pos << 1));
    return (uint8_t)(5 + ((pos - scroll) << 1));
}

void item_menu_cursor_banked(void)
{
    Game *g = (Game *)g_bk_ptr_a;
    uint8_t mode, old_index, new_index;
    uint8_t old_row, new_row;
    uint8_t pos, need_scroll;

    if (!g) return;
    mode = bm_mode(g);
    old_index = g_bk_byte_a;
    new_index = g_bk_byte_b;
    g->item_menu_index = new_index;

    /* Window-shift decision (CARDS tab only): the visible card window must
     * follow the caret.  When it moves, let render() do the full redraw
     * (the per-cell row math is meaningless outside the visible window). */
    if (mode == 0) {
        need_scroll = g->item_menu_scroll;
        if (new_index <= ITEM_FIRST_CARD)
            need_scroll = 0;
        else {
            pos = (uint8_t)(new_index - ITEM_FIRST_CARD);
            if (pos < need_scroll)
                need_scroll = pos;
            else if (pos > (uint8_t)(need_scroll + ITEM_VISIBLE_CARD - 1))
                need_scroll = (uint8_t)(pos - (ITEM_VISIBLE_CARD - 1));
        }
        if (need_scroll != g->item_menu_scroll) {
            g->item_menu_scroll = need_scroll;
            g->render_cache.valid = false;   /* rare window shift -> full redraw */
            return;
        }
    }

    /* Pure caret move: repaint the two cells only. */
    old_row = bm_cursor_row(mode, old_index, g->item_menu_scroll);
    new_row = bm_cursor_row(mode, new_index, g->item_menu_scroll);
    if (old_row != new_row) {
        sc_put_char(0, old_row, ' ');
        sc_put_char(0, new_row, '>');
    }
}

void shop_content_render(void)
{
    Game *g = (Game *)g_bk_ptr_a;
    const ShopDefinition *def;
    char str[7];
    uint8_t i, y;
    int16_t gold;

    if (!g) return;
    def = sc_shop_active(g);
    gold = g->state.currency.amount[0];

    sc_draw_text(0, 3, "GOLD:", 5);
    sc_format_int(gold, str);
    sc_draw_text(5, 3, str, 14);

    if (!def) {
        sc_draw_text(0, 5, "(nothing)", 9);
        sc_draw_text(0, 7, "[B] Leave", 9);
        return;
    }

    for (i = 0; i < def->count; i++) {
        const CardDefinition *card = sc_card_get_def(def->items[i]);
        char code[6];
        y = (uint8_t)(5 + i);
        sc_put_char(0, y, (g->item_menu_index == i) ? '>' : ' ');
        if (card) {
            sc_card_code_str(card->battle_type, card->power, code);
            sc_draw_text(1, y, code, 4);
            sc_color_span(1, y, 4,
                          sc_card_color(card->battle_type, card->status_id,
                                         (card->battle_type == BATTLE_CARD_TYPE_HEAL) ||
                                         (card->effect == CARD_EFFECT_HEAL_HP)));
        } else {
            sc_draw_text(1, y, "???", 3);
        }
        sc_format_int(card ? (int16_t)card->price : 0, str);
        sc_draw_text(12, y, str, 4);
        sc_put_char(16, y, 'G');
    }

    sc_draw_text(0, (uint8_t)(6 + def->count), "[A] Buy  [B] Leave", 18);
    if (g->shop_message != SC_SHOP_MSG_NONE) {
        sc_draw_text(0, (uint8_t)(8 + def->count),
                     (g->shop_message == SC_SHOP_MSG_BOUGHT) ? "Bought!" :
                     (g->shop_message == SC_SHOP_MSG_MAX_COPIES) ? "Too many!" : "Not enough!",
                     12);
    }
}

/* ── Save / load screen content ──────────────────────────────────────
 * The fixed wrapper pre-computes each slot's present/empty state with the
 * real save_present_slot() (full GameState checksum) and stages a bitmask
 * in g_bk_byte_a -- bit 0 = slot 0 present, bit 1 = slot 1, bit 2 = slot 2.
 * The banked body only draws the resulting text; it must not re-derive the
 * checksum (save_present_slot dispatches a bank-3 body we cannot call).  */

void save_load_content_render(void)
{
    Game *g = (Game *)g_bk_ptr_a;
    uint8_t present = g_bk_byte_a;
    uint8_t i, y;

    if (!g) return;

    for (i = 0; i < SAVE_SLOT_COUNT; i++) {
        y = (uint8_t)(4 + (i << 1));
        sc_put_char(0, y, (g->save_slot_index == i) ? '>' : ' ');
        sc_draw_text(1, y, (i == 0) ? "SLOT 1:" : ((i == 1) ? "SLOT 2:" : "SLOT 3:"), 7);
        sc_draw_text(9, y, (present & (1 << i)) ? "SAVED" : "(EMPTY)", 7);
    }
    sc_draw_text(0, 11, (g->save_slot_mode == 1) ? "[A] SAVE  " : "[A] LOAD  ", 10);
    sc_draw_text(10, 11, "[B] BACK", 8);
    if (g->save_slot_message != 0) {
        sc_draw_text(2, 13, (g->save_slot_message == 1) ? "Game Saved!" : "Slot is empty!", 14);
    }
}

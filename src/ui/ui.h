#ifndef UI_H
#define UI_H

#include "world.h"
#include "battle.h"
#include "dialogue.h"
#include <stdint.h>

extern char g_ui_screen_buf[18][21];
extern uint8_t ui_font_tile_base;

/* CGB present flag (detected at ui_init).  Gates VRAM bank-1 attribute
 * writes; DMG = 0. */
extern uint8_t g_is_cgb;

/* Per-tile background palette indices (CGB VRAM bank-1 attributes):
 * 0 = default grayscale, 1 = fire, 2 = ice, 3 = heal, 4 = poison. */
#define UI_COLOR_NONE   0
#define UI_COLOR_FIRE   1
#define UI_COLOR_ICE    2
#define UI_COLOR_HEAL   3
#define UI_COLOR_POISON 4

/* Effect color for a card (status_id = on-hit rider element, is_heal =
 * ring/heal role).  Returns a UI_COLOR_* palette index. */
uint8_t ui_color_class(uint8_t status_id, uint8_t is_heal);

/* Set the CGB per-tile palette for a horizontal span of background tiles
 * (no-op on DMG and for palette 0). */
void ui_color_span(uint8_t x, uint8_t y, uint8_t len, uint8_t palette);

void ui_init(void);
void ui_clear_screen(void);

/* Player rendered as a real OAM sprite.  Background stays console-font
 * ASCII; only the player is a hardware sprite.  Deliberately scoped to the
 * ui layer: no other module needs to know the player is a sprite instead
 * of a printed '@'. */
void ui_sprite_init(void);
void ui_sprite_move(uint8_t px, uint8_t py);
void ui_sprite_hide(void);
void ui_sprite_begin_transition(void);
void ui_sprite_commit(void);
void oam_dma_init(void);

/* Toggle LCDC bit 7 directly (harness-safe: no GBDK display_off VBlank
 * wait).  Full-screen redraws span several display sweeps and cannot fit
 * in one VBlank, so every full redraw runs with the LCD off and all VRAM
 * writes land deterministically. */
void ui_lcd_off(void);
void ui_lcd_on(void);

void ui_draw_world_map(const World *world);
void ui_draw_world_full(const World *world);
void ui_draw_actors_sprites(const World *world);
void ui_draw_text_line(uint8_t x, uint8_t y, const char *text, uint8_t max_chars);
void ui_draw_num2(uint8_t x, uint8_t y, uint8_t val);
void ui_draw_hline(uint8_t y, char ch);

/* Set SCX/SCY from the overworld camera pixel position.  Called every
 * overworld frame so the background glides smoothly. */
void ui_update_camera(const World *world);

/* Write value as a decimal string into out (at least 7 bytes).  Avoids the
 * stdio/console chain so _HOME stays under 0x8000. */
void ui_format_int(int16_t value, char *out);

/* Write "XYn" — two-letter battle code ("SW"/"SH"/"BO"/"HE"/"DA" per
 * BattleCardType in src/battle/card.h) plus power digit — into out
 * (at least 4 bytes); "??" when battle_type is out of range. */
void ui_card_code_str(uint8_t battle_type, uint8_t power, char *out);

/* Ones digit of `v` as a character, computed by subtraction: SM83 has
 * no hardware divide and the SDCC divmod library would land in the
 * tight fixed bank (AGENTS.md 52.18).  Use instead of `% 10`. */
char ui_ones_digit(uint8_t v);

/* Banked no-arg body (ROM bank 2) dispatched by ui_format_int(). */
void ui_format_int_banked(void);

/* Bank-3 no-arg body dispatched by ui_color_span() (src/ui/ui_color_banked.c);
 * reads its staged args from g_bk_ptr_a. */
void ui_color_span_banked(void);

void ui_draw_dialogue(const DialogueState *dialogue, uint8_t scroll_x, uint8_t scroll_y);
void ui_draw_dialogue_line(uint8_t x, uint8_t y, const char *text,
                           uint8_t max_chars, uint8_t ox, uint8_t oy);
void ui_draw_battle_full(const Battle *battle);
void ui_update_battle(const Battle *battle);
void ui_update_battle_banked(void);
void ui_draw_battle_timer(const Battle *battle);
uint8_t ui_calc_timer_bar(uint16_t t);

void ui_draw_font_test(void);

#endif /* UI_H */

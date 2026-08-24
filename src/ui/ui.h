#ifndef UI_H
#define UI_H

#include "world.h"
#include "battle.h"
#include "dialogue.h"
#include <stdint.h>

extern char g_ui_screen_buf[18][21];
extern uint8_t ui_font_tile_base;

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

/* Write "XYn" — two-letter battle code ("SW"/"SH"/"BO"/"FI"/"HE" per
 * BattleCardType in src/battle/card.h) plus power digit — into out
 * (at least 4 bytes); "??" when battle_type is out of range. */
void ui_card_code_str(uint8_t battle_type, uint8_t power, char *out);

/* Ones digit of `v` as a character, computed by subtraction: SM83 has
 * no hardware divide and the SDCC divmod library would land in the
 * tight fixed bank (AGENTS.md 52.18).  Use instead of `% 10`. */
char ui_ones_digit(uint8_t v);

/* Banked no-arg body (ROM bank 2) dispatched by ui_format_int(). */
void ui_format_int_banked(void);

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

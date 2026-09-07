#ifndef BATTLE_DATA_H
#define BATTLE_DATA_H

#include <stdint.h>

/* Battle screen definition (matches generated battle_screens.c) */
typedef struct BattleScreenDef {
    const char *id;
    const char *label;
    uint8_t max_enemies;
    uint8_t allowed_categories;  // bit 0=minion, 1=elite, 2=boss
    uint8_t enemy_positions[3][2];
    uint8_t timer_overworld_ticks;
    uint8_t timer_battle_ticks;
    uint8_t turn_banner_row;
    uint8_t enemy_hp_row;
    uint8_t enemy_sprite_row;
    uint8_t enemy_cursor_row;
    uint8_t enemy_col_start;
    uint8_t enemy_col_step;
    uint8_t hero_label_row;
    uint8_t hero_label_col;
    uint8_t hero_hp_row;
    uint8_t hero_hp_col;
    uint8_t deck_row;
    uint8_t deck_col;
    uint8_t ap_row;
    uint8_t ap_col;
    uint8_t combo_row;
    uint8_t cards_row;
    uint8_t card_cursor_row;
    uint8_t card_desc_row;
    uint8_t timer_row;
    uint8_t timer_col;
    uint8_t timer_width;
    uint8_t hud_enemy_row_start;
    uint8_t hud_enemy_row_step;
    uint8_t hud_deck_row;
    uint8_t hud_combo_row_start;
    uint8_t hud_combo_row_step;
    uint8_t hud_timer_row;
    uint8_t hud_caret_x;
} BattleScreenDef;

/* Enemy type definition (matches generated battle_types.c) */
typedef struct EnemyTypeDef {
    const char *id;
    const char *label;
    uint8_t category;  // 0=minion, 1=elite, 2=boss
    const char *name;
    uint8_t base_hp;
    uint8_t base_max_hp;
    const char *battle_id;
    uint8_t gold_reward;
    uint8_t reward_currency;
    uint8_t art_index;  // battle art set (order position in screens/combat_art), 0xFF = text fallback
    uint8_t art_frames;  // animation frames (1..2, 0 when art_index is 0xFF)
    uint8_t art_palette;  // CGB battle palette (ui_color_* index; DMG ignores)
    uint8_t art_w;  // combat art width in tiles (3 for the built-in sets; up to 6)
    uint8_t art_h;  // combat art height in tiles (2 for the built-in sets; up to 4)
    uint16_t art_offset;  // blob tile offset of this set's frame0 in battle_enemy_art.h
    uint8_t ow_tile;  // shared overworld OAM base tile (ENEMY_OW_BASE + blob offset), 0xFF = legacy SPRITE_KIND path
    uint8_t ow_w;  // overworld sprite width in tiles (1 = single-tile sprite, 2 = 2x2 grid)
    uint8_t ow_h;  // overworld sprite height in tiles (1 = single-tile sprite, 2 = 2x2 grid)
    uint8_t ow_frames;  // overworld animation frames (cells / (ow_w*ow_h), 0 when ow_tile is 0xFF)
    uint8_t ow_palette;  // CGB OAM palette index
} EnemyTypeDef;

/* WRAM cache for the active battle screen's HUD layout.
 * Staged from the bank-4 BattleScreenDef by game_battle_hud_load()
 * at every battle entry; read by the bank-3 renderer.
 * All fields are uint8_t.  Field names AND order mirror BattleScreenDef
 * from enemy_positions onward on purpose: the bank-4 loader copies
 * field-by-field with identical names, so a reorder/insert on either
 * side is a review-visible mismatch, not a silent shift.  Keep the two
 * structs in sync (the file-scope asserts in battle_hud_load_banked.c
 * and the battle_compile.py field-order check enforce this). */
typedef struct BattleHudCache {
    uint8_t enemy_positions[3][2];
    uint8_t timer_overworld_ticks;
    uint8_t timer_battle_ticks;
    uint8_t turn_banner_row;
    uint8_t enemy_hp_row;
    uint8_t enemy_sprite_row;
    uint8_t enemy_cursor_row;
    uint8_t enemy_col_start;
    uint8_t enemy_col_step;
    uint8_t hero_label_row;
    uint8_t hero_label_col;
    uint8_t hero_hp_row;
    uint8_t hero_hp_col;
    uint8_t deck_row;
    uint8_t deck_col;
    uint8_t ap_row;
    uint8_t ap_col;
    uint8_t combo_row;
    uint8_t cards_row;
    uint8_t card_cursor_row;
    uint8_t card_desc_row;
    uint8_t timer_row;
    uint8_t timer_col;
    uint8_t timer_width;
    uint8_t hud_enemy_row_start;
    uint8_t hud_enemy_row_step;
    uint8_t hud_deck_row;
    uint8_t hud_combo_row_start;
    uint8_t hud_combo_row_step;
    uint8_t hud_timer_row;
    uint8_t hud_caret_x;
} BattleHudCache;

/* WRAM cache for the active battle screen's HUD layout.
 * Staged from the bank-4 BattleScreenDef by game_battle_hud_load()
 * at every battle entry; read by the bank-3 renderer. */
extern BattleHudCache g_battle_hud;

/* Generated data declarations (bank 4) */
extern const BattleScreenDef* const g_battle_screens[];
extern const uint8_t g_battle_screen_count;

extern const EnemyTypeDef* const g_enemy_types[];
extern const uint8_t g_enemy_type_count;
/* Shared overworld enemy OAM blob size in tiles (for the ui_init stream). */
extern const uint8_t g_enemy_ow_tile_count;

#endif /* BATTLE_DATA_H */
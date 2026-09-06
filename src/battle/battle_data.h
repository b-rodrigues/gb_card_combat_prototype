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
    uint8_t art_index;  // battle art set (0=slime,1=bat,2=boss), 0xFF = text fallback
    uint8_t art_frames;  // animation frames (1..2, 0 when art_index is 0xFF)
    uint8_t art_palette;  // CGB battle palette (ui_color_* index; DMG ignores)
    uint8_t art_w;  // combat art width in tiles (3 for the built-in sets; up to 6)
    uint8_t art_h;  // combat art height in tiles (2 for the built-in sets; up to 4)
} EnemyTypeDef;

/* WRAM cache for the active battle screen's HUD layout.
 * Staged from the bank-4 BattleScreenDef by game_battle_hud_load()
 * at every battle entry; read by the bank-3 renderer.
 * All fields are uint8_t; layout mirrors BattleScreenDef from
 * enemy_positions onward (indices match the struct order). */
typedef struct BattleHudCache {
    uint8_t pos[3][2];      // enemy positions (x,y)
    uint8_t tick_o;         // timer_overworld_ticks
    uint8_t tick_b;         // timer_battle_ticks
    uint8_t rows[28];       // HUD rows/cols in BattleScreenDef order:
    // 0: turn_banner_row
    // 1: enemy_hp_row
    // 2: enemy_sprite_row
    // 3: enemy_cursor_row
    // 4: enemy_col_start
    // 5: enemy_col_step
    // 6: hero_label_row
    // 7: hero_label_col
    // 8: hero_hp_row
    // 9: hero_hp_col
    // 10: deck_row
    // 11: deck_col
    // 12: ap_row
    // 13: ap_col
    // 14: combo_row
    // 15: cards_row
    // 16: card_cursor_row
    // 17: card_desc_row
    // 18: timer_row
    // 19: timer_col
    // 20: timer_width
    // 21: hud_enemy_row_start
    // 22: hud_enemy_row_step
    // 23: hud_deck_row
    // 24: hud_combo_row_start
    // 25: hud_combo_row_step
    // 26: hud_timer_row
    // 27: hud_caret_x
} BattleHudCache;

/* Indices for rows[] array -- matches BattleScreenDef order from turn_banner_row */
#define HUD_BANNER_ROW        0
#define HUD_ENEMY_HP          1
#define HUD_ENEMY_SPRITE      2
#define HUD_ENEMY_CURSOR      3
#define HUD_ENEMY_COL_START   4
#define HUD_ENEMY_COL_STEP    5
#define HUD_HERO_LROW         6
#define HUD_HERO_LCOL         7
#define HUD_HERO_HROW         8
#define HUD_HERO_HCOL         9
#define HUD_DECK_ROW          10
#define HUD_DECK_COL          11
#define HUD_AP_ROW            12
#define HUD_AP_COL            13
#define HUD_COMBO_ROW         14
#define HUD_CARDS_ROW         15
#define HUD_CARD_CUR_ROW      16
#define HUD_CARD_DSC_ROW      17
#define HUD_TIMER_ROW         18
#define HUD_TIMER_COL         19
#define HUD_TIMER_W           20
#define HUD_ENEMY_ROW_START   21
#define HUD_ENEMY_ROW_STEP    22
#define HUD_DECK_ROW2         23
#define HUD_COMBO_ROW_START   24
#define HUD_COMBO_ROW_STEP    25
#define HUD_TIMER_ROW2        26
#define HUD_CARET_X           27

/* WRAM cache for the active battle screen's HUD layout.
 * Staged from the bank-4 BattleScreenDef by game_battle_hud_load()
 * at every battle entry; read by the bank-3 renderer. */
extern BattleHudCache g_battle_hud;

/* Generated data declarations (bank 4) */
extern const BattleScreenDef* const g_battle_screens[];
extern const uint8_t g_battle_screen_count;

extern const EnemyTypeDef* const g_enemy_types[];
extern const uint8_t g_enemy_type_count;

#endif /* BATTLE_DATA_H */
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
    const char *sprite;
    const char *name;
    uint8_t base_hp;
    uint8_t base_max_hp;
    const char *battle_id;
    uint8_t gold_reward;
    uint8_t reward_currency;
} EnemyTypeDef;

/* Generated data declarations (bank 4) */
extern const BattleScreenDef* const g_battle_screens[];
extern const uint8_t g_battle_screen_count;

extern const EnemyTypeDef* const g_enemy_types[];
extern const uint8_t g_enemy_type_count;

#endif /* BATTLE_DATA_H */
#ifndef BATTLE_H
#define BATTLE_H

#include "combatant.h"
#include "card.h"
#include "combo.h"
#include "deck.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_BATTLE_ENEMIES 3
#define BATTLE_HAND_SIZE 5
#define BATTLE_TIMER_MAX_FRAMES 1200 /* 20 seconds at 60fps */
/* Timer-bar divider: 20 tiles drained across 20s = 1 tile/second.  Keep in
 * sync between ui_draw_battle_timer() (ui.c) and calc_timer_bar()
 * (battle_screen.c). */
#define BATTLE_TIMER_BAR_DIVISOR 60

typedef enum {
    BATTLE_PHASE_PLAYER_SELECT = 0,    /* Player selecting attack cards */
    BATTLE_PHASE_PLAYER_ANIM = 1,      /* Executing attack / heal */
    BATTLE_PHASE_ENEMY_TELEGRAPH = 2,  /* Enemy announces incoming attack */
    BATTLE_PHASE_PLAYER_DEFEND = 3,    /* Player selecting shield defense cards */
    BATTLE_PHASE_DEFENSE_RESOLVE = 4,  /* Resolving defense & damage taken */
    BATTLE_PHASE_RESULT = 5            /* Victory / Defeat / Fled */
} BattlePhase;

/* BattleTurn enum preserved for telemetry & backward compatibility */
typedef enum {
    BATTLE_TURN_PLAYER = 0,
    BATTLE_TURN_ENEMY_DELAY = 1,
    BATTLE_TURN_ENEMY = 2,
    BATTLE_TURN_RESULT = 3
} BattleTurn;

typedef enum {
    BATTLE_RESULT_NONE = 0,
    BATTLE_RESULT_VICTORY = 1,
    BATTLE_RESULT_DEFEAT = 2,
    BATTLE_RESULT_FLED = 3
} BattleResult;

typedef struct {
    Combatant player;
    Combatant enemies[MAX_BATTLE_ENEMIES];
    uint8_t enemy_count;                       /* 1..3 */
    uint8_t target_idx;                        /* 0..enemy_count-1 */
    uint8_t dirty;
    Deck deck;
    Card hand[BATTLE_HAND_SIZE];
    uint8_t selected_indices[BATTLE_HAND_SIZE]; /* Hand indices in combo order */
    uint8_t combo_count;                       /* Number of selected cards (0..5) */
    uint8_t cursor_pos;                        /* 0..4 in hand */
    uint16_t timer_ticks;                      /* Countdown timer ticks remaining */
    uint16_t timer_max;                        /* Max timer ticks (1200) */
    BattlePhase phase;
    BattleTurn turn;                           /* Mirror for telemetry / snapshots */
    BattleResult result;
    uint8_t delay_timer;                       /* Animation / state advance delay */
    bool battle_over;
    ComboResult last_combo;
    uint8_t enemy_incoming_dmg;                /* Enemy attack power telegraphed */
} Battle;

void battle_start(Battle *b, const char *enemy_name, uint8_t player_hp,
                  uint8_t player_max_hp, uint8_t player_attack,
                  uint8_t enemy_hp, uint8_t enemy_max_hp);
void battle_cursor_move(Battle *b, int8_t dir);
void battle_target_move(Battle *b, int8_t dir);
void battle_target_auto_advance(Battle *b);
bool battle_all_enemies_dead(const Battle *b);
uint8_t battle_calc_enemy_attack(const Battle *b);
void battle_card_select(Battle *b);
void battle_card_undo(Battle *b);
void battle_execute_combo(Battle *b);
void battle_update(Battle *b);
bool battle_is_card_selected(const Battle *b, uint8_t hand_idx);

#endif /* BATTLE_H */

#ifndef BATTLE_H
#define BATTLE_H

#include "combatant.h"
#include "card.h"
#include "combo.h"
#include "deck.h"
#include "rpg/deck.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_BATTLE_ENEMIES 3
#define BATTLE_HAND_SIZE 5
#define BATTLE_TIMER_MAX_FRAMES 1200 /* 20 seconds at 60fps */
/* Timer cadence.  The bar is BATTLE_TIMER_BAR_LENGTH cells, each draining one
 * tile every BATTLE_TIMER_BAR_DIVISOR frames (= BATTLE_TIMER_SECONDS), so the
 * whole window is an exact integer number of cells.  The divisor is derived
 * from the length so MAX_FRAMES / BAR_LENGTH is always integral; a non-integer
 * split would make individual tiles drain over varying frames (audible/visible
 * skips).  A compile-time guard in battle.c enforces the divisibility.  Keep
 * bar length and divisor in sync with ui_draw_battle_timer()/calc_timer_bar(). */
#define BATTLE_TIMER_BAR_LENGTH  20
#define BATTLE_TIMER_BAR_DIVISOR (BATTLE_TIMER_MAX_FRAMES / BATTLE_TIMER_BAR_LENGTH)
#define BATTLE_TIMER_SECONDS     (BATTLE_TIMER_MAX_FRAMES / BATTLE_TIMER_BAR_DIVISOR)

#define BATTLE_DIRTY_BANNER  0x01
#define BATTLE_DIRTY_ENEMIES 0x02
#define BATTLE_DIRTY_HERO    0x04
#define BATTLE_DIRTY_COMBO   0x08
#define BATTLE_DIRTY_HAND    0x10
#define BATTLE_DIRTY_DESC    0x20
#define BATTLE_DIRTY_BLINK   0x40
#define BATTLE_DIRTY_ALL     0xFF

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
    uint8_t attacking_enemy_idx;               /* Index of enemy currently attacking / blinking */
} Battle;

void battle_start(Battle *b, const char *enemy_name, uint8_t player_hp,
                  uint8_t player_max_hp, uint8_t player_attack,
                  uint8_t enemy_hp, uint8_t enemy_max_hp,
                  const DeckState *ds);
void battle_add_enemy(Battle *b, const char *name, uint8_t hp, uint8_t max_hp, uint8_t attack);
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

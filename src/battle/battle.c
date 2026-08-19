#include "battle.h"
#include "telemetry.h"
#include "rpg/cards.h"
#include "rpg/deck.h"
#include "game/game_ids.h"
#include <string.h>

/* ── Bridge: persistent DeckState → battle Deck ───────────────────
 * When a DeckState is provided (player has cards), build the battle
 * deck from the player's owned cards.  When NULL, fall back to the
 * hardcoded starter deck (all unlimited uses). */
static void battle_init_from_deck_state(Battle *b, const DeckState *ds)
{
    uint8_t i;
    const CardDefinition *def;
    b->deck.count = ds->count;
    b->deck.draw_idx = 0;
    b->deck.discard_count = 0;
    for (i = 0; i < ds->count; i++) {
        def = card_get_def(ds->cards[i]);
        if (def) {
            b->deck.cards[i].type = def->battle_type;
            b->deck.cards[i].value = def->power;
            b->deck.cards[i].uses_remaining =
                (def->uses_per_battle == 0) ? 0xFF : def->uses_per_battle;
        } else {
            b->deck.cards[i].type = BATTLE_CARD_TYPE_SWORD;
            b->deck.cards[i].value = 2;
            b->deck.cards[i].uses_remaining = 0xFF;
        }
    }
}

/* Compile-time guarantee that the timer window is an exact integer number of
 * bar cells and whole seconds: every '=' drains one cell per
 * BATTLE_TIMER_SECONDS frames with no fractional remainder (which would make
 * tiles drain over uneven frame counts).  Same array-size trap as input.c. */
static const int g_battle_timer_cadence_ok[
    (BATTLE_TIMER_MAX_FRAMES % BATTLE_TIMER_BAR_LENGTH == 0) &&
    (BATTLE_TIMER_BAR_DIVISOR == BATTLE_TIMER_MAX_FRAMES / BATTLE_TIMER_BAR_LENGTH) ? 1 : 0
];

void battle_start(Battle *b, const char *enemy_name, uint8_t player_hp,
                  uint8_t player_max_hp, uint8_t player_attack,
                  uint8_t enemy_hp, uint8_t enemy_max_hp,
                  const DeckState *ds)
{
    uint8_t *p = (uint8_t *)b;
    uint16_t n = sizeof(Battle);
    uint8_t i;
    if (!b) return;

    while (n--) *p++ = 0;
    b->player.name = "Hero";
    b->player.hp = player_hp;
    b->player.max_hp = player_max_hp;
    b->player.attack = player_attack;

    b->enemies[0].name = enemy_name ? enemy_name : "Enemy";
    b->enemies[0].hp = enemy_hp;
    b->enemies[0].max_hp = enemy_max_hp;
    b->enemies[0].attack = 3;
    b->enemy_count = 1;

    if (ds && ds->count > 0) {
        battle_init_from_deck_state(b, ds);
    } else {
        deck_init_default(&b->deck);
    }
    for (i = 0; i < BATTLE_HAND_SIZE; i++) {
        deck_draw(&b->deck, &b->hand[i]);
    }

    b->timer_ticks = BATTLE_TIMER_MAX_FRAMES;
    b->timer_max = BATTLE_TIMER_MAX_FRAMES;
    b->phase = BATTLE_PHASE_PLAYER_SELECT;
    b->turn = BATTLE_TURN_PLAYER;
    b->dirty = BATTLE_DIRTY_ALL;

    telemetry_emit(EVENT_BATTLE_STARTED, 0, 0, 0, 0);
}

void battle_add_enemy(Battle *b, const char *name, uint8_t hp, uint8_t max_hp, uint8_t attack)
{
    uint8_t idx;
    if (!b || b->enemy_count >= MAX_BATTLE_ENEMIES) return;
    idx = b->enemy_count++;
    b->enemies[idx].name = name ? name : "Enemy";
    b->enemies[idx].hp = hp;
    b->enemies[idx].max_hp = max_hp;
    b->enemies[idx].attack = attack;
    b->dirty = BATTLE_DIRTY_ALL;
}

bool battle_is_card_selected(const Battle *b, uint8_t hand_idx)
{
    uint8_t i;
    if (!b) return false;
    for (i = 0; i < b->combo_count; i++) {
        if (b->selected_indices[i] == hand_idx) {
            return true;
        }
    }
    return false;
}

void battle_cursor_move(Battle *b, int8_t dir)
{
    uint8_t start, step;
    if (!b) return;
    if (b->phase != BATTLE_PHASE_PLAYER_SELECT && b->phase != BATTLE_PHASE_PLAYER_DEFEND) return;
    start = b->cursor_pos;
    for (step = 0; step < BATTLE_HAND_SIZE; step++) {
        if (dir < 0) {
            b->cursor_pos = (b->cursor_pos == 0) ? (BATTLE_HAND_SIZE - 1) : (uint8_t)(b->cursor_pos - 1);
        } else {
            b->cursor_pos = (uint8_t)((b->cursor_pos + 1 >= BATTLE_HAND_SIZE) ? 0 : (b->cursor_pos + 1));
        }
        if (b->hand[b->cursor_pos].uses_remaining != 0) break;
        if (b->cursor_pos == start) break;
    }
    b->dirty |= (BATTLE_DIRTY_HAND | BATTLE_DIRTY_DESC);
}

void battle_target_move(Battle *b, int8_t dir)
{
    uint8_t i, t;
    if (!b || b->enemy_count <= 1) return;

    t = b->target_idx;
    for (i = 0; i < b->enemy_count; i++) {
        if (dir < 0) {
            t = (t == 0) ? (uint8_t)(b->enemy_count - 1) : (uint8_t)(t - 1);
        } else {
            t = (uint8_t)((t + 1 >= b->enemy_count) ? 0 : (t + 1));
        }
        if (b->enemies[t].hp != 0) {
            b->target_idx = t;
            b->dirty |= BATTLE_DIRTY_ENEMIES;
            return;
        }
    }
}

void battle_target_auto_advance(Battle *b)
{
    if (b && b->enemies[b->target_idx].hp == 0) {
        battle_target_move(b, 1);
    }
}

bool battle_all_enemies_dead(const Battle *b)
{
    uint8_t i;
    if (!b) return true;
    for (i = 0; i < b->enemy_count; i++) {
        if (b->enemies[i].hp != 0) return false;
    }
    return true;
}

uint8_t battle_calc_enemy_attack(const Battle *b)
{
    uint8_t i, total = 0;
    if (!b || b->enemy_count == 0) return 3;
    for (i = 0; i < b->enemy_count; i++) {
        if (b->enemies[i].hp != 0) {
            total = (uint8_t)(total + (b->enemies[i].attack ? b->enemies[i].attack : 3));
        }
    }
    return total ? total : 3;
}

void battle_card_select(Battle *b)
{
    uint8_t step, next_pos;
    if (!b) return;
    if (b->phase != BATTLE_PHASE_PLAYER_SELECT && b->phase != BATTLE_PHASE_PLAYER_DEFEND) {
        return;
    }

    if (battle_is_card_selected(b, b->cursor_pos)) {
        return;
    }

    if (b->hand[b->cursor_pos].uses_remaining == 0) {
        return;
    }

    if (b->combo_count < BATTLE_HAND_SIZE) {
        b->selected_indices[b->combo_count++] = b->cursor_pos;
        b->dirty |= (BATTLE_DIRTY_COMBO | BATTLE_DIRTY_HAND | BATTLE_DIRTY_DESC);

        next_pos = b->cursor_pos;
        for (step = 1; step < BATTLE_HAND_SIZE; step++) {
            next_pos++;
            if (next_pos >= BATTLE_HAND_SIZE) next_pos = 0;
            if (!battle_is_card_selected(b, next_pos) &&
                b->hand[next_pos].uses_remaining != 0) {
                b->cursor_pos = next_pos;
                break;
            }
        }
    }
}

void battle_set_result(Battle *b, uint8_t res)
{
    uint8_t ev = (res == BATTLE_RESULT_VICTORY) ? EVENT_BATTLE_WON :
                 ((res == BATTLE_RESULT_DEFEAT) ? EVENT_BATTLE_LOST : EVENT_BATTLE_FLED);
    b->result = (BattleResult)res;
    b->phase = BATTLE_PHASE_RESULT;
    b->turn = BATTLE_TURN_RESULT;
    b->battle_over = true;
    b->dirty = BATTLE_DIRTY_ALL;
    telemetry_emit(ev, 0, 0, 0, 0);
}

void battle_card_undo(Battle *b)
{
    if (!b) return;
    if (b->phase != BATTLE_PHASE_PLAYER_SELECT && b->phase != BATTLE_PHASE_PLAYER_DEFEND) {
        return;
    }

    if (b->combo_count > 0) {
        b->cursor_pos = b->selected_indices[--b->combo_count];
        b->dirty |= (BATTLE_DIRTY_COMBO | BATTLE_DIRTY_HAND | BATTLE_DIRTY_DESC);
    } else if (b->phase == BATTLE_PHASE_PLAYER_SELECT) {
        battle_set_result(b, BATTLE_RESULT_FLED);
    }
}

static void battle_resolve_hand_discard(Battle *b)
{
    uint8_t i, idx;
    for (i = 0; i < b->combo_count; i++) {
        idx = b->selected_indices[i];
        if (b->hand[idx].uses_remaining != 0xFF && b->hand[idx].uses_remaining > 0) {
            b->hand[idx].uses_remaining--;
        }
        deck_discard(&b->deck, b->hand[idx]);
        deck_draw(&b->deck, &b->hand[idx]);
    }
    b->combo_count = 0;
}

static uint8_t battle_eval_current_combo(Battle *b)
{
    uint8_t i;
    ComboPhase phase;
    if (b->combo_count == 0) return 0;
    for (i = 0; i < b->combo_count; i++) {
        b->last_combo.cards[i] = b->hand[b->selected_indices[i]];
    }
    phase = (b->phase == BATTLE_PHASE_PLAYER_DEFEND) ? COMBO_PHASE_DEFEND : COMBO_PHASE_ATTACK;
    combo_evaluate(b->last_combo.cards, b->combo_count, phase, &b->last_combo);
    return (uint8_t)b->last_combo.final_power;
}

void battle_execute_combo(Battle *b)
{
    uint8_t power;
    if (!b || b->battle_over) return;

    if (b->phase == BATTLE_PHASE_PLAYER_SELECT) {
        if (b->combo_count == 0) {
            if (b->hand[b->cursor_pos].uses_remaining == 0) {
                uint8_t s;
                for (s = 0; s < BATTLE_HAND_SIZE; s++) {
                    if (b->hand[s].uses_remaining != 0 &&
                        !battle_is_card_selected(b, s)) {
                        b->cursor_pos = s;
                        break;
                    }
                }
                if (b->hand[b->cursor_pos].uses_remaining == 0) {
                    b->phase = BATTLE_PHASE_PLAYER_ANIM;
                    b->delay_timer = 30;
                    b->dirty = BATTLE_DIRTY_ALL;
                    return;
                }
            }
            b->selected_indices[0] = b->cursor_pos;
            b->combo_count = 1;
        }
        power = battle_eval_current_combo(b);
        if (b->last_combo.cards[0].type == BATTLE_CARD_TYPE_HEAL) {
            b->player.hp += power;
            if (b->player.hp > b->player.max_hp) b->player.hp = b->player.max_hp;
            telemetry_emit(EVENT_HEALED, power, 0, 0, 0);
        } else {
            combatant_take_damage(&b->enemies[b->target_idx], power);
            telemetry_emit(EVENT_DAMAGE_DEALT, power, 0, 0, 0);
        }
        battle_resolve_hand_discard(b);
        b->phase = BATTLE_PHASE_PLAYER_ANIM;
        b->delay_timer = 30;
        b->dirty = BATTLE_DIRTY_ALL;
        if (b->enemies[b->target_idx].hp == 0) {
            telemetry_emit(EVENT_ENTITY_DEFEATED, (uint8_t)(b->target_idx + 1), 0, 0, 0);
            battle_target_auto_advance(b);
        }
        if (battle_all_enemies_dead(b)) {
            battle_set_result(b, BATTLE_RESULT_VICTORY);
        }
    } else if (b->phase == BATTLE_PHASE_PLAYER_DEFEND) {
        power = battle_eval_current_combo(b);
        power = (b->enemy_incoming_dmg > power) ? (b->enemy_incoming_dmg - power) : 0;
        combatant_take_damage(&b->player, power);
        telemetry_emit(EVENT_DAMAGE_RECEIVED, power, 0, 0, 0);
        battle_resolve_hand_discard(b);
        b->phase = BATTLE_PHASE_DEFENSE_RESOLVE;
        b->delay_timer = 30;
        b->dirty = BATTLE_DIRTY_ALL;
        if (b->player.hp == 0) {
            telemetry_emit(EVENT_ENTITY_DEFEATED, 0, 0, 0, 0);
            battle_set_result(b, BATTLE_RESULT_DEFEAT);
        }
    }
}

void battle_update(Battle *b)
{
    if (!b || b->battle_over) return;

    if (b->phase == BATTLE_PHASE_PLAYER_SELECT || b->phase == BATTLE_PHASE_PLAYER_DEFEND) {
        if (b->timer_ticks > 0) {
            b->timer_ticks--;
            if (b->phase == BATTLE_PHASE_PLAYER_DEFEND && ((b->timer_ticks & 15) == 0 || (b->timer_ticks & 15) == 15)) {
                b->dirty |= BATTLE_DIRTY_BLINK;
            }
        } else {
            battle_execute_combo(b);
        }
    } else if (b->delay_timer > 0) {
        b->delay_timer--;
    } else {
        if (b->phase == BATTLE_PHASE_PLAYER_ANIM) {
            uint8_t count = 0;
            b->phase = BATTLE_PHASE_ENEMY_TELEGRAPH;
            b->turn = BATTLE_TURN_ENEMY;
            b->enemy_incoming_dmg = battle_calc_enemy_attack(b);
            b->delay_timer = 20;
            while (count < b->enemy_count && b->enemies[b->attacking_enemy_idx].hp == 0) {
                b->attacking_enemy_idx = (uint8_t)((b->attacking_enemy_idx + 1) % b->enemy_count);
                count++;
            }
        } else {
            b->phase = (b->phase == BATTLE_PHASE_ENEMY_TELEGRAPH) ? BATTLE_PHASE_PLAYER_DEFEND : BATTLE_PHASE_PLAYER_SELECT;
            b->turn = BATTLE_TURN_PLAYER;
            b->combo_count = 0;
            b->timer_ticks = BATTLE_TIMER_MAX_FRAMES;
            if (b->phase == BATTLE_PHASE_PLAYER_SELECT && b->enemy_count > 1) {
                b->attacking_enemy_idx = (uint8_t)((b->attacking_enemy_idx + 1) % b->enemy_count);
            }
        }
        b->dirty = BATTLE_DIRTY_ALL;
    }
}

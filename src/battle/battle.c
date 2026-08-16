#include "battle.h"
#include "telemetry.h"

void battle_start(Battle *b, const char *enemy_name, uint8_t player_hp,
                  uint8_t player_max_hp, uint8_t player_attack,
                  uint8_t enemy_hp, uint8_t enemy_max_hp)
{
    uint8_t i;
    if (!b) return;

    combatant_init(&b->player, "Hero", player_hp, player_max_hp);
    combatant_init(&b->enemy, enemy_name ? enemy_name : "Enemy", enemy_hp, enemy_max_hp);
    b->player.attack = player_attack;
    b->enemy.attack = 3;

    deck_init_default(&b->deck);
    for (i = 0; i < BATTLE_HAND_SIZE; i++) {
        deck_draw(&b->deck, &b->hand[i]);
        b->selected_indices[i] = 0;
    }

    b->combo_count = 0;
    b->cursor_pos = 0;
    b->timer_ticks = BATTLE_TIMER_MAX_FRAMES;
    b->timer_max = BATTLE_TIMER_MAX_FRAMES;
    b->phase = BATTLE_PHASE_PLAYER_SELECT;
    b->turn = BATTLE_TURN_PLAYER;
    b->result = BATTLE_RESULT_NONE;
    b->delay_timer = 0;
    b->battle_over = false;
    b->enemy_incoming_dmg = 0;

    telemetry_emit(EVENT_BATTLE_STARTED, 0, 0, 0, 0);
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
    if (!b) return;
    if (dir < 0) {
        b->cursor_pos = (uint8_t)((b->cursor_pos > 0) ? (b->cursor_pos - 1) : (BATTLE_HAND_SIZE - 1));
    } else if (dir > 0) {
        b->cursor_pos = (uint8_t)((b->cursor_pos < BATTLE_HAND_SIZE - 1) ? (b->cursor_pos + 1) : 0);
    }
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

    if (b->combo_count < BATTLE_HAND_SIZE) {
        b->selected_indices[b->combo_count++] = b->cursor_pos;

        for (step = 1; step < BATTLE_HAND_SIZE; step++) {
            next_pos = (uint8_t)((b->cursor_pos + step) % BATTLE_HAND_SIZE);
            if (!battle_is_card_selected(b, next_pos)) {
                b->cursor_pos = next_pos;
                break;
            }
        }
    }
}

void battle_card_undo(Battle *b)
{
    if (!b) return;
    if (b->phase != BATTLE_PHASE_PLAYER_SELECT && b->phase != BATTLE_PHASE_PLAYER_DEFEND) {
        return;
    }

    if (b->combo_count > 0) {
        b->cursor_pos = b->selected_indices[--b->combo_count];
    } else if (b->phase == BATTLE_PHASE_PLAYER_SELECT) {
        b->result = BATTLE_RESULT_FLED;
        b->turn = BATTLE_TURN_RESULT;
        b->battle_over = true;
        telemetry_emit(EVENT_BATTLE_FLED, 0, 0, 0, 0);
    }
}

static void battle_resolve_hand_discard(Battle *b)
{
    uint8_t i, idx;
    for (i = 0; i < b->combo_count; i++) {
        idx = b->selected_indices[i];
        deck_discard(&b->deck, b->hand[idx]);
        deck_draw(&b->deck, &b->hand[idx]);
    }
    b->combo_count = 0;
}

static uint8_t battle_eval_current_combo(Battle *b)
{
    Card selected[BATTLE_HAND_SIZE];
    uint8_t i;
    if (b->combo_count == 0) return 0;
    for (i = 0; i < b->combo_count; i++) {
        selected[i] = b->hand[b->selected_indices[i]];
    }
    combo_evaluate(selected, b->combo_count, &b->last_combo);
    return (uint8_t)b->last_combo.final_power;
}

static void battle_check_death(Battle *b, Combatant *c, BattleResult res, uint8_t entity_idx, uint8_t ev)
{
    if (combatant_is_dead(c)) {
        telemetry_emit(EVENT_ENTITY_DEFEATED, entity_idx, 0, 0, 0);
        b->result = res;
        b->turn = BATTLE_TURN_RESULT;
        b->battle_over = true;
        telemetry_emit(ev, 0, 0, 0, 0);
    }
}

void battle_execute_combo(Battle *b)
{
    uint8_t power;
    if (!b || b->battle_over) return;

    if (b->phase == BATTLE_PHASE_PLAYER_SELECT) {
        if (b->combo_count == 0) {
            b->selected_indices[0] = b->cursor_pos;
            b->combo_count = 1;
        }
        power = battle_eval_current_combo(b);
        if (b->last_combo.cards[0].type == CARD_TYPE_HEAL) {
            b->player.hp = (uint8_t)((b->player.hp + power > b->player.max_hp) ? b->player.max_hp : (b->player.hp + power));
            telemetry_emit(EVENT_HEALED, power, 0, 0, 0);
        } else {
            combatant_take_damage(&b->enemy, power);
            telemetry_emit(EVENT_DAMAGE_DEALT, power, 0, 0, 0);
        }
        battle_resolve_hand_discard(b);
        b->phase = BATTLE_PHASE_PLAYER_ANIM;
        b->delay_timer = 30;
        battle_check_death(b, &b->enemy, BATTLE_RESULT_VICTORY, 1, EVENT_BATTLE_WON);
    } else if (b->phase == BATTLE_PHASE_PLAYER_DEFEND) {
        power = battle_eval_current_combo(b);
        power = (b->enemy_incoming_dmg > power) ? (b->enemy_incoming_dmg - power) : 0;
        combatant_take_damage(&b->player, power);
        telemetry_emit(EVENT_DAMAGE_RECEIVED, power, 0, 0, 0);
        battle_resolve_hand_discard(b);
        b->phase = BATTLE_PHASE_DEFENSE_RESOLVE;
        b->delay_timer = 30;
        battle_check_death(b, &b->player, BATTLE_RESULT_DEFEAT, 0, EVENT_BATTLE_LOST);
    }
}

void battle_update(Battle *b)
{
    if (!b || b->battle_over) return;

    if (b->phase == BATTLE_PHASE_PLAYER_SELECT || b->phase == BATTLE_PHASE_PLAYER_DEFEND) {
        if (b->timer_ticks > 0) {
            b->timer_ticks--;
        } else {
            battle_execute_combo(b);
        }
    } else if (b->delay_timer > 0) {
        b->delay_timer--;
    } else {
        if (b->phase == BATTLE_PHASE_PLAYER_ANIM) {
            b->phase = BATTLE_PHASE_ENEMY_TELEGRAPH;
            b->turn = BATTLE_TURN_ENEMY;
            b->enemy_incoming_dmg = b->enemy.attack ? b->enemy.attack : 3;
            b->delay_timer = 20;
        } else {
            b->phase = (b->phase == BATTLE_PHASE_ENEMY_TELEGRAPH) ? BATTLE_PHASE_PLAYER_DEFEND : BATTLE_PHASE_PLAYER_SELECT;
            b->turn = BATTLE_TURN_PLAYER;
            b->combo_count = 0;
            b->timer_ticks = BATTLE_TIMER_MAX_FRAMES;
        }
    }
}

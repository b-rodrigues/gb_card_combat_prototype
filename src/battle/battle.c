#include "battle.h"
#include "telemetry.h"
#include "banked.h"
#include "rpg/cards.h"
#include "rpg/deck.h"
#include "rpg/effects.h"
#include "rpg/status.h"
#include "rng.h"
#include "game/game_ids.h"
#include <string.h>

/* ── Bridge: persistent DeckState → battle Deck ───────────────────
 * When a DeckState is provided (player has cards), build the battle
 * deck from the player's owned cards.  When NULL, fall back to the
 * hardcoded starter deck (all unlimited uses).
 *
 * The bridge body lives in ROM bank 2 (src/battle/battle_init_content.c)
 * so it can read the registered card catalog directly without consuming
 * the fixed-bank budget; this wrapper only stages its two pointers. */
static void battle_init_from_deck_state(Battle *b, const DeckState *ds)
{
    g_bk_call_bank = 2;
    g_bk_call_target = (uint16_t)&battle_init_deck_banked;
    g_bk_ptr_a = (void *)b;
    g_bk_ptr_b = (void *)ds;
    banked_call_run();
}

/* Compile-time guarantee that the timer window is an exact integer number of
 * bar cells and whole seconds: every '=' drains one cell per
 * BATTLE_TIMER_SECONDS frames with no fractional remainder (which would make
 * tiles drain over uneven frame counts).  Same array-size trap as input.c. */
static const int g_battle_timer_cadence_ok[
    (BATTLE_TIMER_MAX_FRAMES % BATTLE_TIMER_BAR_LENGTH == 0) &&
    (BATTLE_TIMER_BAR_DIVISOR == BATTLE_TIMER_MAX_FRAMES / BATTLE_TIMER_BAR_LENGTH) ? 1 : 0
];

/* Compile-time guarantee that the deck-removal floor equals the dealt hand
 * size: every battle must open with DECK_MIN_CARDS real cards, so a battle
 * hand-size change that forgets rpg/deck.h would silently reintroduce the
 * phantom-card fallback path.  Same array-size trap as above. */
static const int g_deck_min_matches_hand_size[
    (DECK_MIN_CARDS == BATTLE_HAND_SIZE) ? 1 : 0
];

void battle_start(Battle *b, const char *enemy_name, uint8_t player_hp,
                  uint8_t player_max_hp,
                  uint8_t enemy_hp, uint8_t enemy_max_hp,
                  const DeckState *ds, uint8_t battle_id)
{
    uint8_t *p = (uint8_t *)b;
    uint16_t n = sizeof(Battle);
    uint8_t i;
    if (!b) return;

    while (n--) *p++ = 0;
    { const char *s = "Hero"; uint8_t j; for (j = 0; j < 7 && s[j]; j++) b->player.name[j] = s[j]; b->player.name[j] = '\0'; }
    b->player.hp = player_hp;
    b->player.max_hp = player_max_hp;

    { const char *s = enemy_name ? enemy_name : "Enemy"; uint8_t j; for (j = 0; j < 7 && s[j]; j++) b->enemies[0].name[j] = s[j]; b->enemies[0].name[j] = '\0'; }
    b->enemies[0].hp = enemy_hp;
    b->enemies[0].max_hp = enemy_max_hp;
    b->enemy_count = 1;
    b->enemy_battle_id = battle_id;

    if (ds && ds->count > 0) {
        battle_init_from_deck_state(b, ds);
    } else {
        deck_init_default(&b->deck);
    }
    for (i = 0; i < BATTLE_HAND_SIZE; i++) {
        deck_draw(&b->deck, &b->hand[i]);
    }

    if (battle_id != 0) {
        g_bk_byte_a = battle_id;
        g_bk_ptr_a = (void *)&b->enemy_deck;
        enemy_deck_setup();
    }

    b->timer_ticks = BATTLE_TIMER_MAX_FRAMES;
    b->energy = BATTLE_ENERGY_PER_TURN;
    b->phase = BATTLE_PHASE_PLAYER_SELECT;
    b->turn = BATTLE_TURN_PLAYER;
    b->dirty = BATTLE_DIRTY_ALL;

    status_reset_battle();

    telemetry_emit(EVENT_BATTLE_STARTED, 0, 0, 0, 0);
}

void battle_add_enemy(Battle *b, const char *name, uint8_t hp, uint8_t max_hp)
{
    uint8_t idx;
    if (!b || b->enemy_count >= MAX_BATTLE_ENEMIES) return;
    idx = b->enemy_count++;
    { const char *nm = name ? name : "Enemy"; uint8_t j; for (j = 0; j < 7 && nm[j]; j++) b->enemies[idx].name[j] = nm[j]; b->enemies[idx].name[j] = '\0'; }
    b->enemies[idx].hp = hp;
    b->enemies[idx].max_hp = max_hp;
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

/* Total energy cost reserved by the cards currently selected into the combo.
 * Selection validates against the un-reserved remainder, so this never
 * exceeds b->energy. */
static uint8_t combo_reserved_cost(const Battle *b)
{
    uint8_t i, sum = 0;
    for (i = 0; i < b->combo_count; i++) {
        sum += b->hand[b->selected_indices[i]].cost;
    }
    return sum;
}

/* A hand card can be added to the current combo only while it has uses left
 * AND its cost fits in the phase energy not yet reserved by the pending
 * combo (deck.md Phase 10: check affordability at select, pay at resolve). */
static bool hand_card_playable(const Battle *b, uint8_t hand_idx)
{
    uint8_t available;
    if (b->hand[hand_idx].uses_remaining == 0) return false;
    available = b->energy - combo_reserved_cost(b);
    return b->hand[hand_idx].cost <= available;
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
        if (hand_card_playable(b, b->cursor_pos)) break;
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

    if (!hand_card_playable(b, b->cursor_pos)) {
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
                hand_card_playable(b, next_pos)) {
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
    BattleResult prev_result;
    if (!b) return;

    prev_result = b->result;
    g_bk_call_bank = 2;
    g_bk_call_target = (uint16_t)&battle_card_undo_banked;
    g_bk_ptr_a = (void *)b;
    banked_call_run();

    if (prev_result != BATTLE_RESULT_FLED && b->result == BATTLE_RESULT_FLED) {
        telemetry_emit(EVENT_BATTLE_FLED, 0, 0, 0, 0);
    }
}

static void battle_resolve_hand_discard(Battle *b)
{
    uint8_t i, idx;
    uint8_t cost = 0;
    for (i = 0; i < b->combo_count; i++) {
        idx = b->selected_indices[i];
        cost += b->hand[idx].cost;
        if (b->hand[idx].uses_remaining != 0xFF && b->hand[idx].uses_remaining > 0) {
            b->hand[idx].uses_remaining--;
        }
        telemetry_emit(EVENT_CARD_PLAYED, idx, b->hand[idx].type, b->hand[idx].value, b->hand[idx].uses_remaining);
        deck_discard(&b->deck, b->hand[idx]);
        /* Played cards leave an empty slot; the turn-start draw refills the
         * hand (deck.md: no mid-turn instant redraw). */
        b->hand[idx].type = BATTLE_CARD_TYPE_EMPTY;
    }
    /* Pay the combo's energy cost (selection already proved affordability). */
    b->energy = (b->energy >= cost) ? (uint8_t)(b->energy - cost) : 0;
    b->combo_count = 0;
}

/* Refill empty hand slots from the draw pile at a decision-phase start.
 * Returns false when the pile is dry while slots remain open AND the
 * discard pile can feed a reshuffle — the caller then runs the reshuffle
 * turn (reshuffle + re-deal + skip the player's action). */
static bool battle_turn_draw(Battle *b)
{
    uint8_t i, need = 0;
    for (i = 0; i < BATTLE_HAND_SIZE; i++) {
        if (b->hand[i].type == BATTLE_CARD_TYPE_EMPTY) need++;
    }
    if (need == 0) return true;
    if (b->deck.draw_idx >= b->deck.count) {
        return (b->deck.discard_count == 0);
    }
    for (i = 0; i < BATTLE_HAND_SIZE && need > 0; i++) {
        if (b->hand[i].type == BATTLE_CARD_TYPE_EMPTY) {
            deck_draw(&b->deck, &b->hand[i]);
            need--;
        }
    }
    return true;
}

/* Evaluate the pending selection AND resolve its effect in one banked
 * dispatch, announcing both steps.  The evaluator produces hand quality
 * into last_combo; effect resolution scales it into g_effect_last, which
 * is copied to *out for the caller to APPLY (application stays here:
 * docs/combo-system.md §5/§7).
 *
 * attack_phase selects the leading card's own effect; defend play always
 * requests BLOCK_DAMAGE -- the phase defines the action, so a shieldless
 * selection simply resolves to base_power 0 (full damage taken). */
static void battle_play_hand(Battle *b, bool attack_phase, EffectResult *out)
{
    ComboPhase phase = attack_phase ? COMBO_PHASE_ATTACK : COMBO_PHASE_DEFEND;
    uint8_t i, fx;

    for (i = 0; i < b->combo_count; i++) {
        b->last_combo.cards[i] = b->hand[b->selected_indices[i]];
    }
    fx = attack_phase ? b->last_combo.cards[0].effect
                      : (uint8_t)CARD_EFFECT_BLOCK_DAMAGE;
    combo_resolve(b->last_combo.cards, b->combo_count, phase, fx,
                  &b->last_combo);
    telemetry_emit(EVENT_COMBO_RESOLVED,
                   (uint8_t)b->last_combo.multiplier,
                   b->last_combo.eff_count,
                   (uint8_t)((b->last_combo.is_straight ? 1 : 0) |
                             (b->last_combo.all_same_type ? 2 : 0)),
                   (uint8_t)phase);
    out->type = g_effect_last.type;
    out->amount = g_effect_last.amount;
    telemetry_emit(EVENT_EFFECT_RESOLVED, out->amount, out->type,
                   (uint8_t)phase, b->target_idx);
}

void battle_execute_combo(Battle *b)
{
    uint8_t power;
    EffectResult res;
    if (!b || b->battle_over) return;

    if (b->phase == BATTLE_PHASE_PLAYER_SELECT) {
        if (b->combo_count == 0) {
            if (!hand_card_playable(b, b->cursor_pos)) {
                uint8_t s;
                for (s = 0; s < BATTLE_HAND_SIZE; s++) {
                    if (hand_card_playable(b, s) &&
                        !battle_is_card_selected(b, s)) {
                        b->cursor_pos = s;
                        break;
                    }
                }
                if (!hand_card_playable(b, b->cursor_pos)) {
                    b->phase = BATTLE_PHASE_PLAYER_ANIM;
                    b->delay_timer = 30;
                    b->dirty = BATTLE_DIRTY_ALL;
                    return;
                }
            }
            b->selected_indices[0] = b->cursor_pos;
            b->combo_count = 1;
        }
        battle_play_hand(b, true, &res);
        if (res.type == CARD_EFFECT_HEAL_HP) {
            b->player.hp += res.amount;
            if (b->player.hp > b->player.max_hp) b->player.hp = b->player.max_hp;
            telemetry_emit(EVENT_HEALED, res.amount, 0, 0, 0);
        } else {
            /* Damage hand: shield-led fodder deals the non-shield sum,
             * exactly as before -- only HEAL_HP heals. */
            combatant_take_damage(&b->enemies[b->target_idx], res.amount);
            telemetry_emit(EVENT_DAMAGE_DEALT, res.amount, 0, 0, 0);
            /* On-hit status rider (Phase C): the leading card's data
             * decides; the deterministic RNG roll decides landing
             * (docs/combo-system.md §13/§17). */
            if (b->enemies[b->target_idx].hp != 0 &&
                b->last_combo.cards[0].status_id != STATUS_NONE &&
                (uint8_t)rng_next() < b->last_combo.cards[0].status_chance) {
                status_apply(status_slots((uint8_t)(b->target_idx + 1)),
                             b->last_combo.cards[0].status_id, 1, 0);
            }
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
        battle_play_hand(b, false, &res);
        power = (b->enemy_incoming_dmg > res.amount)
                    ? (uint8_t)(b->enemy_incoming_dmg - res.amount) : 0;
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

/* End-of-round status ticks (Phase C): every living combatant's active
 * statuses deal their tick damage, durations decrement, finished
 * instances expire (docs/combo-system.md §15).  Runs once per cycle at
 * the transition back into PLAYER_SELECT; poison deaths resolve like
 * combat deaths.  Returns nothing; result transitions happen here. */
static void battle_tick_statuses(Battle *b)
{
    uint8_t i, dmg;

    for (i = 0; i < b->enemy_count; i++) {
        if (b->enemies[i].hp == 0) continue;
        dmg = status_tick(status_slots((uint8_t)(i + 1)), (uint8_t)(i + 1));
        if (dmg == 0) continue;
        combatant_take_damage(&b->enemies[i], dmg);
        if (b->enemies[i].hp == 0) {
            telemetry_emit(EVENT_ENTITY_DEFEATED, (uint8_t)(i + 1), 0, 0, 0);
        }
    }
    if (b->player.hp != 0) {
        dmg = status_tick(status_slots(0), 0);
        if (dmg != 0) {
            combatant_take_damage(&b->player, dmg);
            if (b->player.hp == 0) {
                telemetry_emit(EVENT_ENTITY_DEFEATED, 0, 0, 0, 0);
            }
        }
    }
    if (battle_all_enemies_dead(b)) {
        battle_set_result(b, BATTLE_RESULT_VICTORY);
    } else if (b->player.hp == 0) {
        battle_set_result(b, BATTLE_RESULT_DEFEAT);
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
        if (b->phase == BATTLE_PHASE_PLAYER_ANIM ||
            b->phase == BATTLE_PHASE_SHUFFLE) {
            uint8_t count = 0;
            b->phase = BATTLE_PHASE_ENEMY_TELEGRAPH;
            b->turn = BATTLE_TURN_ENEMY;
            if (b->enemy_deck.count > 0) {
                b->enemy_played_card = b->enemy_deck.cards[b->enemy_deck.draw_idx];
                b->enemy_incoming_dmg = b->enemy_played_card.value;
                telemetry_emit(EVENT_ENEMY_CARD_PLAYED,
                               b->enemy_deck.draw_idx,
                               b->enemy_played_card.type,
                               b->enemy_played_card.value, 0);
                b->enemy_deck.draw_idx++;
                if (b->enemy_deck.draw_idx >= b->enemy_deck.count) {
                    b->enemy_deck.draw_idx = 0;
                }
            } else {
                b->enemy_incoming_dmg = 3;
            }
            b->delay_timer = 20;
            while (count < b->enemy_count && b->enemies[b->attacking_enemy_idx].hp == 0) {
                b->attacking_enemy_idx = (uint8_t)((b->attacking_enemy_idx + 1) % b->enemy_count);
                count++;
            }
        } else {
            b->phase = (b->phase == BATTLE_PHASE_ENEMY_TELEGRAPH) ? BATTLE_PHASE_PLAYER_DEFEND : BATTLE_PHASE_PLAYER_SELECT;
            b->turn = BATTLE_TURN_PLAYER;
            b->combo_count = 0;
            b->energy = BATTLE_ENERGY_PER_TURN;
            b->timer_ticks = BATTLE_TIMER_MAX_FRAMES;
            if (b->phase == BATTLE_PHASE_PLAYER_SELECT && b->enemy_count > 1) {
                b->attacking_enemy_idx = (uint8_t)((b->attacking_enemy_idx + 1) % b->enemy_count);
            }
            if (b->phase == BATTLE_PHASE_PLAYER_SELECT) {
                battle_tick_statuses(b);
                if (b->battle_over) {
                    b->dirty = BATTLE_DIRTY_ALL;
                    return;
                }
            }
            if (!battle_turn_draw(b)) {
                /* Deck and hand are dry: reshuffling consumes the player's
                 * action for this cycle — re-deal, announce, then the enemy
                 * still attacks (deck.md Phase 10). */
                deck_reshuffle(&b->deck);
                battle_turn_draw(b);
                telemetry_emit(EVENT_DECK_RESHUFFLED,
                               (uint8_t)(b->deck.count - b->deck.draw_idx),
                               b->deck.discard_count, 0, 0);
                b->phase = BATTLE_PHASE_SHUFFLE;
                b->turn = BATTLE_TURN_ENEMY;
                b->delay_timer = 45;
            }
        }
        b->dirty = BATTLE_DIRTY_ALL;
    }
}

#pragma bank 2

#include "combo.h"
#include "rpg/effects.h"
#include "banked.h"

/* ── Banked combo evaluation + effect dispatch ──────────────────────
 * The fixed-bank wrapper (src/battle/combo.c) stages its arguments into
 * the _DATA globals (banked.c) and runs this no-arg body through the WRAM
 * banked-call trampoline (crt0.s).  This file stays in ROM bank 2 so combo
 * evaluation does not consume the fixed-bank budget.  Apart from the
 * bank-local call into effects_content.c below (same ROM bank, direct
 * call -- allowed), it is self-contained: no fixed-bank calls.
 *
 * Architectural rule (docs/combo-system.md §5): the combo evaluator answers
 * "what hand did these cards form" -- shape flags, multiplier, base_power.
 * It must never compute an effect magnitude; scaling base_power into
 * damage/heal/block happens in effect_resolve_into() at the end of this
 * dispatch, whose result battle reads via effect_last(). */

static const uint8_t s_straight_mults[4] = { 120, 150, 175, 200 };

void combo_resolve_banked(void)
{
    const Card *cards = (const Card *)g_bk_ptr_a;
    uint8_t count = g_bk_byte_a;
    uint8_t phase = (uint8_t)(g_bk_byte_b & 0x01);
    uint8_t effect = (uint8_t)(g_bk_byte_b >> 1);
    ComboResult *out_result = (ComboResult *)g_bk_ptr_b;
    uint8_t i, eff_count = 0, prev_val = 0;
    uint8_t sum = 0;
    bool straight = true, same_type = true;
    uint16_t mult = 100;

    if (!out_result) return;

    if (!cards || count == 0) {
        out_result->count = 0;
        out_result->eff_count = 0;
        out_result->is_straight = false;
        out_result->all_same_type = false;
        out_result->multiplier = 100;
        out_result->base_power = 0;
        g_effect_last.type = effect;
        g_effect_last.amount = 0;
        return;
    }

    if (count > 5) count = 5;
    out_result->count = count;

    for (i = 0; i < count; i++) {
        uint8_t t = cards[i].type;
        uint8_t v = cards[i].value;

        if (phase == COMBO_PHASE_ATTACK) {
            if (t != BATTLE_CARD_TYPE_SHIELD) sum += v;
            if (i > 0) {
                if (v != (uint8_t)(prev_val + 1)) straight = false;
                if (t != cards[0].type) same_type = false;
            }
            prev_val = v;
            eff_count++;
        } else if (t == BATTLE_CARD_TYPE_SHIELD) {
            sum += v;
            if (eff_count > 0 && v != (uint8_t)(prev_val + 1)) straight = false;
            prev_val = v;
            eff_count++;
        }
    }

    if (eff_count < 2) {
        straight = false;
        same_type = false;
    }

    if (straight) {
        mult = (uint16_t)(s_straight_mults[eff_count - 2] + (same_type ? 25 : 0));
    } else if (same_type) {
        mult = 110;
    }

    out_result->is_straight = straight;
    out_result->all_same_type = same_type;
    out_result->multiplier = mult;
    out_result->base_power = sum;
    out_result->eff_count = eff_count;

    /* Effect resolution consumes the evaluated hand (bank-local call). */
    effect_resolve_into(effect, out_result, &g_effect_last);
}

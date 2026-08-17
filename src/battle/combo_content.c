#pragma bank 2

#include "combo.h"
#include "banked.h"

/* ── Banked combo evaluation ────────────────────────────────────────
 * The fixed-bank wrapper (src/battle/combo.c) stages its arguments into
 * the _DATA globals (banked.c) and runs this no-arg body through the WRAM
 * banked-call trampoline (crt0.s / src/core/banked.c).  This file stays in
 * ROM bank 2 so combo_evaluate's logic does not consume the fixed-bank
 * budget.  It must be self-contained (no calls back into fixed-bank code);
 * it only reads the staged globals, its own banked const data, and writes
 * through the staged out_result pointer. */

static const uint8_t s_straight_mults[4] = { 120, 150, 175, 200 };

void combo_evaluate_banked(void)
{
    const Card *cards = (const Card *)g_bk_ptr_a;
    uint8_t count = g_bk_byte_a;
    ComboPhase phase = (ComboPhase)g_bk_byte_b;
    ComboResult *out_result = (ComboResult *)g_bk_ptr_b;
    uint8_t i, eff_count = 0, prev_val = 0;
    uint8_t sum = 0, power;
    bool straight = true, same_type = true;
    uint16_t mult = 100;

    if (!out_result) return;

    if (!cards || count == 0) {
        out_result->count = 0;
        out_result->is_straight = false;
        out_result->all_same_type = false;
        out_result->multiplier = 100;
        out_result->base_power = 0;
        out_result->final_power = 0;
        return;
    }

    if (count > 5) count = 5;
    out_result->count = count;

    for (i = 0; i < count; i++) {
        uint8_t t = cards[i].type;
        uint8_t v = cards[i].value;

        if (phase == COMBO_PHASE_ATTACK) {
            if (t != CARD_TYPE_SHIELD) sum += v;
            if (i > 0) {
                if (v != (uint8_t)(prev_val + 1)) straight = false;
                if (t != cards[0].type) same_type = false;
            }
            prev_val = v;
            eff_count++;
        } else if (t == CARD_TYPE_SHIELD) {
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

    power = sum;
    if (straight) {
        uint8_t add = (eff_count == 5) ? sum :
                      (eff_count == 4) ? (uint8_t)((sum >> 1) + (sum >> 2)) :
                      (eff_count == 3) ? (uint8_t)(sum >> 1) :
                      (uint8_t)(sum >> 2);
        power = (uint8_t)(power + add + (same_type ? (uint8_t)(sum >> 2) : 0));
    } else if (same_type) {
        power = (uint8_t)(power + (uint8_t)(sum >> 3));
    }
    out_result->final_power = power;
}
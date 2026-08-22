#pragma bank 2

#include "effects.h"
#include "banked.h"

/* ── Banked effect resolution (docs/combo-system.md Phase B) ─────────
 * Runs inside the combo_resolve() dispatch (ROM bank 2): the combo body
 * calls effect_resolve_into() bank-locally after evaluating hand shape.
 * Self-contained: pure function of its arguments; writes the shared
 * g_effect_last slot (WRAM) and/or the caller's out pointer; never calls
 * fixed-bank code.
 *
 * The combo→power scaling below is the former final_power block of
 * combo_evaluate, ported verbatim so battle numbers stay bit-identical.
 * Every effect currently scales the same way; when per-effect scaling
 * arrives (plan §10 EffectScaling), each case here grows its own
 * response to combo quality. */

/* Scale a hand's base_power by its hand-shape bonuses.
 * straight: +tiered bonus by effective card count (+25% of sum if suited)
 * same-type (unsuited): +12.5% of sum */
static uint8_t effect_scale_power(const ComboResult *combo)
{
    uint8_t sum = (uint8_t)combo->base_power;
    uint8_t amount = sum;

    if (combo->is_straight) {
        uint8_t add = (combo->eff_count == 5) ? sum :
                      (combo->eff_count == 4) ? (uint8_t)((sum >> 1) + (sum >> 2)) :
                      (combo->eff_count == 3) ? (uint8_t)(sum >> 1) :
                      (uint8_t)(sum >> 2);
        amount = (uint8_t)(amount + add +
                           (combo->all_same_type ? (uint8_t)(sum >> 2) : 0));
    } else if (combo->all_same_type) {
        amount = (uint8_t)(amount + (uint8_t)(sum >> 3));
    }
    return amount;
}

void effect_resolve_into(uint8_t effect_type, const ComboResult *combo,
                         EffectResult *out)
{
    EffectResult *dst = out ? out : &g_effect_last;

    dst->type = effect_type;

    switch (effect_type) {
        case CARD_EFFECT_DAMAGE_TARGET:
        case CARD_EFFECT_BLOCK_DAMAGE:
        case CARD_EFFECT_HEAL_HP:
            dst->amount = combo ? effect_scale_power(combo) : 0;
            break;
        default:
            dst->amount = 0;
            break;
    }
}

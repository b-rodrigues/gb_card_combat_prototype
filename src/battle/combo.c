#include "combo.h"

void combo_evaluate(const Card *cards, uint8_t count, ComboResult *out_result)
{
    uint8_t i;
    uint16_t sum = 0;
    bool straight = true;
    bool same_type = true;
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
        out_result->cards[i] = cards[i];
        sum += cards[i].value;
        if (i > 0) {
            if (cards[i].value != (cards[i - 1].value + 1)) {
                straight = false;
            }
            if (cards[i].type != cards[0].type) {
                same_type = false;
            }
        }
    }

    if (count >= 2 && straight) {
        static const uint8_t s_straight_mults[4] = { 120, 150, 175, 200 };
        mult = s_straight_mults[count - 2];
        if (same_type) mult += 25;
    } else if (count >= 2 && same_type) {
        mult = 110;
    }

    out_result->is_straight = (count >= 2 && straight);
    out_result->all_same_type = (count >= 2 && same_type);
    out_result->multiplier = mult;
    out_result->base_power = sum;
    {
        uint8_t power = (uint8_t)sum;
        if (count >= 2 && straight) {
            uint8_t add = (count == 5) ? (uint8_t)sum :
                          (count == 4) ? (uint8_t)((sum >> 1) + (sum >> 2)) :
                          (count == 3) ? (uint8_t)(sum >> 1) :
                          (uint8_t)(sum >> 2);
            /* 2-card straight bonus is sum >> 2 (i.e. /4).  This is an
             * intentional rebalance vs the previous exact /5, changed to
             * avoid the SDCC runtime division helper (Bank 0 budget). */
            power += add;
            if (same_type) power += (uint8_t)(sum >> 2);
        } else if (count >= 2 && same_type) {
            /* Flush-only bonus is sum >> 3 (i.e. /8).  Intentional rebalance
             * vs the previous exact /10 to drop the division helper. */
            power += (uint8_t)(sum >> 3);
        }
        out_result->final_power = power;
    }
}

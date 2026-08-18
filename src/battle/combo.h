#ifndef COMBO_H
#define COMBO_H

#include "card.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    COMBO_PHASE_ATTACK = 0,
    COMBO_PHASE_DEFEND = 1
} ComboPhase;

typedef struct {
    Card cards[5];
    uint8_t count;
    bool is_straight;
    bool all_same_type;
    uint16_t multiplier; /* Percentage, e.g. 100, 150, 175, 200 */
    uint16_t base_power;
    uint16_t final_power;
} ComboResult;

/* Evaluate a sequence of up to 5 cards for the specified phase.
 * Note: To conserve Bank 0 ROM budget, combo_evaluate does not copy cards into
 * out_result->cards[]; the caller is expected to pre-populate or alias
 * out_result->cards[] if needed (as in battle_eval_current_combo). */
void combo_evaluate(const Card *cards, uint8_t count, ComboPhase phase, ComboResult *out_result);

/* The banked no-arg body (src/battle/combo_content.c, ROM bank 2), run via
 * the WRAM banked-call trampoline.  Read only the staged _DATA globals and
 * writes through the staged out_result pointer; it must stay self-contained
 * so it never calls fixed-bank code. */
void combo_evaluate_banked(void);

#endif /* COMBO_H */

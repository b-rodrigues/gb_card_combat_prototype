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
    uint8_t eff_count;  /* Cards that actually entered the hand (attack: every
                         * card; defend: SHIELD cards only).  The straight/
                         * suited bonus tiers key off this, not count. */
    bool is_straight;
    bool all_same_type;
    uint16_t multiplier; /* Percentage, e.g. 100, 150, 175, 200 */
    uint16_t base_power;
} ComboResult;

/* Evaluate a sequence of up to 5 cards for the specified phase AND resolve
 * the requested effect against the evaluated hand in one banked dispatch.
 *
 * The evaluator produces ONLY the quality of the selection (hand shape +
 * multiplier + base_power) into out_result -- never an effect magnitude.
 * The resolver (src/rpg/effects_content.c, same bank) then scales that
 * quality into g_effect_last per effect_type; battle reads it via
 * effect_last() and applies it (docs/combo-system.md §5/§7).
 *
 * effect_type: CardEffectType the played hand performs.  Attack play uses
 * the leading card's Card.effect; defend play requests BLOCK_DAMAGE (the
 * phase defines the action).
 *
 * Note: To conserve Bank 0 ROM budget, the dispatch does not copy cards
 * into out_result->cards[]; the caller pre-populates or aliases
 * out_result->cards[] if needed (as in battle_play_hand). */
void combo_resolve(const Card *cards, uint8_t count, ComboPhase phase,
                   uint8_t effect_type, ComboResult *out_result);

/* The banked no-arg body (src/battle/combo_content.c, ROM bank 2), run via
 * the WRAM banked-call trampoline.  Read only the staged _DATA globals;
 * self-contained apart from the bank-local call into effects_content.c. */
void combo_resolve_banked(void);

#endif /* COMBO_H */

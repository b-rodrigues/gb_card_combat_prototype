#ifndef COMBO_H
#define COMBO_H

#include "card.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    Card cards[5];
    uint8_t count;
    bool is_straight;
    bool all_same_type;
    uint16_t multiplier; /* Percentage, e.g. 100, 150, 175, 200 */
    uint16_t base_power;
    uint16_t final_power;
} ComboResult;

/* Evaluate a sequence of up to 5 cards */
void combo_evaluate(const Card *cards, uint8_t count, ComboResult *out_result);

#endif /* COMBO_H */

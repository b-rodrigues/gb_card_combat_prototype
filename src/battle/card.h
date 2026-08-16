#ifndef CARD_H
#define CARD_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    CARD_TYPE_SWORD = 0,  /* SW: Physical attack */
    CARD_TYPE_SHIELD = 1, /* SH: Defense / block */
    CARD_TYPE_BOW = 2,    /* BO: Ranged physical attack */
    CARD_TYPE_FIRE = 3,   /* FI: Elemental fire attack */
    CARD_TYPE_HEAL = 4    /* HE: Restore HP */
} CardType;

#define CARD_TYPE_COUNT 5

typedef struct {
    uint8_t type;   /* CardType */
    uint8_t value;  /* Number 1 - 9 */
} Card;

/* Get one-line short description for card */
const char *card_get_description(Card card);

/* Two-letter abbreviation for CardType (e.g. "SW", "SH", "BO", "FI", "HE") */
const char *card_type_code(uint8_t type);

#endif /* CARD_H */

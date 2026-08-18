#ifndef CARD_H
#define CARD_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    BATTLE_CARD_TYPE_SWORD = 0,  /* SW: Physical attack */
    BATTLE_CARD_TYPE_SHIELD = 1, /* SH: Defense / block */
    BATTLE_CARD_TYPE_BOW = 2,    /* BO: Ranged physical attack */
    BATTLE_CARD_TYPE_FIRE = 3,   /* FI: Elemental fire attack */
    BATTLE_CARD_TYPE_HEAL = 4    /* HE: Restore HP */
} BattleCardType;

#define BATTLE_CARD_TYPE_COUNT 5

typedef struct {
    uint8_t type;   /* BattleCardType */
    uint8_t value;  /* Number 1 - 9 */
} Card;

/* Get one-line short description for card type */
const char *card_get_description(uint8_t type);

/* Two-letter abbreviation for CardType (e.g. "SW", "SH", "BO", "FI", "HE") */
const char *card_type_code(uint8_t type);

#endif /* CARD_H */

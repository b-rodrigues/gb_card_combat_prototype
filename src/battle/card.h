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

/* Sentinel for an empty hand slot (played cards stay empty until the turn-
 * start draw refills them).  Must not collide with any BattleCardType; kept
 * as a #define because SDCC enums are signed 8-bit (0x80+ would wrap). */
#define BATTLE_CARD_TYPE_EMPTY 0xFF

typedef struct {
    uint8_t type;           /* BattleCardType */
    uint8_t value;          /* Number 1 - 9 */
    uint8_t uses_remaining; /* 0 = depleted; 0xFF = unlimited */
    uint8_t cost;           /* Energy cost to play (paid from Battle.energy) */
    /* What playing this card DOES (CardEffectType, rpg/cards.h).  Copied
     * from CardDefinition.effect when the battle deck is built from the
     * persistent collection; the definition owns the effect, combat only
     * consumes it (docs/combo-system.md §3-4). */
    uint8_t effect;
} Card;

/* Get one-line short description for card type */
const char *card_get_description(uint8_t type);

/* Packed description table geometry shared by card.c (fixed wrapper) and
 * card_content.c (bank-2 blob).  Stride = longest text + NUL, padded. */
#define CARD_DESC_STRIDE 18
#define CARD_DESC_TYPES  5

#endif /* CARD_H */

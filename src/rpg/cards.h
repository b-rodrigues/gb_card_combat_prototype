#ifndef RPG_CARDS_H
#define RPG_CARDS_H

#include <stdint.h>
#include <stdbool.h>

/* Card identity.  The engine defines CARD_NONE and the start of the
 * per-game content range; the game names its cards in game_ids.h. */
typedef uint8_t CardId;

#define CARD_NONE       0
#define CARD_FIRST_GAME 0x40

/* Broad card categories. */
typedef enum {
    CARD_TYPE_ATTACK  = 0,
    CARD_TYPE_DEFENSE = 1,
    CARD_TYPE_HEAL    = 2,
    CARD_TYPE_STATUS  = 3,
    CARD_TYPE_UTILITY = 4
} CardType;

#define CARD_TYPE_COUNT 5

/* What happens when a card is played. */
typedef enum {
    CARD_EFFECT_DAMAGE_TARGET = 0,
    CARD_EFFECT_BLOCK_DAMAGE   = 1,
    CARD_EFFECT_HEAL_HP        = 2,
    CARD_EFFECT_STATUS_BUFF    = 3,
    CARD_EFFECT_DRAW_CARDS     = 4
} CardEffectType;

/* Static card definition.  Registered at boot via card_register_defs().
 * name is a short display string (4-8 chars). */
typedef struct {
    CardId id;
    uint8_t type;       /* CardType */
    uint8_t power;      /* numerical value */
    uint8_t cost;       /* energy cost to play (0 = free) */
    uint8_t uses_per_battle;
    uint8_t max_copies;   /* 0 = unlimited in collection & deck */
    uint8_t effect;     /* CardEffectType */
    uint8_t battle_type; /* BattleCardType for combat deck mapping */
    uint8_t price;      /* shop price in gold (0 = not sold) */
    const char *name;   /* 4-8 char display name */
} CardDefinition;

/* Register the game's card catalog (content, banked ROM).
 * mechanics below are generic over whatever catalog is registered. */
void card_register_defs(const CardDefinition *defs, uint8_t count,
                        uint8_t bank);

/* Look up a card definition by id.  Returns a pointer to a scratch copy
 * (valid until the next call).  Returns NULL for unknown ids. */
const CardDefinition *card_get_def(CardId id);

#endif /* RPG_CARDS_H */

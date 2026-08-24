#ifndef GAME_SHOPS_H
#define GAME_SHOPS_H

#include <stdint.h>
#include "rpg/cards.h"

#define SHOP_MAX_ITEMS 4

/* A per-shop stock list (game content).  id matches the WorldActorDefinition
 * shop_id of the actor that runs the shop.  Prices are read from the
 * CardDefinition.price field — single source of truth.  `buys` marks a
 * CARD MERCHANT (docs/loot.md §34.6): the SELL mode lists the player's
 * owned loot cards at their centralized sell value. */
typedef struct {
    uint8_t id;
    uint8_t count;
    uint8_t buys;
    CardId items[SHOP_MAX_ITEMS];
} ShopDefinition;

/* Look up a shop's stock by its id, or NULL if unknown. */
const ShopDefinition *game_shop_for_id(uint8_t id);

#endif /* GAME_SHOPS_H */

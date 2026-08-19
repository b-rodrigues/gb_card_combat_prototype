#ifndef GAME_SHOPS_H
#define GAME_SHOPS_H

#include <stdint.h>
#include "rpg/cards.h"

#define SHOP_MAX_ITEMS 4

/* A per-shop stock list (game content).  id matches the WorldActorDefinition
 * shop_id of the actor that runs the shop. */
typedef struct {
    uint8_t id;
    uint8_t count;
    CardId items[SHOP_MAX_ITEMS];
    uint8_t prices[SHOP_MAX_ITEMS];
} ShopDefinition;

/* Look up a shop's stock by its id, or NULL if unknown. */
const ShopDefinition *game_shop_for_id(uint8_t id);

#endif /* GAME_SHOPS_H */

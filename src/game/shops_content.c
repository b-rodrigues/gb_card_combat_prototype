#pragma bank 2

#include "shops.h"
#include "game_ids.h"

/* ── Shop stock lists (game content, banked ROM) ───────────────────
 * buys=1 marks a card merchant: SELECT toggles SELL mode (§34.6). */
const ShopDefinition g_shops[] = {
    { 1, 1, 0, { CARD_WOOD_RING } },
    { 2, 2, 1, { CARD_IRON_SWORD, CARD_BOW_10 } }
};

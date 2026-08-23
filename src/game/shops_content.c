#pragma bank 2

#include "shops.h"
#include "game_ids.h"

/* ── Shop stock lists (game content, banked ROM) ─────────────────── */
const ShopDefinition g_shops[] = {
    { 1, 1, { CARD_HEALING_HERB } },
    { 2, 2, { CARD_IRON_SWORD, CARD_BOW_10 } }
};

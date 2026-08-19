#pragma bank 2

#include "shops.h"
#include "game_ids.h"

/* ── Shop stock lists (game content, banked ROM) ─────────────────── */
const ShopDefinition g_shops[] = {
    { 1, 1, { CARD_HEALING_HERB }, { 20 } },
    { 2, 1, { CARD_IRON_SWORD },   { 10 } }
};

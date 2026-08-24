#pragma bank 2

#include "content.h"
#include "game_ids.h"
#include "banked.h"

/* ── Enemy loot profiles (docs/loot.md §17), bank-2 body ────────────
 * Copies the selected family's archetype pool into the caller's WRAM
 * buffer (fixed code must never keep bank-2 pointers).  Self-contained:
 * reads only its own bank-local tables; reports count through the shared
 * WRAM byte (g_loot_pool_len, defined in content.c). */

void game_loot_pool_banked(void)
{
    static const CardId s_loot_slime[] = {
        CARD_IRON_SWORD, CARD_WOODEN_SHIELD, CARD_HEALING_HERB,
        CARD_POISON_DAGGER
    };
    static const CardId s_loot_bat[] = { CARD_FIRE_TOME };

    uint8_t battle_type = g_bk_byte_a;
    CardId *out = (CardId *)g_bk_ptr_a;
    uint8_t n = 0;

    if (battle_type == BATTLE_BAT) {
        out[n++] = CARD_FIRE_TOME;
    } else {
        out[n++] = CARD_IRON_SWORD;
        out[n++] = CARD_WOODEN_SHIELD;
        out[n++] = CARD_POISON_DAGGER;
        out[n++] = CARD_HEALING_HERB;
    }
    g_loot_pool_len = n;
}

#include "rpg/loot.h"
#include "telemetry.h"

void loot_state_init(LootCollectionState *lc)
{
    if (!lc) return;
    lc->count = 0;
}

bool loot_collection_add(LootCollectionState *lc, CardId archetype,
                         uint8_t rarity)
{
    LootCardInstance *slot;
    if (!lc || lc->count >= LOOT_MAX_INSTANCES) return false;
    slot = &lc->cards[lc->count++];
    slot->archetype = archetype;
    slot->rarity = rarity;
    slot->affixes = 0;   /* affixes arrive in Phase 3 */
    slot->power = 0;     /* 0 = archetype default */
    slot->cost = 0;
    slot->sell_value = 0;/* selling phase */
    telemetry_emit(EVENT_LOOT_CARD_ADDED, archetype, lc->count, rarity, 0);
    return true;
}

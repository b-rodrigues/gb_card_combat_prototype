#include "rpg/loot.h"
#include "telemetry.h"
#include "rng.h"

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

/* ── Combat drop roll (docs/loot.md §8/§18, Phase 2) ────────────────
 * Picks an archetype from the caller-supplied pool, rolls a rarity tier
 * from the placeholder weights (50/30/15/5), and appends the instance.
 * Deterministic: consumes the shared game RNG (docs/loot.md §26).
 *
 * Emits LOOT_GENERATED (d0=archetype d1=rarity d2=tier power bonus) and,
 * on success, loot_collection_add's LOOT_CARD_ADDED.  Returns false when
 * the pool is empty or the collection is full (no rng consumed). */
bool loot_roll_combat(LootCollectionState *lc, const CardId *pool, uint8_t len)
{
    uint8_t roll, tier, pick;

    if (!lc || !pool || len == 0) return false;
    if (lc->count >= LOOT_MAX_INSTANCES) return false;

    pick = pool[rng_next() % len];
    roll = rng_next() % 100;
    if      (roll < 50) tier = 0;   /* COMMON */
    else if (roll < 80) tier = 1;   /* UNCOMMON */
    else if (roll < 95) tier = 2;   /* RARE */
    else                tier = 3;   /* EPIC */

    telemetry_emit(EVENT_LOOT_GENERATED, pick, tier, tier, 0);
    return loot_collection_add(lc, pick, tier);
}

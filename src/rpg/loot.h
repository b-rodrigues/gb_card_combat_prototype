#ifndef RPG_LOOT_H
#define RPG_LOOT_H

#include <stdint.h>
#include <stdbool.h>
#include "rpg/cards.h"

/* ── Procedural loot: card instances (docs/loot.md §20, §34) ────────
 * The player owns card INSTANCES; the deck will reference instances.
 * Increment 1 is storage-only: no generator exists yet, the collection
 * stays empty in real play, and nothing here touches battle code. */

#define LOOT_MAX_INSTANCES 12

/* Rarity tiers (docs/loot.md §6).  LEGENDARY is reserved for a later
 * phase -- it carries special rules, not bigger numbers. */
enum {
    RARITY_COMMON = 0,
    RARITY_UNCOMMON,
    RARITY_RARE,
    RARITY_EPIC,
    RARITY_LEGENDARY
};

typedef struct {
    CardId   archetype;   /* catalog card this instance is based on */
    uint8_t  rarity;      /* Rarity enum; COMMON only in increment 1 */
    uint8_t  affixes;     /* packed prefix/suffix ids; 0 = none (§28) */
    uint8_t  power;       /* 0 = archetype default (later-phase override) */
    uint8_t  cost;        /* 0 = archetype default */
    uint16_t sell_value;  /* reserved for the selling phase; ECUs */
} LootCardInstance;

typedef struct {
    LootCardInstance cards[LOOT_MAX_INSTANCES];
    uint8_t count;
} LootCollectionState;

void loot_state_init(LootCollectionState *lc);

/* Append an instance.  Returns false (and changes nothing) when full.
 * Emits EVENT_LOOT_CARD_ADDED on success (d0 = archetype,
 * d1 = count after). */
bool loot_collection_add(LootCollectionState *lc, CardId archetype,
                         uint8_t rarity);

/* Phase 2 drop roll: archetype picked from pool, rarity from placeholder
 * weights; appends to the collection.  Consumes the shared game RNG. */
bool loot_roll_combat(LootCollectionState *lc, const CardId *pool,
                      uint8_t len);

#endif /* RPG_LOOT_H */

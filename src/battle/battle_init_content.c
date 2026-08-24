#pragma bank 2

#include "battle.h"
#include "rpg/cards.h"
#include "rpg/loot.h"
#include "rpg/status.h"
#include "banked.h"

/* Banked body of battle_init_from_deck_state() (see src/battle/battle.c).
 * Lives in ROM bank 2 so the deck-bridge loop does not consume the
 * fixed-bank budget, and reads the registered card catalog DIRECTLY
 * (g_card_defs, same bank) instead of per-card banked_copy staging.
 * Self-contained: reads only the staged Battle/DeckState pointers and its
 * own bank-local data; writes through the staged pointer. */

void battle_init_deck_banked(void)
{
    Battle *b = (Battle *)g_bk_ptr_a;
    const DeckState *ds = (const DeckState *)g_bk_ptr_b;
    uint8_t i, j;

    if (!b || !ds) return;
    b->deck.count = ds->count;
    b->deck.draw_idx = 0;
    b->deck.discard_count = 0;
    for (i = 0; i < ds->count; i++) {
        const CardDefinition *def = (const CardDefinition *)0;
        for (j = 0; j < g_card_defs_count; j++) {
            if (g_card_defs[j].id == ds->cards[i]) {
                def = &g_card_defs[j];
                break;
            }
        }
        if (def) {
            b->deck.cards[i].type = def->battle_type;
            b->deck.cards[i].value = def->power;
            b->deck.cards[i].uses_remaining =
                (def->uses_per_battle == 0) ? 0xFF : def->uses_per_battle;
            b->deck.cards[i].cost = def->cost;
            b->deck.cards[i].effect = def->effect;
            b->deck.cards[i].status_id = def->status_id;
            b->deck.cards[i].status_chance = def->status_chance;
            /* Loot RING marker (docs/loot.md §34.3): macros only, no
             * fixed-bank calls. */
            b->deck.cards[i].ring =
                (loot_is_loot_id(def->id) &&
                 loot_id_weapon(def->id) == WPN_RING) ? 1 : 0;
        } else {
            /* Unknown id: safety-net sword, mirroring the phantom draw. */
            b->deck.cards[i].type = BATTLE_CARD_TYPE_SWORD;
            b->deck.cards[i].value = 2;
            b->deck.cards[i].uses_remaining = 0xFF;
            b->deck.cards[i].cost = 1;
            b->deck.cards[i].effect = CARD_EFFECT_DAMAGE_TARGET;
            b->deck.cards[i].status_id = STATUS_NONE;
            b->deck.cards[i].status_chance = 0;
            b->deck.cards[i].ring = 0;
        }
    }
}

#include "rpg/deck.h"
#include "rpg/cards.h"
#include <stddef.h>

/* ── Collection internals ───────────────────────────────────────── */

static CardCollectionEntry *collection_find(CardCollectionState *col,
                                            CardId id)
{
    uint8_t i;
    for (i = 0; i < col->count; i++) {
        if (col->entries[i].id == id) {
            return &col->entries[i];
        }
    }
    return NULL;
}

/* ── Collection API ─────────────────────────────────────────────── */

bool deck_collection_add(CardState *cs, CardId id, uint8_t quantity)
{
    CardCollectionEntry *e;
    if (!cs || id == CARD_NONE || quantity == 0) return false;

    e = collection_find(&cs->collection, id);
    if (e) {
        e->count = (uint8_t)(e->count + quantity);
        return true;
    }

    if (cs->collection.count >= MAX_CARD_COLLECTION) return false;
    e = &cs->collection.entries[cs->collection.count];
    e->id = id;
    e->count = quantity;
    cs->collection.count++;
    return true;
}

bool deck_collection_remove(CardState *cs, CardId id, uint8_t quantity)
{
    CardCollectionEntry *e;
    if (!cs || id == CARD_NONE || quantity == 0) return false;

    e = collection_find(&cs->collection, id);
    if (!e || e->count < quantity) return false;

    if (e->count == quantity) {
        *e = cs->collection.entries[--cs->collection.count];
    } else {
        e->count = (uint8_t)(e->count - quantity);
    }
    return true;
}

uint8_t deck_collection_count(const CardState *cs, CardId id)
{
    CardCollectionEntry *e;
    if (!cs) return 0;
    e = collection_find((CardCollectionState *)&cs->collection, id);
    return e ? e->count : 0;
}

bool deck_collection_is_owned(const CardState *cs, CardId id)
{
    return deck_collection_count(cs, id) > 0;
}

/* ── Deck internals ─────────────────────────────────────────────── */

uint8_t deck_count_in_deck(const DeckState *d, CardId id)
{
    uint8_t i, n = 0;
    for (i = 0; i < d->count; i++) {
        if (d->cards[i] == id) n++;
    }
    return n;
}

/* ── Deck API ───────────────────────────────────────────────────── */

bool deck_add_card(CardState *cs, CardId id)
{
    const CardDefinition *def;
    uint8_t in_deck;
    if (!cs || id == CARD_NONE) return false;
    if (cs->deck.count >= MAX_DECK_CARDS) return false;
    if (!deck_collection_is_owned(cs, id)) return false;

    def = card_get_def(id);
    if (!def) return false;
    in_deck = deck_count_in_deck(&cs->deck, id);
    if (in_deck >= def->max_copies) return false;

    cs->deck.cards[cs->deck.count] = id;
    cs->deck.count++;
    return true;
}

bool deck_remove_card(CardState *cs, CardId id)
{
    uint8_t i;
    if (!cs || id == CARD_NONE) return false;

    for (i = 0; i < cs->deck.count; i++) {
        if (cs->deck.cards[i] == id) {
            cs->deck.cards[i] = cs->deck.cards[--cs->deck.count];
            return true;
        }
    }
    return false;
}

uint8_t deck_count(const CardState *cs)
{
    if (!cs) return 0;
    return cs->deck.count;
}

bool deck_validate(const CardState *cs)
{
    const CardDefinition *def;
    uint8_t i, owned, in_deck;
    CardId id;
    if (!cs) return false;

    for (i = 0; i < cs->deck.count; i++) {
        id = cs->deck.cards[i];
        owned = deck_collection_count(cs, id);
        if (owned == 0) return false;
        def = card_get_def(id);
        if (!def) return false;
        in_deck = deck_count_in_deck(&cs->deck, id);
        if (in_deck > def->max_copies) return false;
    }
    return true;
}

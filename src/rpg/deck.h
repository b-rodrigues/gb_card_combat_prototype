#ifndef RPG_DECK_H
#define RPG_DECK_H

#include <stdint.h>
#include <stdbool.h>
#include "rpg/cards.h"

/* Deck and collection sizes.
 * MAX_DECK_CARDS = 20 matches the target deck size from deck.md.
 * GameState is ~202 bytes with these limits, well within the 252-byte
 * save slot limit. */
#define MAX_CARD_COLLECTION 12
#define MAX_DECK_CARDS      20

/* A single owned card in the collection. */
typedef struct {
    CardId id;
    uint8_t count;      /* copies owned */
} CardCollectionEntry;

/* The player's permanent card collection. */
typedef struct {
    CardCollectionEntry entries[MAX_CARD_COLLECTION];
    uint8_t count;
} CardCollectionState;

/* The player's current battle deck (subset of the collection). */
typedef struct {
    CardId cards[MAX_DECK_CARDS];
    uint8_t count;
} DeckState;

/* Combined card state, replacing InventoryState + EquipmentState. */
typedef struct {
    CardCollectionState collection;
    DeckState deck;
} CardState;

/* ── Collection API ─────────────────────────────────────────────── */

/* Add cards to the collection.  Stacks with existing entries.
 * Returns false if the collection is full. */
bool deck_collection_add(CardState *cs, CardId id, uint8_t quantity);

/* Remove cards from the collection.  Returns false if the player
 * does not own enough copies. */
bool deck_collection_remove(CardState *cs, CardId id, uint8_t quantity);

/* How many copies of a card the player owns. */
uint8_t deck_collection_count(const CardState *cs, CardId id);

/* ── Deck API ───────────────────────────────────────────────────── */

/* Add a card to the deck.  The card must be owned in the collection.
 * Returns false if the deck is full or the card is not owned. */
bool deck_add_card(CardState *cs, CardId id);

/* Remove a card from the deck.  Returns false if the card is not
 * in the deck. */
bool deck_remove_card(CardState *cs, CardId id);

/* How many copies of a specific card are in the deck. */
uint8_t deck_count_in_deck(const DeckState *d, CardId id);

#endif /* RPG_DECK_H */

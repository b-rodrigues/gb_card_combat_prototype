#ifndef DECK_H
#define DECK_H

#include "card.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_DECK_SIZE 20

typedef struct {
    Card cards[MAX_DECK_SIZE];
    uint8_t count;
    uint8_t draw_idx;
    Card discard[MAX_DECK_SIZE];
    uint8_t discard_count;
} Deck;

/* Initialize standard starter deck (deterministic fixed order) */
void deck_init_default(Deck *d);

/* Draw next card from deck, reshuffling discard pile if empty */
void deck_draw(Deck *d, Card *out_card);

/* Add a card to the discard pile */
void deck_discard(Deck *d, Card c);

#endif /* DECK_H */

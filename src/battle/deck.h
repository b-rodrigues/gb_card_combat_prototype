#ifndef DECK_H
#define DECK_H

#include "card.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_DECK_SIZE 20
#define ENEMY_DECK_SIZE 8

typedef struct {
    Card cards[MAX_DECK_SIZE];
    uint8_t count;
    uint8_t draw_idx;
    Card discard[MAX_DECK_SIZE];
    uint8_t discard_count;
} Deck;

/* Initialize standard starter deck (deterministic fixed order) */
void deck_init_default(Deck *d);

/* Banked no-arg body (ROM bank 2) dispatched by deck_init_default(). */
void deck_init_default_banked(void);

/* Draw next card from deck, reshuffling discard pile if empty */
void deck_draw(Deck *d, Card *out_card);

/* Add a card to the discard pile */
void deck_discard(Deck *d, Card c);

/* Enemy compact deck: small fixed-size deck for enemy AI card draws. */
typedef struct {
    Card cards[ENEMY_DECK_SIZE];
    uint8_t count;
    uint8_t draw_idx;
} EnemyCompactDeck;

/* Banked no-arg body (ROM bank 2) dispatched by enemy_deck_setup(). */
void enemy_deck_setup_banked(void);

/* Set up the enemy deck for the current battle via banked call. */
void enemy_deck_setup(void);

#endif /* DECK_H */

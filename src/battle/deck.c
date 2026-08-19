#include "deck.h"
#include "rng.h"

static const uint8_t s_default_starter_deck_packed[MAX_DECK_SIZE] = {
    0x02, 0x15, 0x23, 0x04, 0x47,
    0x03, 0x14, 0x22, 0x34, 0x45,
    0x05, 0x13, 0x24, 0x33, 0x42,
    0x02, 0x12, 0x25, 0x35, 0x43
};

void deck_init_default(Deck *d)
{
    uint8_t i, p;
    if (!d) return;
    d->count = MAX_DECK_SIZE;
    d->draw_idx = 0;
    d->discard_count = 0;
    for (i = 0; i < MAX_DECK_SIZE; i++) {
        p = s_default_starter_deck_packed[i];
        d->cards[i].type = (uint8_t)(p >> 4);
        d->cards[i].value = (uint8_t)(p & 0x0F);
        d->cards[i].uses_remaining = 0xFF; /* unlimited until overridden */
    }
}

void deck_draw(Deck *d, Card *out_card)
{
    uint8_t i, j;
    if (!out_card) return;
    if (!d || d->count == 0) {
        out_card->type = BATTLE_CARD_TYPE_SWORD;
        out_card->value = 2;
        out_card->uses_remaining = 0xFF;
        return;
    }

    if (d->draw_idx >= d->count) {
        if (d->discard_count > 0) {
            for (i = 0; i < d->discard_count; i++) {
                d->cards[i] = d->discard[i];
            }
            d->count = d->discard_count;
            d->discard_count = 0;
            for (i = (uint8_t)(d->count - 1); i > 0; i--) {
                Card tmp;
                do {
                    j = (uint8_t)(rng_next() & 0x1F);
                } while (j > i);
                tmp = d->cards[i];
                d->cards[i] = d->cards[j];
                d->cards[j] = tmp;
            }
        }
        d->draw_idx = 0;
    }

    *out_card = d->cards[d->draw_idx++];
}

void deck_discard(Deck *d, Card c)
{
    if (d && d->discard_count < MAX_DECK_SIZE) {
        d->discard[d->discard_count++] = c;
    }
}

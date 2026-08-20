#include "deck.h"
#include "banked.h"
#include "rng.h"

void deck_init_default(Deck *d)
{
    g_bk_call_bank = 2;
    g_bk_call_target = (uint16_t)&deck_init_default_banked;
    g_bk_ptr_a = (void *)d;
    banked_call_run();
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

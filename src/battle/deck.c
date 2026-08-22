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

void enemy_deck_setup(void)
{
    g_bk_call_bank = 2;
    g_bk_call_target = (uint16_t)&enemy_deck_setup_banked;
    banked_call_run();
}

void deck_draw(Deck *d, Card *out_card)
{
    if (!out_card) return;
    if (!d || d->draw_idx >= d->count) {
        /* Dry pile.  Reshuffling is now an explicit battle-level event (the
         * reshuffle turn), so a draw from a dry pile yields the phantom card
         * safety net instead of silently cycling the discard pile. */
        out_card->type = BATTLE_CARD_TYPE_SWORD;
        out_card->value = 2;
        out_card->uses_remaining = 0xFF;
        out_card->cost = 1;
        return;
    }

    *out_card = d->cards[d->draw_idx++];
}

/* Banked dispatch: the Fisher-Yates body lives in ROM bank 2
 * (src/battle/deck_init.c) to keep the fixed-bank _CODE budget small. */
void deck_reshuffle(Deck *d)
{
    g_bk_call_bank = 2;
    g_bk_call_target = (uint16_t)&deck_reshuffle_banked;
    g_bk_ptr_a = (void *)d;
    banked_call_run();
}

void deck_discard(Deck *d, Card c)
{
    if (d && d->discard_count < MAX_DECK_SIZE) {
        d->discard[d->discard_count++] = c;
    }
}

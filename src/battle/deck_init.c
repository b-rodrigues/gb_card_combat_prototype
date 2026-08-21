#pragma bank 2

#include "deck.h"
#include "banked.h"

/* Banked body of deck_init_default() (see deck.c).  Lives in ROM bank 2 and
 * runs through the WRAM banked-call trampoline so the starter-deck unpacker
 * and its packed table do not consume the fixed-bank _CODE budget.
 * Self-contained: it reads only its own bank-local s_starter_deck_packed
 * table and the staged Deck pointer (ptr_a), and writes through that pointer.
 * It never calls fixed-bank code (see src/core/banked.h). */

/* Safety-net fallback deck, mirroring the granted starter deck
 * (docs/deck-management.md §1): 2x SW3, 2x SH2, 1x FI4 (3 uses).  Only used
 * when the persistent DeckState is empty (legacy saves / debug states). */
static const uint8_t s_starter_deck_packed[MAX_DECK_SIZE] = {
    0x03, 0x12, 0x34,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

void deck_init_default_banked(void)
{
    Deck *d = (Deck *)g_bk_ptr_a;
    uint8_t i, p;

    if (!d) return;
    d->count = 5;
    d->draw_idx = 0;
    d->discard_count = 0;
    for (i = 0; i < d->count; i++) {
        p = s_starter_deck_packed[i];
        d->cards[i].type = (uint8_t)(p >> 4);
        d->cards[i].value = (uint8_t)(p & 0x0F);
        d->cards[i].uses_remaining = 0xFF; /* unlimited until overridden */
        d->cards[i].cost = 1;              /* basic starter cards are cheap */
    }
    /* FI4 mirrors FIRE_TOME: 3 uses per battle, energy cost 2. */
    d->cards[4].uses_remaining = 3;
    d->cards[4].cost = 2;
}

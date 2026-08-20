#pragma bank 2

#include "deck.h"
#include "banked.h"

/* Banked body of deck_init_default() (see deck.c).  Lives in ROM bank 2 and
 * runs through the WRAM banked-call trampoline so the starter-deck unpacker
 * and its packed table do not consume the fixed-bank _CODE budget.
 * Self-contained: it reads only its own bank-local s_starter_deck_packed
 * table and the staged Deck pointer (ptr_a), and writes through that pointer.
 * It never calls fixed-bank code (see src/core/banked.h). */

static const uint8_t s_starter_deck_packed[MAX_DECK_SIZE] = {
    0x02, 0x15, 0x23, 0x04, 0x47,
    0x03, 0x14, 0x22, 0x34, 0x45,
    0x05, 0x13, 0x24, 0x33, 0x42,
    0x02, 0x12, 0x25, 0x35, 0x43
};

void deck_init_default_banked(void)
{
    Deck *d = (Deck *)g_bk_ptr_a;
    uint8_t i, p;

    if (!d) return;
    d->count = MAX_DECK_SIZE;
    d->draw_idx = 0;
    d->discard_count = 0;
    for (i = 0; i < MAX_DECK_SIZE; i++) {
        p = s_starter_deck_packed[i];
        d->cards[i].type = (uint8_t)(p >> 4);
        d->cards[i].value = (uint8_t)(p & 0x0F);
        d->cards[i].uses_remaining = 0xFF; /* unlimited until overridden */
    }
}

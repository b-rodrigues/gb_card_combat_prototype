#pragma bank 2
#pragma disable_warning 110

#include "deck.h"
#include "banked.h"

/* ── Enemy deck tables (banked ROM) ─────────────────────────────
 * Small decks for each enemy type.  Each enemy draws one card per
 * attack turn; the card's power is the incoming damage the player
 * must defend against.  Decks reshuffle when exhausted.
 * Fixed order (no startup shuffle) — the starter deck follows the
 * same pattern (see deck_init.c). */

static const Card s_slime_deck[] = {
    { BATTLE_CARD_TYPE_SWORD, 2, 0xFF },
    { BATTLE_CARD_TYPE_SWORD, 3, 0xFF },
    { BATTLE_CARD_TYPE_SWORD, 3, 0xFF },
    { BATTLE_CARD_TYPE_HEAL,  2, 0xFF },
};
static const Card s_bat_deck[] = {
    { BATTLE_CARD_TYPE_BOW,   2, 0xFF },
    { BATTLE_CARD_TYPE_BOW,   3, 0xFF },
    { BATTLE_CARD_TYPE_SWORD, 4, 0xFF },
    { BATTLE_CARD_TYPE_SWORD, 3, 0xFF },
};
static const Card s_slime_trio_deck[] = {
    { BATTLE_CARD_TYPE_SWORD, 2, 0xFF },
    { BATTLE_CARD_TYPE_SWORD, 2, 0xFF },
    { BATTLE_CARD_TYPE_SWORD, 3, 0xFF },
    { BATTLE_CARD_TYPE_SWORD, 3, 0xFF },
    { BATTLE_CARD_TYPE_HEAL,  2, 0xFF },
};

void enemy_deck_setup_banked(void)
{
    EnemyCompactDeck *ed = (EnemyCompactDeck *)g_bk_ptr_a;
    uint8_t battle_id = g_bk_byte_a;
    const Card *src = (const Card *)0;
    uint8_t count = 0, i;

    if (!ed) return;

    switch (battle_id) {
    case 1:  src = s_slime_deck;      count = 4; break;
    case 2:  src = s_bat_deck;        count = 4; break;
    case 3:  src = s_slime_trio_deck; count = 5; break;
    }

    if (!src) return;
    if (count == 0 || count > ENEMY_DECK_SIZE) return;

    for (i = 0; i < count; i++) {
        ed->cards[i].type = src[i].type;
        ed->cards[i].value = src[i].value;
        ed->cards[i].uses_remaining = src[i].uses_remaining;
        ed->cards[i].cost = 0; /* enemies never pay energy */
        ed->cards[i].effect = src[i].effect; /* NONE: enemies never resolve
                                              * effects -- their card value
                                              * IS the incoming damage. */
        ed->cards[i].status_id = src[i].status_id;
        ed->cards[i].status_chance = src[i].status_chance;
    }
    ed->count = count;
    ed->draw_idx = 0;
}

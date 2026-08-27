#pragma bank 2

#include "deck.h"
#include "banked.h"
#include "rng.h"
#include "rpg/cards.h"
#include "rpg/status.h"

/* Banked body of deck_init_default() (see deck.c).  Lives in ROM bank 2 and
 * runs through the WRAM banked-call trampoline so the starter-deck unpacker
 * and its packed table do not consume the fixed-bank _CODE budget.
 * Self-contained: it reads only its own bank-local s_starter_deck_packed
 * table and the staged Deck pointer (ptr_a), and writes through that pointer.
 * It never calls fixed-bank code (see src/core/banked.h). */

/* Safety-net fallback deck, mirroring the granted starter deck
 * (docs/deck-management.md §1): 4x SW3, 3x SH2, 3x Fire Sword (SW4, burn
 * rider applied below), 2x DA1.  Only used when the persistent DeckState
 * is empty (legacy saves / debug states).  First five entries match the
 * granted deal order so the opening hand is identical whether battles run
 * on real or fallback state.  Nibble = (type<<4)|value with the renumbered
 * BattleCardType (SW=0, SH=1, BO=2, HE=3, DA=4); the fire sword rides the
 * SWORD type (0) with value 4 and is marked BURN below by index. */
static const uint8_t s_starter_deck_packed[MAX_DECK_SIZE] = {
    0x03, 0x03, 0x12, 0x12, 0x04,
    0x03, 0x12, 0x04, 0x04, 0x03,
    0x41, 0x41,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

/* Bank-local type→effect defaults (mirrors card_effect_for_type(); banked
 * code must not call fixed-bank helpers, AGENTS.md 52.11.1). */
static const uint8_t s_type_effects[5] = {
    CARD_EFFECT_DAMAGE_TARGET,  /* SWORD */
    CARD_EFFECT_BLOCK_DAMAGE,   /* SHIELD */
    CARD_EFFECT_DAMAGE_TARGET,  /* BOW */
    CARD_EFFECT_HEAL_HP,        /* HEAL */
    CARD_EFFECT_DAMAGE_TARGET   /* DAGGER */
};

/* Compile-time check: s_type_effects positional order must match the
 * BattleCardType enum (SW=0, SH=1, BO=2, HE=3, DA=4). */
typedef char assert_battle_card_types_match[
    (BATTLE_CARD_TYPE_SWORD == 0 &&
     BATTLE_CARD_TYPE_SHIELD == 1 &&
     BATTLE_CARD_TYPE_BOW == 2 &&
     BATTLE_CARD_TYPE_HEAL == 3 &&
     BATTLE_CARD_TYPE_DAGGER == 4) ? 1 : -1
];

void deck_init_default_banked(void)
{
    Deck *d = (Deck *)g_bk_ptr_a;
    uint8_t i, p, t;

    if (!d) return;
    d->count = 12;
    d->draw_idx = 0;
    d->discard_count = 0;
    for (i = 0; i < d->count; i++) {
        p = s_starter_deck_packed[i];
        t = (uint8_t)(p >> 4);
        d->cards[i].type = t;
        d->cards[i].value = (uint8_t)(p & 0x0F);
        d->cards[i].uses_remaining = 0xFF; /* unlimited until overridden */
        d->cards[i].cost = 1;              /* basic starter cards are cheap */
        d->cards[i].effect =
            (t < 5) ? s_type_effects[t] : CARD_EFFECT_DAMAGE_TARGET;
        d->cards[i].status_id = STATUS_NONE; /* starter cards: no rider */
        d->cards[i].status_chance = 0;
        /* Fire Sword: the starter's fire entries are SWORD value 4 and
         * carry the BURN rider (the packed nibble encodes type+value
         * only, so the rider is applied here, mirroring CARD_FIRE_SWORD). */
        if (t == BATTLE_CARD_TYPE_SWORD && d->cards[i].value == 4) {
            d->cards[i].status_id = STATUS_BURN;
            d->cards[i].status_chance = 128;
        }
    }
}

/* Field-wise card copy/swap helpers.  The banked body MUST NOT use struct
 * assignment: SDCC lowers Card-sized copies to __memcpy, which links into
 * the switchable home bank 1 -- unreachable while bank 2 is mapped
 * (AGENTS.md 52.11.1). */
static void card_copy_banked(Card *dst, const Card *src)
{
    dst->type = src->type;
    dst->value = src->value;
    dst->uses_remaining = src->uses_remaining;
    dst->cost = src->cost;
    dst->effect = src->effect;
    dst->status_id = src->status_id;
    dst->status_chance = src->status_chance;
}

static void card_swap_banked(Card *a, Card *b)
{
    Card t;
    t.type = a->type;
    t.value = a->value;
    t.uses_remaining = a->uses_remaining;
    t.cost = a->cost;
    t.effect = a->effect;
    t.status_id = a->status_id;
    t.status_chance = a->status_chance;
    a->type = b->type;
    a->value = b->value;
    a->uses_remaining = b->uses_remaining;
    a->cost = b->cost;
    a->effect = b->effect;
    a->status_id = b->status_id;
    a->status_chance = b->status_chance;
    b->type = t.type;
    b->value = t.value;
    b->uses_remaining = t.uses_remaining;
    b->cost = t.cost;
    b->effect = t.effect;
    b->status_id = t.status_id;
    b->status_chance = t.status_chance;
}

/* Banked body of deck_reshuffle() (see deck.c).  Self-contained: reads only
 * the staged Deck pointer (ptr_a) and its own bank-local data, and advances
 * the shared RNG stream via an inlined xorshift on g_rng_state (banked code
 * must not call fixed-bank functions; see AGENTS.md 52.11.1).  The step
 * must stay byte-identical to rng_next(). */
void deck_reshuffle_banked(void)
{
    Deck *d = (Deck *)g_bk_ptr_a;
    uint8_t i, j;

    if (!d || d->discard_count == 0) return;

    for (i = 0; i < d->discard_count; i++) {
        card_copy_banked(&d->cards[i], &d->discard[i]);
    }
    d->count = d->discard_count;
    d->discard_count = 0;
    d->draw_idx = 0;

    /* Fisher-Yates: rejection-sample j in [0, i] (mask 0x1F covers
     * MAX_DECK_SIZE - 1). */
    for (i = (uint8_t)(d->count - 1); i > 0; i--) {
        do {
            g_rng_state ^= g_rng_state << 7;
            g_rng_state ^= g_rng_state >> 9;
            g_rng_state ^= g_rng_state << 8;
            j = (uint8_t)(g_rng_state & 0x1F);
        } while (j > i);
        card_swap_banked(&d->cards[i], &d->cards[j]);
    }
}

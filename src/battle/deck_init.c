#pragma bank 2

#include "deck.h"
#include "banked.h"
#include "rng.h"
#include "rpg/cards.h"
#include "rpg/status.h"

/* Banked body of deck_init_default() (see deck.c).  Lives in ROM bank 2 and
 * runs through the WRAM banked-call trampoline so the starter-deck unpacker
 * does not consume the fixed-bank _CODE budget.
 * Self-contained: it reads only the staged Deck pointer (ptr_a), the
 * generated starter table (same bank: direct read, like
 * battle_init_deck_banked's g_card_defs reads) and the registered card
 * catalog (bank 2, direct read).  It never calls fixed-bank code
 * (see src/core/banked.h). */

/* Generated hero starter deck (screens/hero.json, compiled by
 * battle_compile.py into src/game/hero_content.c, bank 2).  Declared
 * locally (not via a game-layer header) so this engine file keeps its
 * generic dependency direction (AGENTS.md 55.1); the table shape is the
 * contract: count byte + ordered CardIds in exact draw-pile order. */
extern const uint8_t g_hero_starter_deck_count;
extern const uint8_t g_hero_starter_deck_ids[];

void deck_init_default_banked(void)
{
    Deck *d = (Deck *)g_bk_ptr_a;
    uint8_t i, j;
    uint8_t n;

    if (!d) return;
    n = g_hero_starter_deck_count;
    if (n > MAX_DECK_SIZE) n = MAX_DECK_SIZE;
    d->count = n;
    d->draw_idx = 0;
    d->discard_count = 0;
    for (i = 0; i < n; i++) {
        /* Same row resolution as battle_init_deck_banked() for the same
         * ids, so the fallback deck is byte-identical to a granted deck
         * built from the same starter table (opening hand identical
         * whether battles run on real or fallback state). */
        const CardDefinition *def = (const CardDefinition *)0;
        for (j = 0; j < g_card_defs_count; j++) {
            if (g_card_defs[j].id == g_hero_starter_deck_ids[i]) {
                def = &g_card_defs[j];
                break;
            }
        }
        if (def) {
            d->cards[i].type = def->battle_type;
            d->cards[i].value = def->power;
            d->cards[i].uses_remaining =
                (def->uses_per_battle == 0) ? 0xFF : def->uses_per_battle;
            d->cards[i].cost = def->cost;
            d->cards[i].effect = def->effect;
            d->cards[i].status_id = def->status_id;
            d->cards[i].status_chance = def->status_chance;
            d->cards[i].ring =
                (def->battle_type == BATTLE_CARD_TYPE_HEAL) ? 1 : 0;
        } else {
            /* Unknown id: safety-net sword, mirroring the phantom draw. */
            d->cards[i].type = BATTLE_CARD_TYPE_SWORD;
            d->cards[i].value = 2;
            d->cards[i].uses_remaining = 0xFF;
            d->cards[i].cost = 1;
            d->cards[i].effect = CARD_EFFECT_DAMAGE_TARGET;
            d->cards[i].status_id = STATUS_NONE;
            d->cards[i].status_chance = 0;
            d->cards[i].ring = 0;
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

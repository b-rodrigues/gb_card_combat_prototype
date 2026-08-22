#include "rpg/cards.h"
#include "banked.h"
#include <stddef.h>

/* The card catalog is game content, registered at boot via
 * card_register_defs() (see src/game/cards.c). */
static const CardDefinition *g_cards = NULL;
static uint8_t g_card_count = 0;
static uint8_t g_card_bank = 2;
static CardDefinition s_card_scratch;

void card_register_defs(const CardDefinition *defs, uint8_t count,
                        uint8_t bank)
{
    g_cards = defs;
    g_card_count = count;
    g_card_bank = bank;
}

const CardDefinition *card_get_def(CardId id)
{
    uint8_t i;
    if (!g_cards) return NULL;
    for (i = 0; i < g_card_count; i++) {
        banked_copy(g_card_bank, &s_card_scratch, &g_cards[i],
                    sizeof(CardDefinition));
        if (s_card_scratch.id == id) {
            return &s_card_scratch;
        }
    }
    return NULL;
}

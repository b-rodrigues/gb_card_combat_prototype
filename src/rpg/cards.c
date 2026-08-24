#include "rpg/cards.h"
#include "banked.h"
#include <stddef.h>

/* The card catalog is game content, registered at boot via
 * card_register_defs() (see src/game/cards.c). */
const CardDefinition *g_card_defs = NULL;
uint8_t g_card_defs_count = 0;
static uint8_t g_card_bank = 2;
static CardDefinition g_card_scratch;

void card_register_defs(const CardDefinition *defs, uint8_t count,
                        uint8_t bank)
{
    g_card_defs = defs;
    g_card_defs_count = count;
    g_card_bank = bank;
}

const CardDefinition *card_get_def(CardId id)
{
    uint8_t i;
    if (!g_card_defs) return NULL;

    for (i = 0; i < g_card_defs_count; i++) {
        banked_copy(g_card_bank, &g_card_scratch, &g_card_defs[i],
                    sizeof(CardDefinition));
        if (g_card_scratch.id == id) {
            return &g_card_scratch;
        }
    }
    return NULL;
}

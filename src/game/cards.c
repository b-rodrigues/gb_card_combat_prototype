#include "rpg/cards.h"
#include "game_ids.h"

extern const CardDefinition g_cards[];

void game_cards_register(void)
{
    card_register_defs(g_cards, GAME_CARD_COUNT, GAME_CONTENT_BANK);
}

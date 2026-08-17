#include "card.h"

static const char * const s_card_codes[5] = { "SW", "SH", "BO", "FI", "HE" };
static const char * const s_card_descs[5] = {
    "Sword: physical",
    "Shield: block dmg",
    "Bow: ranged dmg",
    "Fire: magic dmg",
    "Heal: restore HP"
};

const char *card_type_code(uint8_t type)
{
    return (type < 5) ? s_card_codes[type] : "??";
}

const char *card_get_description(uint8_t type)
{
    return (type < 5) ? s_card_descs[type] : "";
}



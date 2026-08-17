#include "card.h"

static const char * const s_card_codes = "SW\0SH\0BO\0FI\0HE\0??";

const char *card_type_code(uint8_t type)
{
    return (type < 5) ? (s_card_codes + (type * 3)) : (s_card_codes + 15);
}

const char *card_get_description(uint8_t type)
{
    switch (type) {
        case CARD_TYPE_SWORD:  return "Sword: physical";
        case CARD_TYPE_SHIELD: return "Shield: block dmg";
        case CARD_TYPE_BOW:    return "Bow: ranged dmg";
        case CARD_TYPE_FIRE:   return "Fire: magic dmg";
        case CARD_TYPE_HEAL:   return "Heal: restore HP";
        default: return "";
    }
}



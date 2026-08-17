#include "card.h"

const char *card_type_code(uint8_t type)
{
    switch (type) {
        case CARD_TYPE_SWORD:  return "SW";
        case CARD_TYPE_SHIELD: return "SH";
        case CARD_TYPE_BOW:    return "BO";
        case CARD_TYPE_FIRE:   return "FI";
        case CARD_TYPE_HEAL:   return "HE";
        default: return "??";
    }
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



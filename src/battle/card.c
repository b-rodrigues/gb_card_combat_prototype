#include "card.h"

static const char s_type_codes[5][3] = {
    "SW", "SH", "BO", "FI", "HE"
};

const char *card_type_code(uint8_t type)
{
    return (type < 5) ? s_type_codes[type] : "??";
}

static const char * const s_card_descs[5] = {
    "Sword: physical",
    "Shield: block dmg",
    "Bow: ranged dmg",
    "Fire: magic dmg",
    "Heal: restore HP"
};

const char *card_get_description(Card card)
{
    return (card.type < 5) ? s_card_descs[card.type] : "";
}

void card_format(Card card, char *out_buf)
{
    const char *code;
    if (!out_buf) return;
    code = card_type_code(card.type);
    out_buf[0] = code[0];
    out_buf[1] = code[1];
    out_buf[2] = (char)('0' + card.value);
    out_buf[3] = '\0';
}

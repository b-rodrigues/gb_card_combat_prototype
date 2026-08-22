#pragma bank 2

#include "card.h"

/* Packed card descriptions (fixed 18-byte stride, NUL-padded), staged to
 * WRAM by card_get_description() via banked_copy.  Bank 2 keeps the text
 * out of the fixed-bank budget (make memmap). */
const uint8_t s_card_desc_blob[CARD_DESC_TYPES * CARD_DESC_STRIDE] = {
    /* SWORD */  "Sword: physical\0\0\0",
    /* SHIELD */ "Shield: block dmg\0",
    /* BOW */    "Bow: ranged dmg\0\0\0",
    /* FIRE */   "Fire: magic dmg\0\0\0",
    /* HEAL */   "Heal: restore HP\0\0"
};

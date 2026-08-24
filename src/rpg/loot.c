#include "rpg/loot.h"
#include "banked.h"

/* Fixed-bank shim for the bank-2 loot module (src/rpg/loot_banked.c):
 * tables, rolls, and definition synthesis live behind the trampoline;
 * only the derived-id byte and this tiny dispatcher stay in the fixed
 * bank. */

uint8_t g_loot_id;

void loot_roll_drop(uint8_t weapon)
{
    g_bk_call_bank = 2;
    g_bk_call_target = (uint16_t)&loot_roll_drop_banked;
    g_bk_byte_a = weapon;
    banked_call_run();
}

CardId loot_encode_id(uint8_t material, uint8_t effect, uint8_t weapon)
{
    /* Matches loot_banked.c's encode: BASE + mat*64 + eff*8 + wpn.
     * Shifts/masks only -- no library calls (52.11.1). */
    return (CardId)(LOOT_ID_BASE +
                    (((material << 3) + effect) << 3) + weapon);
}

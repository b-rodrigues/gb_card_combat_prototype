#include "save.h"
#include "banked.h"

/* Fixed-bank dispatchers for the bank-2 SRAM bodies
 * (src/rpg/save_banked.c), keeping the checksum/copy loops out of the
 * fixed-bank budget.  Results report through g_save_ok (shared WRAM
 * byte, written by the banked body).  See banked.h for the ABI. */

extern uint8_t g_save_ok;

bool save_present_slot(uint8_t slot)
{
    g_bk_call_bank = 2;
    g_bk_call_target = (uint16_t)&save_op_banked;
    g_bk_ptr_a = (void *)0;
    g_bk_byte_a = slot;
    g_bk_byte_b = 0; /* present */
    banked_call_run();
    return g_save_ok != 0;
}

bool save_game_slot(uint8_t slot, const GameState *state)
{
    if (!state) return false;
    g_bk_call_bank = 2;
    g_bk_call_target = (uint16_t)&save_op_banked;
    g_bk_ptr_a = (void *)state;
    g_bk_byte_a = slot;
    g_bk_byte_b = 1; /* save */
    banked_call_run();
    return g_save_ok != 0;
}

bool load_game_slot(uint8_t slot, GameState *state)
{
    if (!state) return false;
    g_bk_call_bank = 2;
    g_bk_call_target = (uint16_t)&save_op_banked;
    g_bk_ptr_a = (void *)state;
    g_bk_byte_a = slot;
    g_bk_byte_b = 2; /* load */
    banked_call_run();
    return g_save_ok != 0;
}

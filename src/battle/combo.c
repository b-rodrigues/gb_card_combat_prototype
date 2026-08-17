#include "combo.h"
#include "banked.h"

/* combo_evaluate() is a fixed-bank wrapper around the banked body in
 * src/battle/combo_content.c (ROM bank 2), which keeps combo evaluation
 * out of the fixed-bank budget.  The wrapper stages its arguments into the
 * _DATA globals (banked.c) and runs the banked no-arg function through the
 * WRAM banked-call trampoline (crt0.s): see src/core/banked.h.  The banked
 * body must be self-contained, so all data it reads lives either in bank 2
 * or in the staged _DATA globals. */
void combo_evaluate(const Card *cards, uint8_t count, ComboPhase phase, ComboResult *out_result)
{
    g_bk_call_bank = 2;
    g_bk_call_target = (uint16_t)&combo_evaluate_banked;
    g_bk_ptr_a = (void *)cards;
    g_bk_ptr_b = (void *)out_result;
    g_bk_byte_a = count;
    g_bk_byte_b = (uint8_t)phase;
    banked_call_run();
}
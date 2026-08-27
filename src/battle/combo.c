#include "combo.h"
#include "banked.h"

/* combo_resolve() is a fixed-bank wrapper around the banked body in
 * src/battle/combo_content.c (ROM bank 3), which evaluates the hand and
 * resolves its effect in one dispatch -- keeping both out of the
 * fixed-bank budget.  The wrapper stages its arguments into the _DATA
 * globals (banked.c) and runs the banked no-arg function through the WRAM
 * banked-call trampoline (crt0.s): see src/core/banked.h.
 *
 * Staging: byte_b packs phase (bit 0) and effect_type (bits 1..) -- the
 * trampoline exposes exactly two pointers and two bytes, and effect ids
 * are small enums. */
void combo_resolve(const Card *cards, uint8_t count, ComboPhase phase,
                   uint8_t effect_type, ComboResult *out_result)
{
    g_bk_call_bank = 3;
    g_bk_call_target = (uint16_t)&combo_resolve_banked;
    g_bk_ptr_a = (void *)cards;
    g_bk_ptr_b = (void *)out_result;
    g_bk_byte_a = count;
    g_bk_byte_b = (uint8_t)((uint8_t)phase | (uint8_t)(effect_type << 1));
    banked_call_run();
}


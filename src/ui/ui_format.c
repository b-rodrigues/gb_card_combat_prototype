#pragma bank 2

#include "banked.h"

/* Banked body of ui_format_int() (see ui.c).  Lives in ROM bank 2 and runs
 * through the WRAM banked-call trampoline so the ~700-byte decimal formatter
 * does not consume the fixed-bank _CODE budget.  Self-contained: it reads
 * only its own bank-local s_p10 table and the staged _DATA globals (value in
 * byte_a/byte_b, out pointer in ptr_a), and writes through the staged out
 * pointer.  It never calls fixed-bank code (see src/core/banked.h). */

static const uint16_t s_p10[4] = { 10000, 1000, 100, 10 };

void ui_format_int_banked(void)
{
    int16_t value;
    char *out;
    uint16_t uval;
    uint8_t i, started = 0;

    value = (int16_t)((uint16_t)g_bk_byte_a | ((uint16_t)g_bk_byte_b << 8));
    out = (char *)g_bk_ptr_a;
    if (!out) return;

    if (value < 0) {
        *out++ = '-';
        uval = (uint16_t)(-value);
    } else {
        uval = (uint16_t)value;
    }
    for (i = 0; i < 4; i++) {
        uint16_t p = s_p10[i];
        uint8_t d = 0;
        while (uval >= p) {
            uval -= p;
            d++;
        }
        if (d != 0 || started) {
            *out++ = (char)('0' + d);
            started = 1;
        }
    }
    *out++ = (char)('0' + (uint8_t)uval);
    *out = '\0';
}

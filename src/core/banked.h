#ifndef BANKED_H
#define BANKED_H

#include <stdint.h>

/* RAM-resident MBC5 bank-switch + copy (src/crt0.s).  banked_copy() copies
 * `n` bytes from a banked ROM source into a WRAM destination, running the
 * MBC5 bank switch from a WRAM trampoline so it is safe regardless of the
 * caller's currently mapped ROM bank.  The trampoline must be resident
 * before any banked content is read: game_init() calls banked_copy_init()
 * once at boot (the harness skips CRT0, so the copy cannot live there).
 *
 * The bank register is always restored to the project's home bank 1 (the
 * -yo8 layout maps fixed-bank code above CPU 0x4000 through ROMB=1; see
 * crt0.s) before returning, so banked content must be fully copied out
 * before the call returns.  Never read banked data after the call.  `n` is
 * a uint8_t, so tables (rows or strings) must be smaller than 256 bytes.
 *
 * The trampoline itself lives in the linker-allocated WRAM buffer
 * g_banked_tramp[] (declared here so the engine and crt0.s agree on its
 * symbol): a hardcoded WRAM address (e.g. 0xC940) would collide with
 * linker-placed _INITIALIZED variables such as _g_boot_phase. */
extern uint8_t g_banked_tramp[64];

void banked_copy(uint8_t bank, void *dst, const void *src, uint8_t n);
void banked_copy_init(void);

/* ── Banked-call trampoline (src/crt0.s) ─────────────────────────────
 * Runs a fixed no-arg function that lives in a banked ROM bank from a
 * fixed-bank caller (used to host self-contained engine modules such as
 * src/battle/combo.c out of the fixed bank).
 *
 * ── ABI / reentrancy contract ──────────────────────────────────────
 * banked_call_run() is:
 *   * SYNCHRONOUS: it does not return until the banked target returns.
 *   * NON-REENTRANT: it dispatches through the shared staging globals
 *     below (g_bk_call_bank/target/ptr_a/ptr_b/byte_a/byte_b).  There is
 *     no nesting: a banked target must NOT call another banked function,
 *     and a caller must not invoke banked_call_run() from within a
 *     banked target.  The staging globals are a one-shot transfer, not a
 *     persistent API — overwrite them before each call.
 *   * INTERRUPT-SAFE only because the WRAM trampoline runs with
 *     interrupts disabled (di at entry, ei on return) for the entire
 *     switch + execute + restore.  Do not re-enable interrupts inside
 *     the target.
 *   * The target runs with a CONTENT ROM BANK SELECTED, not the caller's
 *     bank.  It may access only (a) its own bank-local code/data and (b)
 *     the staged _DATA globals below.  It must NOT call any fixed-bank
 *     function or return through a fixed-bank pointer while executing.
 *   * On return the home bank (HOME_BANK, crt0.s) and __current_bank are
 *     restored, so the caller's execution ring is unchanged.
 *
 * The fixed-bank wrapper stages the target bank + logical address and its
 * arguments into the _DATA globals below, then calls banked_call_run()
 * (asm) so the WRAM trampoline never parses SDCC's stack layout.  The
 * banked target reads the staged globals.  g_bk_ptr_a/b carry pointer
 * arguments; g_bk_byte_a/b carry byte arguments.  The target's logical
 * address must be a bank-relative offset in 0x0000-0x3FFF, because the
 * trampoline maps it to runtime 0x4000 | (target & 0x3FFF).  The
 * fixed-bank wrapper takes the address of the banked symbol, whose low 14
 * bits are the bank-relative offset by construction of the linker layout. */
extern uint8_t g_bk_call_bank;
extern uint16_t g_bk_call_target;
extern void *g_bk_ptr_a;
extern void *g_bk_ptr_b;
extern uint8_t g_bk_byte_a;
extern uint8_t g_bk_byte_b;
extern uint8_t g_bk_byte_c;
extern uint8_t g_bk_byte_d;

/* Linker-allocated home for the banked-call trampoline (WRAM), copied by
 * banked_call_init() at boot (game_init calls it, like banked_copy_init). */
extern uint8_t g_banked_call_tramp[64];

extern void banked_call_run(void);
void banked_call_init(void);

#endif /* BANKED_H */

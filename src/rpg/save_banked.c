#pragma bank 2

#include "save.h"
#include <gb/gb.h>
#include "banked.h"

/* ── Banked SRAM save/load bodies (src/rpg/save.c dispatchers) ──────
 * Runs from ROM bank 2 via the WRAM banked-call trampoline so the
 * checksum/copy loops do not consume the fixed-bank budget.  Pure
 * WRAM/SRAM byte access: ENABLE_RAM/DISABLE_RAM expand to plain rRAMG
 * register stores (no calls), so this is self-contained per the banked
 * ABI (AGENTS.md 52.11.1).
 *
 * Staging: ptr_a = GameState* (NULL for save_present_slot), byte_a =
 * slot index.  Result reports through the shared WRAM byte below;
 * battles/save flows are sequential, single slot by design. */

uint8_t g_save_ok;


/* NOTE: SDCC does not ERROR on the negative-array-size assert idiom, so a
 * compile-time slot guard cannot be trusted.  save_op_banked() therefore
 * enforces the bound at RUNTIME and refuses to touch SRAM when the state
 * outgrew the slot (preventing silent cross-slot corruption). */

static uint8_t *save_slot_base(uint8_t slot)
{
    /* Stride via explicit shift-10 (= x1024), NOT a multiply: banked code
     * must not pull the SDCC multiply library (fixed bank, AGENTS.md
     * 52.11.1).  Keep in sync with SAVE_SLOT_STRIDE (0x400 = 1<<10). */
    return (uint8_t *)(SAVE_SRAM_BASE + ((uint16_t)slot << 10));
}

static uint16_t save_checksum(const GameState *state)
{
    uint16_t sum = 0;
    uint16_t n = (uint16_t)sizeof(GameState);   /* u16: state grew past 255
                                                 * with the loot collection */
    const uint8_t *b = (const uint8_t *)state;
    while (n--) sum = (uint8_t)(sum + *b++);
    return sum & 0xFF;                          /* stored checksum stays u8 */
}

static bool save_valid_at_slot(const uint8_t *sram)
{
    if (sram[0] != (uint8_t)(SAVE_MAGIC & 0xFF) ||
        sram[1] != (uint8_t)(SAVE_MAGIC >> 8) ||
        sram[2] != SAVE_VERSION) {
        return false;
    }
    return sram[3] == save_checksum((const GameState *)(sram + 4));
}

static void sram_copy(uint8_t *dst, const uint8_t *src, uint16_t count)
{
    while (count--) *dst++ = *src++;
}

void save_op_banked(void)
{
    uint8_t slot = g_bk_byte_a;
    GameState *state = (GameState *)g_bk_ptr_a;
    uint8_t op = g_bk_byte_b; /* 0=present, 1=save, 2=load */
    uint8_t *sram;
    bool valid;

    g_save_ok = 0;
    if (slot >= SAVE_SLOT_COUNT) return;
    /* Runtime slot-capacity guard: the compile-time typedef assert is NOT
     * enforced by SDCC -- without this, an oversized GameState silently
     * overflows into the next slot (observed: 806-byte state in a
     * historically 252-byte slot layout). */
    /* Runtime slot-capacity guard: refuses SRAM access when the state
     * outgrew its 1020-byte lane (prevents silent cross-slot corruption).
     * g_save_state_capacity comes from WRAM -- do NOT replace with a
     * compile-time constant: SDCC would fold the branch into unreachable
     * code (lint), and older sibling guards silently never fired. */
    if (g_save_state_capacity <
        (uint16_t)sizeof(GameState)) {
        return;
    }
    sram = save_slot_base(slot);

    switch (op) {
        case 0:
            ENABLE_RAM;
            valid = save_valid_at_slot(sram);
            DISABLE_RAM;
            g_save_ok = valid ? 1 : 0;
            return;
        case 1:
            if (!state) return;
            ENABLE_RAM;
            sram[0] = (uint8_t)(SAVE_MAGIC & 0xFF);
            sram[1] = (uint8_t)(SAVE_MAGIC >> 8);
            sram[2] = SAVE_VERSION;
            sram[3] = save_checksum(state);
            sram_copy(sram + 4, (const uint8_t *)state,
                      (uint16_t)sizeof(GameState));
            DISABLE_RAM;
            g_save_ok = 1;
            return;
        default:
            if (!state) return;
            ENABLE_RAM;
            valid = save_valid_at_slot(sram);
            if (valid) {
                sram_copy((uint8_t *)state, sram + 4,
                          (uint16_t)sizeof(GameState));
            }
            DISABLE_RAM;
            g_save_ok = valid ? 1 : 0;
            return;
    }
}

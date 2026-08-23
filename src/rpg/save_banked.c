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

#define SAVE_SRAM_MAGIC_OFF     0x0000
#define SAVE_SRAM_VERSION_OFF   0x0002
#define SAVE_SRAM_CHECKSUM_OFF  0x0003
#define SAVE_SRAM_STATE_OFF     0x0004
#define SAVE_SRAM_BASE          0xA000
#define SAVE_SLOT_STRIDE        0x0100

typedef char check_save_slot_size[(sizeof(GameState) <= (SAVE_SLOT_STRIDE - SAVE_SRAM_STATE_OFF)) ? 1 : -1];

static uint8_t *save_slot_base(uint8_t slot)
{
    return (uint8_t *)(SAVE_SRAM_BASE + ((uint16_t)slot << 8));
}

static uint8_t save_checksum(const GameState *state)
{
    uint8_t sum = 0;
    uint8_t n = (uint8_t)sizeof(GameState);
    const uint8_t *b = (const uint8_t *)state;
    while (n--) sum = (uint8_t)(sum + *b++);
    return sum;
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

static void sram_copy(uint8_t *dst, const uint8_t *src, uint8_t count)
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
                      (uint8_t)sizeof(GameState));
            DISABLE_RAM;
            g_save_ok = 1;
            return;
        default:
            if (!state) return;
            ENABLE_RAM;
            valid = save_valid_at_slot(sram);
            if (valid) {
                sram_copy((uint8_t *)state, sram + 4,
                          (uint8_t)sizeof(GameState));
            }
            DISABLE_RAM;
            g_save_ok = valid ? 1 : 0;
            return;
    }
}

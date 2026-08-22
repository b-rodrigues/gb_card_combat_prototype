#pragma bank 2

#include "status.h"
#include "banked.h"

/* ── Status definitions + banked bodies (Phase C) ───────────────────
 * Runs from ROM bank 2 via the WRAM banked-call trampoline; reads only
 * its own bank-local definition table and the staged pointers/bytes, and
 * writes through staged pointers / shared WRAM result structs
 * (g_status_applied in status.c).  Never calls fixed-bank code.
 *
 * Stacking rule (plan §16): POISON STACKs up to max_stacks and refreshes
 * duration.  Balance values are deliberately provisional (§26). */

static const StatusDefinition s_status_defs[STATUS_DEF_COUNT] = {
    /* id                      tick/stack  max  dur */
    { STATUS_NONE,                  0,      0,   0 },
    { STATUS_POISON,                1,      5,   3 }
};

static const StatusDefinition *status_def(uint8_t id)
{
    uint8_t i;
    for (i = 0; i < STATUS_DEF_COUNT; i++) {
        if (s_status_defs[i].id == id) {
            return &s_status_defs[i];
        }
    }
    return (const StatusDefinition *)0;
}

void status_apply_banked(void)
{
    StatusSlots *slots = (StatusSlots *)g_bk_ptr_a;
    uint8_t id = g_bk_byte_a;
    uint8_t stacks = (uint8_t)(g_bk_byte_b >> 4);
    uint8_t duration = (uint8_t)(g_bk_byte_b & 0x0F);
    const StatusDefinition *def;
    uint8_t i;

    if (!slots || id == STATUS_NONE) return;
    def = status_def(id);
    if (!def) return;

    if (stacks == 0) stacks = 1;
    if (duration == 0) duration = def->duration;
    if (stacks > def->max_stacks) stacks = def->max_stacks;

    /* STACK rule: existing instance gains stacks (capped), duration
     * refreshed to the request. */
    for (i = 0; i < slots->count; i++) {
        if (slots->slot[i].id == id) {
            uint16_t total = (uint16_t)(slots->slot[i].stacks + stacks);
            if (total > def->max_stacks) total = def->max_stacks;
            slots->slot[i].stacks = (uint8_t)total;
            slots->slot[i].duration = duration;
            g_status_applied.id = id;
            g_status_applied.stacks = slots->slot[i].stacks;
            g_status_applied.duration = duration;
            return;
        }
    }

    if (slots->count >= MAX_STATUSES_PER_COMBATANT) return;
    slots->slot[slots->count].id = id;
    slots->slot[slots->count].stacks = stacks;
    slots->slot[slots->count].duration = duration;
    slots->count++;
    g_status_applied.id = id;
    g_status_applied.stacks = stacks;
    g_status_applied.duration = duration;
}

void status_tick_banked(void)
{
    StatusSlots *slots = (StatusSlots *)g_bk_ptr_a;
    uint8_t i;
    uint8_t damage = 0;
    uint8_t expired = 0;

    if (!slots) return;

    g_status_tick.damage = 0;
    g_status_tick.expired_count = 0;

    for (i = 0; i < slots->count; ) {
        StatusInstance *inst = &slots->slot[i];
        const StatusDefinition *def = status_def(inst->id);
        if (def != (const StatusDefinition *)0 && def->tick_per_stack != 0) {
            /* Add-loop, not multiply: a '*' here promotes through
             * __mulint, which links into the FIXED bank -- an illegal
             * call while bank 2 is mapped (AGENTS.md 52.11.1) -- and the
             * library costs more than this loop ever will. */
            uint8_t s;
            for (s = 0; s < inst->stacks; s++) {
                damage += def->tick_per_stack;
            }
        }
        inst->duration--;
        if (inst->duration == 0) {
            if (expired < MAX_STATUSES_PER_COMBATANT) {
                g_status_tick.expired_ids[expired++] = inst->id;
            }
            /* Remove by shifting the tail down, field-wise: struct
             * assignment lowers to __memcpy, which lives in the fixed
             * bank and is unreachable while bank 2 is mapped. */
            slots->slot[i].id = slots->slot[slots->count - 1].id;
            slots->slot[i].stacks = slots->slot[slots->count - 1].stacks;
            slots->slot[i].duration = slots->slot[slots->count - 1].duration;
            slots->count--;
        } else {
            i++;
        }
    }

    g_status_tick.damage = damage;
    g_status_tick.expired_count = expired;
}

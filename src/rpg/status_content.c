#pragma bank 3

#include "status.h"
#include "banked.h"

/* ── Status definitions + banked bodies (Phase C) ───────────────────
 * Runs from ROM bank 3 via the WRAM banked-call trampoline; reads only
 * its own bank-local definition table and the staged pointers/bytes, and
 * writes through staged pointers / shared WRAM result structs
 * (g_status_applied in status.c).  Never calls fixed-bank code.
 *
 * Stacking rule (plan §16): POISON/BURN STACK up to max_stacks and
 * refresh duration; FREEZE does not stack (control status -- a frozen
 * combatant can neither attack nor defend for the whole duration,
 * battle-side).  Balance values are deliberately provisional (§26). */

static const StatusDefinition s_status_defs[STATUS_DEF_COUNT] = {
    /* id            tick  max  dur */
    { STATUS_NONE,     0,   0,   0 },
    { STATUS_POISON,   1,   5,   3 },
    { STATUS_BURN,     1,   3,   3 },
    { STATUS_FREEZE,   0,   1,   3 }
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
    uint8_t actor_slot = g_bk_byte_a;
    uint8_t i;
    uint8_t damage = 0;
    uint8_t expired = 0;

    if (!slots) return;

    g_status_tick.damage = 0;
    g_status_tick.expired_count = 0;
    /* Consume last round's frozen flag for this actor; re-set below if
     * the status is still active (battle tests the bit once per attack).
     * Slots beyond STATUS_ROUND_SLOTS can't occur (wrapper guards). */
    g_status_frozen_mask &= (uint8_t)~(1 << actor_slot);

    for (i = 0; i < slots->count; ) {
        StatusInstance *inst = &slots->slot[i];
        const StatusDefinition *def = status_def(inst->id);
        if (def != (const StatusDefinition *)0) {
            /* FLAT tick (docs/combo-system.md Phase C decision): poison
             * deals def->tick HP per round regardless of stacks; extra
             * applications only refresh duration and deepen stacks for
             * telemetry. */
            damage += def->tick;
        }
        inst->duration--;
        if (inst->duration == 0) {
            if (expired < MAX_STATUSES_PER_COMBATANT) {
                g_status_tick.expired_ids[expired++] = inst->id;
            }
            /* Remove by shifting the tail down, field-wise: struct
             * assignment lowers to __memcpy, which lives in the fixed
             * bank and is unreachable while bank 3 is mapped.  An
             * expiring FREEZE must NOT re-arm the frozen-mask bit: it
             * armed at the top of the round, and the expiry consumes the
             * status, so the combatant resumes acting next round. */
            slots->slot[i].id = slots->slot[slots->count - 1].id;
            slots->slot[i].stacks = slots->slot[slots->count - 1].stacks;
            slots->slot[i].duration = slots->slot[slots->count - 1].duration;
            slots->count--;
        } else {
            if (def != (const StatusDefinition *)0 && inst->id == STATUS_FREEZE) {
                /* Still frozen after this round: the combatant skips its
                 * attack again; the mask bit is tested (never consumed)
                 * by the battle gates. */
                g_status_frozen_mask |= (uint8_t)(1 << actor_slot);
            }
            i++;
        }
    }

    g_status_tick.damage = damage;
    g_status_tick.expired_count = expired;
}

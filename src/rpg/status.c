#include "status.h"
#include "banked.h"
#include "telemetry.h"

/* Fixed-bank wrappers around the bank-2 bodies (src/rpg/status_content.c),
 * keeping the stacking/ticking logic out of the fixed-bank budget.  The
 * trampoline exposes two pointers + two bytes; the apply call packs
 * stacks/duration into byte_b (both <= 15).  Telemetry is emitted HERE,
 * after the banked body returns -- banked code cannot emit events.
 *
 * Both bodies report through small shared WRAM structs (below); battles
 * resolve sequentially, so single slots are safe. */

/* Written by status_apply_banked(): what landed on the actor. */
StatusInstance g_status_applied;

/* Written by status_tick_banked(); declared in status.h. */
StatusTickResult g_status_tick;

/* Battle-scoped storage (see status.h).  Zeroed at battle start. */
static StatusSlots s_battle_status[STATUS_ROUND_SLOTS];

void status_reset_battle(void)
{
    uint8_t i;
    for (i = 0; i < STATUS_ROUND_SLOTS; i++) {
        s_battle_status[i].count = 0;
    }
}

StatusSlots *status_slots(uint8_t actor_slot)
{
    if (actor_slot >= STATUS_ROUND_SLOTS) return (StatusSlots *)0;
    return &s_battle_status[actor_slot];
}

bool status_apply(StatusSlots *slots, uint8_t id, uint8_t stacks,
                  uint8_t duration)
{
    if (stacks > 15) stacks = 15;
    if (duration > 15) duration = 15;

    g_status_applied.id = STATUS_NONE;

    g_bk_call_bank = 2;
    g_bk_call_target = (uint16_t)&status_apply_banked;
    g_bk_ptr_a = (void *)slots;
    g_bk_byte_a = id;
    g_bk_byte_b = (uint8_t)((stacks << 4) | duration);
    banked_call_run();

    if (g_status_applied.id == STATUS_NONE) {
        return false;
    }
    telemetry_emit(EVENT_STATUS_APPLIED, g_status_applied.id,
                   g_status_applied.stacks, g_status_applied.duration, 0);
    return true;
}

uint8_t status_tick(StatusSlots *slots, uint8_t actor_slot)
{
    g_bk_call_bank = 2;
    g_bk_call_target = (uint16_t)&status_tick_banked;
    g_bk_ptr_a = (void *)slots;
    g_bk_byte_a = actor_slot;
    banked_call_run();

    /* One combined event: damage taken + expiry summary (d2=count,
     * d3=first expired id).  Only one instance can expire per tick while
     * the definition table has a single ticking status. */
    if (g_status_tick.damage != 0 || g_status_tick.expired_count != 0) {
        telemetry_emit(EVENT_STATUS_TICKED, g_status_tick.damage,
                       actor_slot, g_status_tick.expired_count,
                       g_status_tick.expired_ids[0]);
    }
    return g_status_tick.damage;
}

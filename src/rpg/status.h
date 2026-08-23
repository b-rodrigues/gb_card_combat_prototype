#ifndef RPG_STATUS_H
#define RPG_STATUS_H

#include <stdint.h>
#include <stdbool.h>

/* ── Status system foundation (docs/combo-system.md Phase C, §12-§21) ──
 * Three distinct concepts, per plan §12:
 *   StatusDefinition -- what a status IS (bank-2 table, status_content.c)
 *   StatusInstance   -- a status currently affecting one combatant
 *                       (runtime Battle state, never GameState/save)
 *   Application      -- the event of trying to inflict one (battle flow
 *                       requests it; this module owns stacking/duration)
 *
 * The combo/effect pipeline never touches statuses directly: cards carry
 * a (status_id, chance) pair as DATA; battle rolls the die through the
 * deterministic RNG (§17/§18) and calls status_apply(). */

typedef enum {
    STATUS_NONE = 0,
    STATUS_POISON = 1
} StatusId;

/* Instances per combatant.  Small fixed budget: statuses are rare, and
 * Combatant embeds a StatusSlots (player + up to MAX_BATTLE_ENEMIES). */
#define MAX_STATUSES_PER_COMBATANT 2

/* One active affliction/blessing on one combatant. */
typedef struct {
    uint8_t id;        /* StatusId */
    uint8_t stacks;    /* 1..max_stacks (definition) */
    uint8_t duration;  /* turns remaining */
} StatusInstance;

typedef struct {
    StatusInstance slot[MAX_STATUSES_PER_COMBATANT];
    uint8_t count;
} StatusSlots;

/* Definition table geometry (rows live in src/rpg/status_content.c).
 * Kept minimal by design (plan §26 leaves balance open). */
#define STATUS_DEF_COUNT 2

typedef struct {
    uint8_t id;              /* StatusId */
    uint8_t tick;            /* FLAT damage per round (Phase C decision);
                              * stacks only refresh duration/telemetry */
    uint8_t max_stacks;      /* stacking cap (plan §16 STACK rule) */
    uint8_t duration;        /* default duration when applying */
} StatusDefinition;

/* Outcome of one tick, written to shared WRAM by the banked body and
 * consumed/emitted by the fixed wrapper. */
typedef struct {
    uint8_t damage;                    /* total tick damage this actor took */
    uint8_t expired_count;             /* instances removed this tick */
    uint8_t expired_ids[MAX_STATUSES_PER_COMBATANT];
} StatusTickResult;

extern StatusTickResult g_status_tick;

/* What the last apply landed on the actor (STATUS_NONE when nothing did);
 * shared WRAM slot written by the banked body, read by the wrapper. */
extern StatusInstance g_status_applied;

/* Battle-scoped status storage, owned by this module (NOT by Game/Battle):
 * growing the battle structs would shift every later field past SDCC's
 * 8-bit struct-offset encodings and bloat every object touching them.
 * Slots: 0 = player, 1..n = enemy index.  Runtime battle state only --
 * never part of GameState/save (docs/combo-system.md §20); reset at
 * battle_start via status_reset_battle(). */
#define STATUS_ROUND_SLOTS 4

void status_reset_battle(void);
StatusSlots *status_slots(uint8_t actor_slot);

/* Apply a status to an actor's slots (STACK rule: cap at max_stacks,
 * refresh duration).  stacks==0 means 1; duration==0 means the
 * definition's default.  Emits STATUS_APPLIED when a new instance or new
 * stack lands; returns true when the actor is now afflicted.
 *
 * Fixed-bank wrapper around the ROM-bank-2 body; see banked.h. */
bool status_apply(StatusSlots *slots, uint8_t id, uint8_t stacks,
                  uint8_t duration);

/* Bank-2 body dispatched by status_apply(). */
void status_apply_banked(void);

/* Run one end-of-turn tick over an actor's slots: accumulate tick damage
 * (per stack), decrement durations, expire finished instances.  Returns
 * total damage taken (caller applies it to HP and checks deaths).
 * Emits STATUS_TICKED and one STATUS_EXPIRED per removed instance.
 * actor_slot: 0 = player, 1..n = enemy index (telemetry context only). */
uint8_t status_tick(StatusSlots *slots, uint8_t actor_slot);

/* Bank-2 body dispatched by status_tick(). */
void status_tick_banked(void);

#endif /* RPG_STATUS_H */

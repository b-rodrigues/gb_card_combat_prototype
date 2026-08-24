#ifndef RPG_LOOT_H
#define RPG_LOOT_H

#include <stdint.h>
#include <stdbool.h>
#include "rpg/cards.h"

/* ── Procedural loot: material x effect x weapon (docs/loot.md §34) ──
 * Every loot combination maps to a derived CardId; card_get_def()
 * synthesizes its definition from the tables in loot_banked.c.  The
 * existing count-based collection/deck hold derived ids like any other
 * card. */

#define LOOT_ID_BASE 0x80          /* above CARD_FIRST_GAME content ids */

/* Axis counts are POWERS OF TWO: id-decode uses shifts/masks because
 * banked code cannot call the SDCC div/mod library (AGENTS.md 52.11.1).
 * Spare rows are reserved namespace for future growth. */
#define LOOT_NMATERIALS 4          /* WOOD BRONZE IRON MYTHRIL */
#define LOOT_NEFFECTS   8          /* PLAIN POISON FIRE ICE HEALING + rsv */
#define LOOT_NWEAPONS   8          /* SWORD SHIELD BOW DAGGER RING + rsv */

enum {
    MAT_WOOD = 0, MAT_BRONZE, MAT_IRON, MAT_MYTHRIL
};

enum {
    EFF_PLAIN = 0, EFF_POISON, EFF_FIRE_RESV, EFF_ICE_RESV, EFF_HEALING
};

enum {
    WPN_SWORD = 0, WPN_SHIELD, WPN_BOW, WPN_DAGGER, WPN_RING
};

/* Derived-id helpers: pure arithmetic, safe as macros (no code emitted). */
#define loot_is_loot_id(id)          ((id) >= LOOT_ID_BASE)
#define loot_id_material(id)         ((uint8_t)(((id) - LOOT_ID_BASE) >> 6))
#define loot_id_effect(id)           (((id) >> 3) & 7)
#define loot_id_weapon(id)           ((id) & 7)

/* Shared WRAM results (defined in src/rpg/loot.c):
 * g_loot_id       -- derived-id result byte written by the bank-2
 *                    drop/synth bodies, read by fixed-bank wrappers.
 * g_loot_rng_state -- isolated xorshift stream (seeded from the main
 *                    seed by rng_set_seed); bank-2 bodies step it
 *                    inline -- a banked body must never call a
 *                    fixed-bank function (AGENTS.md 52.11.1). */
extern uint8_t g_loot_id;
extern uint16_t g_loot_rng_state;

/* Bank-2 bodies:
 * game_loot_drop_banked   -- victory gate + profile pick + roll
 *                            (src/game/loot_drop_banked.c), dispatched
 *                            by game_loot_drop() in src/game/content.c.
 * loot_synth_banked       -- fills g_card_scratch for a derived id,
 *                            dispatched by card_get_def(). */
void game_loot_drop_banked(void);
void loot_synth_banked(void);

#endif /* RPG_LOOT_H */

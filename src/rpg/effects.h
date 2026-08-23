#ifndef RPG_EFFECTS_H
#define RPG_EFFECTS_H

#include <stdint.h>
#include "rpg/cards.h"
#include "combo.h"

/* ── Effect resolution layer (docs/combo-system.md Phase B) ───────────
 * Combos describe the QUALITY of a card selection (ComboResult); effects
 * describe the CONSEQUENCE (EffectResult).  This module is the only place
 * that turns one into the other -- battle flow applies the result but
 * never computes magnitudes, and the combo evaluator never sees effects.
 *
 * The effect vocabulary is the existing engine-generic CardEffectType
 * (rpg/cards.h): DAMAGE_TARGET / BLOCK_DAMAGE / HEAL_HP correspond to the
 * plan's EFFECT_DAMAGE / EFFECT_BLOCK / EFFECT_HEAL.  New effect kinds
 * (status, draw, ...) extend that enum without touching the combo side.
 *
 * Dispatch note: the scaling body lives in ROM bank 2 next to the combo
 * evaluator, and combo_resolve() runs BOTH in one banked dispatch --
 * the bank-2 combo body calls effect_resolve_into() bank-locally (same
 * bank, no trampoline).  This keeps the fixed-bank cost of the seam to
 * one wrapper (the fixed bank is completely full; see make memmap).
 * A standalone fixed-entry effect_resolve() can be added when a second
 * caller appears.
 *
 * Statuses are deliberately NOT implemented yet (plan §2); the seam they
 * will plug into is the per-effect switch in the banked body. */

/* Outcome of resolving one played hand.  Magnitudes only -- never combo
 * internals, never persistent state. */
typedef struct {
    uint8_t type;    /* CardEffectType actually resolved */
    uint8_t amount;  /* Scaled magnitude (damage / block / heal points) */
} EffectResult;

/* Result of the most recent combo_resolve().  Single slot by design:
 * combat resolution is sequential.  Read via effect_last() immediately
 * after the call. */
extern EffectResult g_effect_last;
const EffectResult *effect_last(void);

/* Bank-2 scaling body, called bank-locally by combo_resolve_banked().
 * Self-contained: pure function of (effect, combo).  Not for fixed-bank
 * callers (bank 2 is not mapped there). */
void effect_resolve_into(uint8_t effect_type, const ComboResult *combo,
                         EffectResult *out);

#endif /* RPG_EFFECTS_H */

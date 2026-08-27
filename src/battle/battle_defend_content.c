#pragma bank 3

#include "battle.h"
#include "rpg/effects.h"
#include "banked.h"

/* Banked body of battle_defend_resolve (src/battle/battle.c).  Computes
 * and applies the DEFENSE net resolution entirely in bank 3 so the
 * fixed bank (which is completely full, see make memmap) only runs a thin
 * staging wrapper + telemetry.
 *
 * Self-contained (AGENTS.md §52.11.1): reads/writes only WRAM structs
 * (the staged Battle* and g_effect_last) and its own bank-local code; it
 * must NOT call any fixed-bank function (the bank is switched away).  The
 * block sum rides in g_effect_last.amount (set by the bank-3 resolver),
 * the ring flag is scanned from last_combo.cards[] here.
 *
 * Defense over-block rule (docs/loot.md §34.4): a defense net
 * <= 0 is CLAMPED to zero (no damage); HP is restored only when a ring
 * (healing shield) is in the defense combo, capped at max HP.
 *
 * Damage path replicates combatant_take_damage() inline (the banked body
 * may not call the fixed-bank helper per §52.11.1): the helper is only
 * `hp = (damage >= hp) ? 0 : hp - damage` -- a pure HP clamp with no death
 * flag / status / invuln side effects, so the inline copy is equivalent.
 * The hero's HP==0 defeat check that follows in battle_execute_combo still
 * relies on this writing hp to 0 on an overkill hit.
 *
 * Telemetry: g_bk_byte_a reports the HP ACTUALLY lost (clamped to current
 * HP), not the nominal `net` -- on an overkill hit it is the remaining HP
 * (possibly 0), a deliberate change from the old nominal-net reporting
 * (the two agree whenever the hit does not overkill; the clamped value is
 * the observable damage).
 *
 * On return it stages the two possible magnitudes into g_bk_byte_a
 * (net damage taken) and g_bk_byte_b (net HP healed); the fixed wrapper
 * emits the corresponding telemetry. */
void battle_defend_resolve_banked(void)
{
    Battle *b = (Battle *)g_bk_ptr_a;
    int16_t net;
    uint8_t di;
    uint8_t combo_has_ring = 0;

    g_bk_byte_a = 0;
    g_bk_byte_b = 0;
    if (!b) return;

    for (di = 0; di < b->combo_count; di++) {
        if (b->last_combo.cards[di].ring) combo_has_ring = 1;
    }
    net = (int16_t)b->enemy_incoming_dmg - (int16_t)g_effect_last.amount;
    if (net > 0) {
        uint8_t dmg = (uint8_t)net;
        if (dmg >= b->player.hp) {
            g_bk_byte_a = b->player.hp;
            b->player.hp = 0;
        } else {
            g_bk_byte_a = dmg;
            b->player.hp = (uint8_t)(b->player.hp - dmg);
        }
    } else if (net < 0 && combo_has_ring) {
        /* net < 0: the unsigned trick is safe -- subtracting a negative
         * `net` ADDS its magnitude; capped at max_hp. */
        uint16_t nh = (uint16_t)b->player.hp - net;
        if (nh > b->player.max_hp) nh = b->player.max_hp;
        g_bk_byte_b = (uint8_t)(nh - (uint16_t)b->player.hp);
        b->player.hp = (uint8_t)nh;
    }
}

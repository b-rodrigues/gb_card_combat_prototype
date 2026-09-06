#pragma bank 4
#pragma disable_warning 110

#include "battle.h"
#include "battle_data.h"
#include "world/actor.h"
#include "banked.h"

/* Banked body (bank 4): load the active battle screen's HUD layout into
 * the WRAM g_battle_hud cache.  Dispatched via banked_call_run() from
 * battle_start().  Reads g_bk_byte_a (battle type id), resolves the
 * screen ("boss" for BATTLE_NONE, else "default"), and copies its
 * positions + music ticks + HUD rows (BattleScreenDef offsets 6..41)
 * into g_battle_hud.
 *
 * Everything is read directly: g_battle_screens lives in this same bank
 * (no banked_copy -- that would restore the home bank mid-body and crash
 * on return), and g_battle_hud / g_bk_byte_a are WRAM (always mapped).
 * Fully self-contained: no fixed-bank calls (banked.h ABI contract). */
void battle_hud_load_banked(void)
{
    uint8_t battle_type = g_bk_byte_a;
    const char *want;
    const BattleScreenDef *def;
    const BattleScreenDef *dflt;
    uint8_t i, k;
    uint8_t match;
    uint8_t *dst;
    const uint8_t *src;

    want = (battle_type == BATTLE_NONE) ? "boss" : "default";
    def = 0;
    dflt = 0;
    for (i = 0; i < g_battle_screen_count; i++) {
        const BattleScreenDef *d = g_battle_screens[i];
        match = 1;
        for (k = 0; ; k++) {
            if (d->id[k] != want[k]) {
                match = 0;
                break;
            }
            if (want[k] == '\0') break;
        }
        if (match) {
            def = d;
            break;
        }
        match = 1;
        for (k = 0; ; k++) {
            if (d->id[k] != "default"[k]) {
                match = 0;
                break;
            }
            if ("default"[k] == '\0') break;
        }
        if (match) dflt = d;
    }
    if (def == 0) def = dflt;
    if (def == 0) return;
    dst = (uint8_t *)&g_battle_hud;
    src = (const uint8_t *)&def->enemy_positions;
    for (i = 0; i < 36; i++) {
        dst[i] = src[i];
    }
}

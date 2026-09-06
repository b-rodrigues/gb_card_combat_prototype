#pragma bank 4
#pragma disable_warning 110

#include "battle.h"
#include "battle_data.h"
#include "world/actor.h"
#include "banked.h"

/* Footprint contract (C89 file-scope assert: a negative array size is a
 * compile error): the cache stays 36 bytes of uint8_t (no padding, no
 * widened types).  Field ORDER needs no assertion: the copy below and
 * the renderer both use named fields, so a reorder on either side stays
 * correct by construction, and a rename fails to compile.  The order
 * that still matters is BattleScreenDef vs the compiler's positional
 * emitter -- enforced host-side by check_battle_struct_order(). */
typedef char battle_hud_cache_size_ok[(sizeof(BattleHudCache) == 36) ? 1 : -1];

/* Banked body (bank 4): load the active battle screen's HUD layout into
 * the WRAM g_battle_hud cache.  Dispatched via banked_call_run() from
 * battle_start().  Reads g_bk_byte_a (battle type id), resolves the
 * screen ("boss" for BATTLE_NONE, else "default"), and stages its
 * positions + music ticks + HUD rows field-by-field (names match
 * BattleScreenDef one-to-one; the file-scope asserts above enforce it).
 *
 * Everything is read directly: g_battle_screens lives in this same bank
 * (no banked_copy -- that would restore the home bank mid-body and crash
 * on return), and g_battle_hud / g_bk_byte_a are WRAM (always mapped).
 * Fully self-contained: no fixed-bank calls (banked.h ABI contract).
 * No struct assignment: banked bodies must not rely on compiler memcpys
 * across banks. */
void battle_hud_load_banked(void)
{
    uint8_t battle_type = g_bk_byte_a;
    const char *want;
    const BattleScreenDef *def;
    const BattleScreenDef *dflt;
    uint8_t i, k;
    uint8_t match;

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
    for (k = 0; k < 3; k++) {
        for (i = 0; i < 2; i++) {
            g_battle_hud.enemy_positions[k][i] = def->enemy_positions[k][i];
        }
    }
    g_battle_hud.timer_overworld_ticks = def->timer_overworld_ticks;
    g_battle_hud.timer_battle_ticks = def->timer_battle_ticks;
    g_battle_hud.turn_banner_row = def->turn_banner_row;
    g_battle_hud.enemy_hp_row = def->enemy_hp_row;
    g_battle_hud.enemy_sprite_row = def->enemy_sprite_row;
    g_battle_hud.enemy_cursor_row = def->enemy_cursor_row;
    g_battle_hud.enemy_col_start = def->enemy_col_start;
    g_battle_hud.enemy_col_step = def->enemy_col_step;
    g_battle_hud.hero_label_row = def->hero_label_row;
    g_battle_hud.hero_label_col = def->hero_label_col;
    g_battle_hud.hero_hp_row = def->hero_hp_row;
    g_battle_hud.hero_hp_col = def->hero_hp_col;
    g_battle_hud.deck_row = def->deck_row;
    g_battle_hud.deck_col = def->deck_col;
    g_battle_hud.ap_row = def->ap_row;
    g_battle_hud.ap_col = def->ap_col;
    g_battle_hud.combo_row = def->combo_row;
    g_battle_hud.cards_row = def->cards_row;
    g_battle_hud.card_cursor_row = def->card_cursor_row;
    g_battle_hud.card_desc_row = def->card_desc_row;
    g_battle_hud.timer_row = def->timer_row;
    g_battle_hud.timer_col = def->timer_col;
    g_battle_hud.timer_width = def->timer_width;
    g_battle_hud.hud_enemy_row_start = def->hud_enemy_row_start;
    g_battle_hud.hud_enemy_row_step = def->hud_enemy_row_step;
    g_battle_hud.hud_deck_row = def->hud_deck_row;
    g_battle_hud.hud_combo_row_start = def->hud_combo_row_start;
    g_battle_hud.hud_combo_row_step = def->hud_combo_row_step;
    g_battle_hud.hud_timer_row = def->hud_timer_row;
    g_battle_hud.hud_caret_x = def->hud_caret_x;
}

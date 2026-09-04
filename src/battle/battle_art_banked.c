#pragma bank 4
#pragma disable_warning 110

#include "battle.h"
#include "battle_data.h"
#include "actor.h"
#include "banked.h"
#include "gfx/battle_enemy_art.h"
#include <gb/gb.h>

/* Battle art loader + resolver (ROM bank 4).  Runs once per battle entry
 * from ui_draw_battle_full() (LCD-off window) via the WRAM trampoline with
 * g_bk_ptr_a = Battle*.  Self-contained: reads the enemy-type tables and
 * the art header directly (same bank), writes VRAM with a hand loop (never
 * calls fixed-bank lib functions while the content bank is selected), and
 * caches per-enemy art in the WRAM globals for the bank-3 stamper.
 *
 * Art selection stays data-driven: type_for_battle() only picks WHICH
 * enemy-type row (engine BattleIds, mirroring enemy_deck_content.c); the
 * art_index/art_frames come from the compiled enemy-type row, so editing
 * an enemy's sprite JSON changes the ROM result.
 * Trios share their base type's art; BATTLE_NONE combat means the boss.
 * Unknown battle ids resolve to 0xFF (text fallback, blank art rows).
 *
 * VRAM layout: slot k loads 12 tiles (2 frames x 3x2) at BG base
 * 128 + 12*k (BATTLE_ART_VRAM_BASE, kept in sync with the stamper in
 * ui_battle_content.c).  World tiles normally living at 128+ are
 * stale-unused during battle and reloaded by ui_load_tileset() on the
 * overworld return path. */

#define BATTLE_ART_VRAM_BASE 128u
#define BATTLE_ART_SLOT_TILES 12u
#define BATTLE_ART_SET_TILES 12u
#define BATTLE_ART_SET_BYTES 192u

static const char *type_for_battle(uint8_t battle_id)
{
    if (battle_id == BATTLE_BAT) return "bat";
    if (battle_id == BATTLE_NONE) return "slime_lord";
    return "slime";
}

static uint8_t streq(const char *a, const char *b)
{
    uint8_t i = 0;
    while (1) {
        if (a[i] != b[i]) return 0;
        if (a[i] == 0) return 1;
        i++;
    }
}

void battle_art_load_banked(void)
{
    const volatile Battle *b = (const Battle *)g_bk_ptr_a;
    const char *want;
    uint8_t art_index = 0xFF;
    uint8_t art_frames = 0;
    uint8_t art_palette = 0;
    uint8_t i, k;
    uint16_t n;
    const uint8_t *src;
    volatile uint8_t *dst;

    if (!b) return;

    want = type_for_battle(b->enemy_battle_id);
    for (i = 0; i < g_enemy_type_count; i++) {
        const EnemyTypeDef *t = g_enemy_types[i];
        if (streq(want, t->id)) {
            art_index = t->art_index;
            art_frames = t->art_frames;
            art_palette = t->art_palette;
            break;
        }
    }

    VBK_REG = 0;
    for (k = 0; k < b->enemy_count && k < MAX_BATTLE_ENEMIES; k++) {
        g_battle_enemy_art[k] = art_index;
        g_battle_enemy_art_frames[k] = art_frames;
        g_battle_enemy_art_pal[k] = art_palette;
        if (art_index == 0xFF) continue;
        src = &battle_enemy_art[(uint16_t)art_index * BATTLE_ART_SET_BYTES];
        dst = (volatile uint8_t *)(0x8000u + (uint16_t)((uint8_t)(BATTLE_ART_VRAM_BASE + k * BATTLE_ART_SLOT_TILES) << 4));
        n = BATTLE_ART_SET_BYTES;
        while (n--) {
            *dst++ = *src++;
        }
    }
    for (; k < MAX_BATTLE_ENEMIES; k++) {
        g_battle_enemy_art[k] = 0xFF;
        g_battle_enemy_art_frames[k] = 0;
        g_battle_enemy_art_pal[k] = 0;
    }
}

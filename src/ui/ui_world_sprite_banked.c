#pragma bank 3

#include <stdint.h>
#include "world.h"
#include "actor.h"
#include "ui.h"
#include "banked.h"
#include "gfx/rpg_tile_lookup.h"

/* ── Overworld actor sprites + castle boss background (bank-3 body) ──
 * The data-driven SPRITE_KIND_* OAM choice, the per-actor shadow-OAM write
 * (position + tile + prop) and the castle-boss 2x2 background-tile draw live
 * in bank 3 (like ui_color_banked) so the fixed-bank ui.c stays under
 * 0x8000 (AGENTS.md §52.18 / §55.5).  ui_draw_actors_sprites() dispatches
 * this one banked pass per frame via the staging globals; the caller only
 * stages the World pointer and reads nothing back (shadow OAM 0xC000 is
 * WRAM, always mapped). */

#ifdef DEBUG_BUILD
extern uint8_t g_tilemap_mirror[32 * 32];
#endif

/* Bat OAM sprite tile data (2 frames), shared by the desolate and castle
 * tileset variants (identical pixel art).  Lives in bank 3 so the fixed
 * bank's _CODE does not carry 64 bytes of sprite data; ui_init() copies it
 * into WRAM via banked_copy and set_sprite_data()s both tile IDs. */
const uint8_t s_bat_tiles[32] = {
    0x00, 0x00, 0xA0, 0xA0, 0x4A, 0x4A, 0x04, 0x04,
    0x00, 0x00, 0x28, 0x28, 0x10, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x0A, 0x0A, 0xA4, 0xA4, 0x40, 0x40,
    0x00, 0x00, 0x00, 0x00, 0x28, 0x28, 0x10, 0x10
};

/* Shadow OAM WRAM base (GBDK's global `shadow_OAM` at 0xC000).  Each entry
 * is 4 bytes: y, x, tile, attr.  Written directly (WRAM is always mapped)
 * so the fixed-bank caller need not loop over actors. */
#define SHADOW_OAM_BASE 0xC000u
#define OAM_SLOT_ACTOR0 1u  /* slot 0 -> OAM entry 1 (entry 0 is the player) */
#define OAM_SLOT_STATIC0 (OAM_SLOT_ACTOR0 + MAX_WORLD_ACTORS)

/* Sub-tile position of an actor mid-step (replica of world_actor_px/py,
 * see src/world/px_banked.c).  bank-3 bodies must not call fixed-bank code. */
static uint8_t spr_axis_px(const WorldActorRuntime *a, uint8_t axis)
{
    uint8_t ax = (axis == 0) ? a->x : a->y;
    uint8_t tx = (axis == 0) ? a->move_target_x : a->move_target_y;
    uint8_t pr = a->move_progress;
    uint8_t px = (uint8_t)(ax << 3);
    if (a->move_state) {
        if (tx > ax) return (uint8_t)(px + pr);
        if (tx < ax) return (uint8_t)(px - pr);
    }
    return px;
}

/* SPRITE_KIND_* -> OAM tile/prop.  Single source for both loops below;
 * the chest shares the kobold/bat convention (CHEST_SPRITE_TILE_ID holds
 * the 1-frame art in both anim slots, so + anim stays uniform). */
static uint8_t sprite_tile_for(uint8_t kind, uint8_t visual, uint8_t castle,
                               uint8_t anim, uint8_t *prop)
{
    switch (kind) {
        case SPRITE_KIND_KOBOLD:
            *prop = 1;
            return (uint8_t)(KOBOLD_SPRITE_TILE_ID + anim);
        case SPRITE_KIND_BAT:
            *prop = 1;
            return (uint8_t)((castle ? BAT_CASTLE_SPRITE_TILE_ID
                                     : BAT_DESOLATE_SPRITE_TILE_ID) + anim);
        case SPRITE_KIND_CHEST:
            *prop = 1;
            return (uint8_t)(CHEST_SPRITE_TILE_ID + anim);
        case SPRITE_KIND_BOSS:
            *prop = 0;
            return 0;
        default: /* SPRITE_KIND_ASCII */
            *prop = 0;
            return (uint8_t)(ui_font_tile_base + (uint8_t)(visual - ' '));
    }
}

/* Compute the OAM tile/prop/position for every active non-boss actor (from
 * its SPRITE_KIND_* and sub-tile position) and write it straight into shadow
 * OAM; static (non-hostile) actors with sprite art follow in the entries
 * after the hostile slots, while ASCII-kind statics keep their background
 * glyph (see ui_draw_world_cell) and stay hidden here.  The castle boss is
 * drawn into the background tilemap as a 2x2 block instead.
 * Args: g_bk_ptr_a = const World * (WRAM), g_bk_byte_a = castle flag,
 * g_bk_byte_b = anim_step.  Inactive and SPRITE_KIND_BOSS actors get their
 * OAM entry hidden (y=0). */
void ui_actors_sprites_banked(void)
{
    const World *w = (const World *)g_bk_ptr_a;
    uint8_t castle = g_bk_byte_a;
    uint8_t anim = g_bk_byte_b;
    uint8_t slot;
    uint8_t i;

    if (!w) return;
    for (slot = 0; slot < MAX_WORLD_ACTORS; slot++) {
        const WorldActorRuntime *a = &w->actors[slot];
        volatile uint8_t *e = (volatile uint8_t *)(SHADOW_OAM_BASE +
                                                   ((OAM_SLOT_ACTOR0 + slot) << 2));
        uint8_t tile = 0;
        uint8_t prop = 0;
        uint8_t px, py;
        if (a->active) {
            tile = sprite_tile_for((uint8_t)a->sprite_kind, a->visual,
                                   castle, anim, &prop);
        }
        if (tile) {
            px = (uint8_t)(spr_axis_px(a, 0) - w->camera_px_x);
            py = (uint8_t)(spr_axis_px(a, 1) - w->camera_px_y);
            if (px < 160 && py < 144 && (int)a->sprite_kind != SPRITE_KIND_BOSS) {
                e[0] = (uint8_t)(py + 16);
                e[1] = (uint8_t)(px + 8);
                e[2] = tile;
                e[3] = prop;
                continue;
            }
        }
        e[0] = 0;  /* hidden */
    }
    for (i = 0; i < MAX_STATIC_ACTORS; i++) {
        const WorldActorDefinition *d;
        volatile uint8_t *e = (volatile uint8_t *)(SHADOW_OAM_BASE +
                                                   ((OAM_SLOT_STATIC0 + i) << 2));
        uint8_t tile;
        uint8_t prop = 0;
        uint8_t px, py;
        if (i >= g_static_actor_count) {
            e[0] = 0;  /* no static here: hide any stale sprite */
            continue;
        }
        d = &g_static_actors[i];
        if (d->sprite_kind == SPRITE_KIND_ASCII ||
            d->sprite_kind == SPRITE_KIND_BOSS) {
            e[0] = 0;  /* glyph path / boss block own these */
            continue;
        }
        tile = sprite_tile_for((uint8_t)d->sprite_kind, d->visual,
                               castle, anim, &prop);
        if (!tile) {
            e[0] = 0;
            continue;
        }
        px = (uint8_t)((uint8_t)(d->x << 3) - w->camera_px_x);
        py = (uint8_t)((uint8_t)(d->y << 3) - w->camera_px_y);
        if (px < 160 && py < 144) {
            e[0] = (uint8_t)(py + 16);
            e[1] = (uint8_t)(px + 8);
            e[2] = tile;
            e[3] = prop;
            continue;
        }
        e[0] = 0;  /* hidden */
    }
    /* Castle boss is drawn directly into the background tilemap as a 2x2
     * block (castle tile indexes 7,8,16,17), not an OAM sprite. */
    if (castle) {
        for (slot = 0; slot < MAX_WORLD_ACTORS; slot++) {
            const WorldActorRuntime *a = &w->actors[slot];
            uint8_t ox, oy;
            volatile uint8_t *tilemap = (volatile uint8_t *)0x9800;
            if (!a->active) continue;
            if ((int)a->sprite_kind != SPRITE_KIND_BOSS) continue;
            ox = a->x;
            oy = a->y;
            {
                uint8_t sx = (uint8_t)(ox - w->scroll_x);
                uint8_t sy = (uint8_t)(oy - w->scroll_y);
                tilemap[(oy & 31) * 32 + (ox & 31)] = (uint8_t)(RPG_TILE_BASE_WORLD + 7);
                tilemap[(oy & 31) * 32 + ((ox + 1) & 31)] = (uint8_t)(RPG_TILE_BASE_WORLD + 8);
                tilemap[((oy + 1) & 31) * 32 + (ox & 31)] = (uint8_t)(RPG_TILE_BASE_WORLD + 16);
                tilemap[((oy + 1) & 31) * 32 + ((ox + 1) & 31)] = (uint8_t)(RPG_TILE_BASE_WORLD + 17);
#ifdef DEBUG_BUILD
                g_tilemap_mirror[(oy & 31) * 32 + (ox & 31)] = (uint8_t)(RPG_TILE_BASE_WORLD + 7);
                g_tilemap_mirror[(oy & 31) * 32 + ((ox + 1) & 31)] = (uint8_t)(RPG_TILE_BASE_WORLD + 8);
                g_tilemap_mirror[((oy + 1) & 31) * 32 + (ox & 31)] = (uint8_t)(RPG_TILE_BASE_WORLD + 16);
                g_tilemap_mirror[((oy + 1) & 31) * 32 + ((ox + 1) & 31)] = (uint8_t)(RPG_TILE_BASE_WORLD + 17);
                if (sx < (uint8_t)(WORLD_VIEW_W - 1) && sy < (uint8_t)(WORLD_VIEW_H - 1)) {
                    g_ui_screen_buf[sy][sx] = (char)a->visual;
                    g_ui_screen_buf[sy][sx + 1] = (char)a->visual;
                    g_ui_screen_buf[sy + 1][sx] = (char)a->visual;
                    g_ui_screen_buf[sy + 1][sx + 1] = (char)a->visual;
                }
#endif
            }
            break;
        }
    }
}
#pragma bank 3

#include "world.h"
#include "actor.h"

extern uint8_t g_patrol_outcome;
extern uint8_t g_patrol_evt[4];
extern World *g_patrol_world;
extern uint8_t g_patrol_slot;

static const uint8_t s_patrol_circle[4] = { 0x36, 0x1A, 0x29, 0x05 };
static const uint8_t s_patrol_line[8]   = { 0x01, 0x15, 0x19, 0x05, 0x24, 0x35, 0x36, 0x25 };

void world_patrol_slot_banked(void)
{
    uint8_t *bp;
    uint8_t slot;
    uint8_t active_v, ai_type_v, ai_step_v, ai_timer_v;
    uint8_t move_state_v, move_progress_v, move_tx_v, move_ty_v;
    uint8_t spawn_x_v, spawn_y_v, x_v, y_v, facing_v, id_v;
    uint8_t entry, target_x, target_y;
    uint8_t blocked, i;
    uint8_t tile;

    g_patrol_outcome = 0;

    if (!g_patrol_world) return;
    slot = g_patrol_slot;
    if (slot >= MAX_WORLD_ACTORS) return;

    bp = (uint8_t *)&g_patrol_world->actors[slot];
    active_v = bp[3];
    if (!active_v) return;
    ai_type_v = bp[17];
    if (ai_type_v == 0) return;

    /* Read ALL fields through byte pointer arithmetic -- no struct
     * pointer, no SDCC pointer caching at all. */
    ai_step_v       = bp[18];
    ai_timer_v      = bp[19];
    move_state_v    = bp[20];
    move_tx_v       = bp[21];
    move_ty_v       = bp[22];
    move_progress_v = bp[23];
    spawn_x_v       = bp[15];
    spawn_y_v       = bp[16];
    x_v             = bp[4];
    y_v             = bp[5];
    facing_v        = bp[6];
    id_v            = bp[2];

    if (move_state_v) {
        move_progress_v++;
        if (move_progress_v >= 8) {
            x_v = move_tx_v;
            y_v = move_ty_v;
            move_state_v = 0;
            move_progress_v = 0;
            ai_timer_v = 32;
            g_patrol_evt[0] = id_v;
            g_patrol_evt[1] = x_v;
            g_patrol_evt[2] = y_v;
            g_patrol_evt[3] = facing_v;
            g_patrol_outcome = 1;
        }
        /* Write back through byte pointer -- no struct pointer at all. */
        bp[23] = move_progress_v;
        bp[20] = move_state_v;
        bp[19] = ai_timer_v;
        bp[4]  = x_v;
        bp[5]  = y_v;
        return;
    }

    if (ai_timer_v > 0) {
        ai_timer_v--;
        bp[19] = ai_timer_v;
        return;
    }

    entry = (ai_type_v == 1) ?
        s_patrol_circle[ai_step_v & 3] :
        s_patrol_line[ai_step_v & 7];
    facing_v  = (uint8_t)(entry >> 4);
    target_x = (uint8_t)(spawn_x_v + (entry & 3) - 1);
    target_y = (uint8_t)(spawn_y_v + ((entry >> 2) & 3) - 1);

    if (target_x == x_v && target_y == y_v) {
        bp[6]  = facing_v;
        bp[18] = (uint8_t)(ai_step_v + 1);
        bp[19] = 32;
        return;
    }

    blocked = 0;
    if (target_x >= g_patrol_world->width || target_y >= g_patrol_world->height) {
        blocked = 1;
    } else {
        tile = g_patrol_world->map[target_y][target_x];
        if (tile != 0) blocked = 1;
    }

    if (!blocked) {
        for (i = 0; i < MAX_WORLD_ACTORS; i++) {
            if (!g_patrol_world->actors[i].active) continue;
            if (g_patrol_world->actors[i].x == target_x &&
                g_patrol_world->actors[i].y == target_y) {
                blocked = 1; break;
            }
            if (g_patrol_world->actors[i].move_state &&
                g_patrol_world->actors[i].move_target_x == target_x &&
                g_patrol_world->actors[i].move_target_y == target_y) {
                blocked = 1; break;
            }
        }
    }

    if (!blocked) {
        for (i = 0; i < g_static_actor_count; i++) {
            if (g_static_actors[i].x == target_x &&
                g_static_actors[i].y == target_y) {
                blocked = 1; break;
            }
        }
    }

    if (blocked) {
        bp[19] = 32;
        return;
    }

    if (target_x == g_patrol_world->player.position.x &&
        target_y == g_patrol_world->player.position.y) {
        g_patrol_world->encounter_actor_index = slot;
        g_patrol_evt[0] = target_x;
        g_patrol_evt[1] = target_y;
        g_patrol_evt[2] = id_v;
        g_patrol_evt[3] = 0;
        g_patrol_outcome = 2;
        return;
    }

    bp[18] = (uint8_t)(ai_step_v + 1);
    bp[20] = 1;
    bp[21] = target_x;
    bp[22] = target_y;
    bp[23] = 0;
    bp[6]  = facing_v;
}

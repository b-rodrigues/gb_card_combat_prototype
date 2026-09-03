#pragma bank 3

#include "world.h"
#include "actor.h"

extern uint8_t g_patrol_outcome;
extern uint8_t g_patrol_evt[4];
extern World *g_patrol_world;
extern uint8_t g_patrol_slot;

static const uint8_t s_patrol_circle[4] = { 0x36, 0x1A, 0x29, 0x05 };
static const uint8_t s_patrol_line[8]   = { 0x01, 0x15, 0x19, 0x05, 0x24, 0x35, 0x36, 0x25 };

#define ACTOR_OFFSET(field) ((uint8_t)&(((WorldActorRuntime *)0)->field))

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
    active_v = bp[ACTOR_OFFSET(active)];
    if (!active_v) return;
    ai_type_v = bp[ACTOR_OFFSET(ai_type)];
    if (ai_type_v == AI_NONE) return;

    /* Read ALL fields through byte pointer arithmetic (via ACTOR_OFFSET)
     * -- no struct pointer, no SDCC pointer caching across branch joins. */
    ai_step_v       = bp[ACTOR_OFFSET(ai_step)];
    ai_timer_v      = bp[ACTOR_OFFSET(ai_timer)];
    move_state_v    = bp[ACTOR_OFFSET(move_state)];
    move_tx_v       = bp[ACTOR_OFFSET(move_target_x)];
    move_ty_v       = bp[ACTOR_OFFSET(move_target_y)];
    move_progress_v = bp[ACTOR_OFFSET(move_progress)];
    spawn_x_v       = bp[ACTOR_OFFSET(spawn_x)];
    spawn_y_v       = bp[ACTOR_OFFSET(spawn_y)];
    x_v             = bp[ACTOR_OFFSET(x)];
    y_v             = bp[ACTOR_OFFSET(y)];
    facing_v        = bp[ACTOR_OFFSET(facing)];
    id_v            = bp[ACTOR_OFFSET(id)];

    if (move_state_v) {
        move_progress_v++;
        if (move_progress_v >= MOVE_FRAMES) {
            x_v = move_tx_v;
            y_v = move_ty_v;
            move_state_v = MOVE_STATE_IDLE;
            move_progress_v = 0;
            ai_timer_v = PATROL_STEP_INTERVAL;
            g_patrol_evt[0] = id_v;
            g_patrol_evt[1] = x_v;
            g_patrol_evt[2] = y_v;
            g_patrol_evt[3] = facing_v;
            g_patrol_outcome = 1;
        }
        /* Write back through byte pointer -- no struct pointer at all. */
        bp[ACTOR_OFFSET(move_progress)] = move_progress_v;
        bp[ACTOR_OFFSET(move_state)]    = move_state_v;
        bp[ACTOR_OFFSET(ai_timer)]      = ai_timer_v;
        bp[ACTOR_OFFSET(x)]             = x_v;
        bp[ACTOR_OFFSET(y)]             = y_v;
        return;
    }

    if (ai_timer_v > 0) {
        ai_timer_v--;
        bp[ACTOR_OFFSET(ai_timer)] = ai_timer_v;
        return;
    }

    entry = (ai_type_v == AI_PATROL_CIRCLE) ?
        s_patrol_circle[ai_step_v & 3] :
        s_patrol_line[ai_step_v & 7];
    facing_v  = (uint8_t)(entry >> 4);
    target_x = (uint8_t)(spawn_x_v + (entry & 3) - 1);
    target_y = (uint8_t)(spawn_y_v + ((entry >> 2) & 3) - 1);

    if (target_x == x_v && target_y == y_v) {
        bp[ACTOR_OFFSET(facing)]   = facing_v;
        bp[ACTOR_OFFSET(ai_step)]  = (uint8_t)(ai_step_v + 1);
        bp[ACTOR_OFFSET(ai_timer)] = PATROL_STEP_INTERVAL;
        return;
    }

    blocked = 0;
    if (target_x >= g_patrol_world->width || target_y >= g_patrol_world->height) {
        blocked = 1;
    } else {
        tile = g_patrol_world->map[target_y][target_x];
        /* Walkable-set replica of world_is_walkable() (src/world/world.c) --
         * bank-3 bodies must not call fixed-bank code.  KEEP IN SYNC. */
        if (tile == TILE_FLOOR || tile == TILE_EXIT) {
            blocked = 0;
        } else if (tile >= TILE_DESOLATE_FLOOR_00 && tile <= TILE_DESOLATE_FLOOR_03) {
            blocked = 0;
        } else if (tile == TILE_DESOLATE_FLOOR_PLAIN || tile == TILE_DESOLATE_STAIRCASE) {
            blocked = 0;
        } else if (tile >= TILE_DESOLATE_LANDSCAPE_21 && tile <= TILE_DESOLATE_LANDSCAPE_24) {
            blocked = 0;
        } else if (tile == TILE_DESOLATE_LANDSCAPE_32 || tile == TILE_DESOLATE_LANDSCAPE_40) {
            blocked = 0;
        } else if (tile >= TILE_DESOLATE_LANDSCAPE_43 && tile <= TILE_DESOLATE_LANDSCAPE_47) {
            blocked = 0;
        } else {
            blocked = 1;
        }
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
        bp[ACTOR_OFFSET(ai_timer)] = PATROL_STEP_INTERVAL;
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

    bp[ACTOR_OFFSET(ai_step)]       = (uint8_t)(ai_step_v + 1);
    bp[ACTOR_OFFSET(move_state)]    = MOVE_STATE_MOVING;
    bp[ACTOR_OFFSET(move_target_x)] = target_x;
    bp[ACTOR_OFFSET(move_target_y)] = target_y;
    bp[ACTOR_OFFSET(move_progress)] = 0;
    bp[ACTOR_OFFSET(facing)]        = facing_v;
}

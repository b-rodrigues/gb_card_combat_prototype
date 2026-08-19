#include "telemetry.h"
#include "audio.h"
#include "screen.h"
#include "actor.h"
#include "banked.h"
uint8_t g_snap_buf[SNAPSHOT_TOTAL_SIZE];
uint8_t g_state_snap_buf[STATE_SNAP_TOTAL_SIZE];
GameEvent g_telemetry_buffer[MAX_TELEMETRY_EVENTS];
uint8_t g_telemetry_count = 0;
uint8_t g_telemetry_head = 0;
static uint32_t event_seq = 0;
static const uint32_t *telemetry_frame_ptr = NULL;

void telemetry_init(void)
{
    g_telemetry_count = 0;
    g_telemetry_head = 0;
    event_seq = 0;
    telemetry_frame_ptr = NULL;
}

void telemetry_emit(uint8_t type, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3)
{
    GameEvent *ev = &g_telemetry_buffer[g_telemetry_head];
    ev->seq = event_seq++;
    ev->frame = telemetry_frame_ptr ? *telemetry_frame_ptr : 0;
    ev->type = (uint8_t)type;
    ev->data[0] = d0;
    ev->data[1] = d1;
    ev->data[2] = d2;
    ev->data[3] = d3;

    g_telemetry_head = (uint8_t)((g_telemetry_head + 1) & (MAX_TELEMETRY_EVENTS - 1));
    if (g_telemetry_count < MAX_TELEMETRY_EVENTS) {
        g_telemetry_count++;
    }
}



void telemetry_set_frame_ptr(const uint32_t *frame_ptr)
{
    telemetry_frame_ptr = frame_ptr;
}

#ifdef DEBUG_BUILD
#include "game.h"
extern Game g_game;

/* Broad screen value for backward-compatible snapshot byte 0.
 * Dialogue is a sub-screen of overworld in the legacy game_state encoding. */
static uint8_t screen_broad(ScreenId s)
{
    if (s == SCREEN_BATTLE) return 1;
    if (s == SCREEN_GAME_OVER) return 2;
    if (s == SCREEN_THANKS) return 3;
    return 0;
}

void debug_snapshot(void)
{
    const Game *g = &g_game;
    uint8_t first_hp = 0;
    uint8_t first_active = 0;
    uint8_t i;

    for (i = 0; i < MAX_WORLD_ACTORS; i++) {
        if (g->world.actors[i].active) {
            first_hp = g->world.actors[i].hp;
            first_active = 1;
            break;
        }
    }

    g_snap_buf[0] = screen_broad(g->screen);
    g_snap_buf[1] = g->world.player.position.x;
    g_snap_buf[2] = g->world.player.position.y;
    g_snap_buf[3] = g->world.player.hp;
    g_snap_buf[4] = first_hp;
    g_snap_buf[5] = first_active;
    g_snap_buf[6] = (uint8_t)audio_get_current_track();
    g_snap_buf[7] = (uint8_t)g->battle.turn;
    g_snap_buf[8] = (uint8_t)g->battle.result;
    g_snap_buf[9] = g->battle.player.hp;
    g_snap_buf[10] = g->battle.enemies[g->battle.target_idx].hp;
    g_snap_buf[11] = (uint8_t)g->world.map_id;
    g_snap_buf[12] = g->state.flags.bytes[0];
    g_snap_buf[13] = g->dialogue.active ? 1 : 0;
    g_snap_buf[14] = g->dialogue.current_line;
    g_snap_buf[15] = (uint8_t)g->dialogue.id;
    g_snap_buf[16] = (uint8_t)g->world.player.facing;
    g_snap_buf[17] = g->game_over_choice;
    g_snap_buf[18] = (uint8_t)g->screen;
    g_snap_buf[19] = (uint8_t)g->state.scene.scene_id;

    actor_write_snapshot(&g->world, &g_snap_buf[SNAPSHOT_BASE_SIZE], MAX_SNAPSHOT_ACTORS);

    debug_state_snapshot();
}

void debug_state_snapshot(void)
{
    /* The ~460-byte builder runs banked (src/debug/telemetry_snap.c, ROM
     * bank 2) through the WRAM banked-call trampoline so it does not consume
     * the fixed-bank _CODE budget.  It reads g_game / g_state_snap_buf
     * directly, so no staging args are needed. */
    g_bk_call_bank = 2;
    g_bk_call_target = (uint16_t)&debug_state_snapshot_banked;
    banked_call_run();
}
#endif

#include "telemetry.h"
#include "audio.h"
#include "screen.h"
#include "actor.h"
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
    g_snap_buf[10] = g->battle.enemy.hp;
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

/* Serialize the canonical GameState into g_state_snap_buf for the host.
 * Fixed offsets documented in telemetry.h. */
static void snap_write16(uint8_t *dst, uint16_t v)
{
    dst[0] = (uint8_t)(v & 0xFF);
    dst[1] = (uint8_t)((v >> 8) & 0xFF);
}

void debug_state_snapshot(void)
{
    const GameState *st;
    uint8_t i;
    uint8_t *p;
    uint8_t *b = g_state_snap_buf;

    if (!&g_game) return;
    st = &g_game.state;

    b[0] = STATE_SNAP_VERSION_BYTE;
    for (i = 0; i < (MAX_STATE_FLAGS / 8); i++) {
        b[STATE_SNAP_FLAGS_OFFSET + i] = st->flags.bytes[i];
    }
    p = b + STATE_SNAP_VARIABLES_OFFSET;
    for (i = 0; i < MAX_STATE_VARIABLES; i++) {
        snap_write16(p, (uint16_t)st->variables.values[i]);
        p += 2;
    }

    b[STATE_SNAP_CURRENCY_COUNT_OFF] = MAX_CURRENCIES;
    p = b + STATE_SNAP_CURRENCY_ENTRY_OFF;
    for (i = 0; i < MAX_CURRENCIES; i++) {
        *p++ = (uint8_t)(i + 1);
        snap_write16(p, (uint16_t)st->currency.amount[i]);
        p += 2;
    }

    b[STATE_SNAP_PARTY_OFFSET] = st->party.count;
    p = b + STATE_SNAP_PARTY_OFFSET + 1;
    for (i = 0; i < st->party.count && i < MAX_PARTY_MEMBERS; i++) {
        *p++ = (uint8_t)st->party.members[i].id;
        *p++ = st->party.members[i].hp;
        *p++ = st->party.members[i].max_hp;
    }

    b[STATE_SNAP_INVENTORY_OFFSET] = st->inventory.count;
    p = b + STATE_SNAP_INVENTORY_OFFSET + 1;
    for (i = 0; i < st->inventory.count && i < 16; i++) {
        *p++ = (uint8_t)st->inventory.entries[i].item_id;
        *p++ = st->inventory.entries[i].quantity;
    }

    b[STATE_SNAP_WORLD_OFFSET] = st->world.count;
    p = b + STATE_SNAP_WORLD_OFFSET + 1;
    for (i = 0; i < st->world.count && i < 16; i++) {
        snap_write16(p, (uint16_t)st->world.actors[i].actor_id);
        p += 2;
        *p++ = st->world.actors[i].state;
    }

    b[STATE_SNAP_PROGRESSION_COUNT_OFF] = st->progression.count;
    p = b + STATE_SNAP_PROGRESSION_ENTRY_OFF;
    for (i = 0; i < st->progression.count && i < MAX_PROGRESSION_TARGETS; i++) {
        *p++ = st->progression.entries[i].target.type;
        snap_write16(p, st->progression.entries[i].target.id);
        p += 2;
        *p++ = st->progression.entries[i].state.level;
        snap_write16(p, st->progression.entries[i].state.progress);
        p += 2;
    }

    b[STATE_SNAP_EQUIPMENT_OFF] = (uint8_t)st->equipment.weapon;

    /* Runtime overworld camera + scene dims */
    b[STATE_SNAP_SCROLL_X_OFF]        = g_game.world.scroll_x;
    b[STATE_SNAP_SCROLL_Y_OFF]        = g_game.world.scroll_y;
    b[STATE_SNAP_WORLD_WIDTH_OFF]     = g_game.world.width;
    b[STATE_SNAP_WORLD_HEIGHT_OFF]    = g_game.world.height;
    b[STATE_SNAP_CAMERA_PX_X_OFF]     = g_game.world.camera_px_x;
    b[STATE_SNAP_CAMERA_PX_Y_OFF]     = g_game.world.camera_px_y;
}
#endif

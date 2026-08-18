#include "scenarios.h"
#include "rng.h"
#include "telemetry.h"
#include "audio.h"
#include "screen.h"
#include "interaction.h"
#include "event.h"
#include "dialogue.h"
#include "actor.h"
#include "scene.h"
#include "rpg/items.h"
#include "rpg/deck.h"
#include "rpg/currency.h"
#include "rpg/progression.h"
#include "rpg/save.h"
#include "content.h"
#include "game_ids.h"
#include "ui.h"
#include "battle.h"

extern Game g_game;

volatile uint8_t g_scen_load = 0;
volatile uint8_t g_scen_load_state = 0;
volatile uint8_t g_debug_action[6];
volatile uint8_t g_debug_action_pending = 0;
uint8_t g_scen_state_buf[STATE_LOAD_DESC_SIZE];

enum {
    DBG_ACT_ADD_ITEM = 1,
    DBG_ACT_REMOVE_ITEM = 2,
    DBG_ACT_ADD_CURRENCY = 3,
    DBG_ACT_ADD_PROGRESS = 4,
    DBG_ACT_BUY_ITEM = 5,
    DBG_ACT_USE_ITEM = 6,
    DBG_ACT_EQUIP_ITEM = 7,
    DBG_ACT_SAVE = 8,
    DBG_ACT_LOAD = 9,
    DBG_ACT_SET_HAND_CARD = 10
};

static void scenario_begin(uint16_t seed)
{
    g_game.frame = 0;
    g_game.game_over_choice = 0;
    rng_set_seed(seed);
    input_reset();
    telemetry_init();
    telemetry_set_frame_ptr(&g_game.frame);
    audio_play_music(MUSIC_OVERWORLD);
}

#define snap_read16(p) (*(const uint16_t *)(p))

static void scenario_load_state(void)
{
    const uint8_t *b = g_scen_state_buf;
    const uint8_t *p;
    SceneId scene;
    MapId map;
    uint8_t x, y, facing, screen, dialogue_id, start_battle, i;
    uint16_t seed;

    if (b[0] != STATE_LOAD_DESC_VERSION) return;

    screen = b[STATE_LOAD_DESC_SCREEN_OFF];
    scene = (SceneId)b[STATE_LOAD_DESC_SCENE_OFF];
    x = b[STATE_LOAD_DESC_PLAYER_X_OFF];
    y = b[STATE_LOAD_DESC_PLAYER_Y_OFF];
    facing = b[STATE_LOAD_DESC_PLAYER_FACING_OFF];
    seed = snap_read16(b + STATE_LOAD_DESC_SEED_OFF);
    dialogue_id = b[STATE_LOAD_DESC_DIALOGUE_ID_OFF];
    start_battle = b[STATE_LOAD_DESC_START_BATTLE_OFF];
    map = scene_id_to_map(scene);

    game_new_game(&g_game.state);

    p = b + STATE_LOAD_DESC_VARIABLES_ENTRY_OFF;
    for (i = 0; i < b[STATE_LOAD_DESC_VARIABLES_COUNT_OFF]; i++) {
        uint8_t vid = *p++;
        int16_t val = (int16_t)snap_read16(p);
        p += 2;
        if (vid >= 1 && vid <= MAX_STATE_VARIABLES) {
            g_game.state.variables.values[vid - 1] = val;
        }
    }

    p = b + STATE_LOAD_DESC_CURRENCY_ENTRY_OFF;
    for (i = 0; i < b[STATE_LOAD_DESC_CURRENCY_COUNT_OFF]; i++) {
        uint8_t cid = *p++;
        int16_t amt = (int16_t)snap_read16(p);
        p += 2;
        if (cid >= 1 && cid <= MAX_CURRENCIES) {
            g_game.state.currency.amount[cid - 1] = amt;
        }
    }

    p = b + STATE_LOAD_DESC_PARTY_ENTRY_OFF;
    for (i = 0; i < b[STATE_LOAD_DESC_PARTY_COUNT_OFF]; i++) {
        g_game.state.party.members[i].id = (CharacterId)*p++;
        g_game.state.party.members[i].hp = *p++;
        g_game.state.party.members[i].max_hp = *p++;
        g_game.state.party.count = (uint8_t)(i + 1);
    }

    p = b + STATE_LOAD_DESC_INVENTORY_ENTRY_OFF;
    for (i = 0; i < b[STATE_LOAD_DESC_INVENTORY_COUNT_OFF]; i++) {
        deck_collection_add(&g_game.state.cards, (CardId)*p, *(p + 1));
        p += 2;
    }

    p = b + STATE_LOAD_DESC_WORLD_ENTRY_OFF;
    for (i = 0; i < b[STATE_LOAD_DESC_WORLD_COUNT_OFF]; i++) {
        g_game.state.world.actors[i].actor_id = (ActorId)snap_read16(p);
        p += 2;
        g_game.state.world.actors[i].state = *p++;
        g_game.state.world.count = (uint8_t)(i + 1);
    }

    p = b + STATE_LOAD_DESC_PROGRESSION_ENTRY_OFF;
    for (i = 0; i < b[STATE_LOAD_DESC_PROGRESSION_COUNT_OFF]; i++) {
        uint8_t t_type = *p++;
        uint16_t t_id = snap_read16(p);
        p += 2;
        progression_ensure(&g_game.state, t_type, t_id, *p, snap_read16(p + 1));
        p += 3;
    }
    /* equipment field removed; skipped */

    g_game.state.scene.scene_id = scene;
    g_game.state.scene.player_x = x;
    g_game.state.scene.player_y = y;
    g_game.state.scene.player_facing = facing;
    world_init(&g_game.world, &g_game.state);
    world_load_map(&g_game.world, map, &g_game.state);
    g_game.world.player.position.x = x;
    g_game.world.player.position.y = y;
    g_game.world.player.facing = (Direction)facing;
    g_game.world.encounter_actor_index = NO_ACTOR_INDEX;
    dialogue_init(&g_game.dialogue);

    scenario_begin(seed);

    for (i = 0; i < STATE_LOAD_DESC_FLAGS_SIZE; i++) {
        g_game.state.flags.bytes[i] = b[STATE_LOAD_DESC_FLAGS_OFF + i];
    }
    g_game.game_over_choice = b[STATE_LOAD_DESC_GAME_OVER_CHOICE_OFF];
    g_game.shop_id = 1;
    g_game.prev_screen = SCREEN_OVERWORLD;
    g_game.screen = SCREEN_OVERWORLD;

    if (b[STATE_LOAD_DESC_FONT_TEST_OFF]) {
        ui_draw_font_test();
    }
    if (dialogue_id != DIALOGUE_ID_NONE) {
        dialogue_start_def(&g_game.dialogue, (DialogueId)dialogue_id);
        g_game.screen = SCREEN_DIALOGUE;
    }
    if (start_battle) {
        for (i = 0; i < MAX_WORLD_ACTORS; i++) {
            if (g_game.world.actors[i].active) {
                g_game.world.encounter_actor_index = i;
                break;
            }
        }
        start_battle_from_world(&g_game);
    }
    if (screen == SCREEN_GAME_OVER || screen == SCREEN_THANKS || screen == SCREEN_ENDING) {
        g_game.screen = (ScreenId)screen;
    }

    game_render_reset(&g_game);

    if (dialogue_id != DIALOGUE_ID_NONE) {
        g_game.render_cache.prev_screen = SCREEN_DIALOGUE;
    }
    debug_snapshot();
}

static void debug_run_action(void)
{
    uint8_t action = g_debug_action[0];
    uint8_t a0 = g_debug_action[1];
    int16_t a1 = (int16_t)snap_read16((const uint8_t *)&g_debug_action[2]);
    uint8_t a2 = g_debug_action[4];
    ProgressionTarget target;
    ProgressionAddResult pres;

    switch (action) {
        case DBG_ACT_ADD_ITEM:
            deck_collection_add(&g_game.state.cards, (CardId)a0, a2);
            break;
        case DBG_ACT_REMOVE_ITEM:
            deck_collection_remove(&g_game.state.cards, (CardId)a0, a2);
            break;
        case DBG_ACT_ADD_CURRENCY:
            currency_add(&g_game.state, (CurrencyId)a0, a1);
            break;
        case DBG_ACT_ADD_PROGRESS:
            target.type = a0;
            target.id = (uint16_t)a1;
            if (progression_add(&g_game.state, a0, (uint16_t)a1, a2, &pres)) {
                game_on_level_up(&g_game.state, target, &pres);
            }
            break;
        case DBG_ACT_BUY_ITEM:
            item_purchase(&g_game.state, (ItemId)a0);
            break;
        case DBG_ACT_USE_ITEM:
            item_use(&g_game.state, (ItemId)a0, (CharacterId)a2);
            break;
        case DBG_ACT_EQUIP_ITEM:
            item_equip(&g_game.state, (ItemId)a0);
            break;
        case DBG_ACT_SAVE:
            save_game_slot(a0 < SAVE_SLOT_COUNT ? a0 : 0, &g_game.state);
            break;
        case DBG_ACT_LOAD:
            if (load_game_slot(a0 < SAVE_SLOT_COUNT ? a0 : 0, &g_game.state)) {
                scene_load(&g_game, g_game.state.scene.scene_id,
                           g_game.state.scene.player_x, g_game.state.scene.player_y);
                g_game.world.player.facing = (Direction)g_game.state.scene.player_facing;
            }
            break;
        case DBG_ACT_SET_HAND_CARD:
            if (a0 < BATTLE_HAND_SIZE) {
                g_game.battle.hand[a0].type = (BattleCardType)((uint16_t)a1 & 0xFF);
                g_game.battle.hand[a0].value = a2;
            }
            break;
        default:
            break;
    }
    debug_snapshot();
}

void scenario_check_and_load(void)
{
    if (g_scen_load_state) {
        g_scen_load_state = 0;
        scenario_load_state();
    }
    if (g_debug_action_pending) {
        g_debug_action_pending = 0;
        debug_run_action();
    }
    g_scen_load = 0;
}

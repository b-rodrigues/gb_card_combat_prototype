#include "world.h"
#include "game.h"
#include "telemetry.h"
#include "actor.h"
#include "scene.h"
#include "event.h"
#include "rpg/currency.h"
#include "rpg/loot.h"
#include "rng.h"
#include "content.h"
#include "ui.h"
#include "banked.h"

void world_load_map(World *w, MapId map_id, const GameState *state)
{
    const SceneDefinition *def;

    if (!w) return;

    /* Scene size comes from the scene definition; it may be smaller than
     * the WORLD_WIDTH/HEIGHT buffer caps.  The overworld camera clamps its
     * view window to width/height. */
    def = scene_definition_for_map(map_id);
    w->width = def ? def->width : WORLD_WIDTH;
    w->height = def ? def->height : WORLD_HEIGHT;
    w->map_id = map_id;
    w->encounter_actor_index = NO_ACTOR_INDEX;
    w->map_changed = false;
    w->move_state = MOVE_STATE_IDLE;
    w->move_progress = 0;
    w->move_outcome = MOVE_OUTCOME_NONE;
    /* New scene: start the camera at the scene origin; world_update_scroll
     * brings the player into view (and clamps) on the next overworld frame. */
    w->scroll_x = 0;
    w->scroll_y = 0;
    w->camera_px_x = 0;
    w->camera_px_y = 0;

    /* Scene data determines the terrain and the exits. */
    scene_load_tiles(w, map_id);

    /* Scene data determines which hostile actors are spawned.  Actors
     * whose ActorId is DEFEATED in state are not re-spawned. */
    actor_load_scene(w, map_id, state);
}

void world_update_scroll(World *w)
{
    uint8_t px, py, max_x, max_y;
    if (!w) return;
    px = world_player_px(w);
    py = world_player_py(w);
    px = (px < 80) ? 0 : (uint8_t)(px - 80);
    py = (py < 72) ? 0 : (uint8_t)(py - 72);
    max_x = (w->width > WORLD_VIEW_W) ? (uint8_t)((w->width - WORLD_VIEW_W) << 3) : 0;
    max_y = (w->height > WORLD_VIEW_H) ? (uint8_t)((w->height - WORLD_VIEW_H) << 3) : 0;
    if (px > max_x) px = max_x;
    if (py > max_y) py = max_y;
    w->camera_px_x = px;
    w->camera_px_y = py;
    w->scroll_x = (uint8_t)(px >> 3);
    w->scroll_y = (uint8_t)(py >> 3);
}

void world_init(World *w, const GameState *state)
{
    if (!w) return;
    entity_init(&w->player, ENTITY_ID_PLAYER, 4, 4, 10, 10);
    world_load_map(w, MAP_FIELD, state);
}

void world_change_map(World *w, MapId map_id, uint8_t spawn_x, uint8_t spawn_y,
                      const GameState *state)
{
    MapId old_map;
    if (!w) return;
    old_map = w->map_id;
    world_load_map(w, map_id, state);
    w->player.position.x = spawn_x;
    w->player.position.y = spawn_y;
    w->map_changed = true;
    telemetry_emit(EVENT_MAP_CHANGED, (uint8_t)old_map, (uint8_t)map_id, spawn_x, spawn_y);
}

bool world_is_walkable(const World *w, uint8_t x, uint8_t y)
{
    uint8_t tile;
    if (!w || x >= w->width || y >= w->height) return false;
    tile = w->map[y][x];
    return (tile == TILE_FLOOR || tile == TILE_EXIT);
}

WorldMoveResult world_try_begin_move(World *w, int8_t dx, int8_t dy,
                                     const GameState *state)
{
    uint8_t target_x, target_y;
    uint8_t hostile_slot;
    const WorldActorDefinition *actor;

    if (!w || w->move_state == MOVE_STATE_MOVING) return MOVE_RESULT_NONE;
    (void)state;

    if (dy < 0) w->player.facing = DIRECTION_UP;
    else if (dy > 0) w->player.facing = DIRECTION_DOWN;
    else if (dx < 0) w->player.facing = DIRECTION_LEFT;
    else if (dx > 0) w->player.facing = DIRECTION_RIGHT;

    target_x = (uint8_t)(w->player.position.x + dx);
    target_y = (uint8_t)(w->player.position.y + dy);

    if (!world_is_walkable(w, target_x, target_y)) {
        return MOVE_RESULT_BLOCKED;
    }

    hostile_slot = actor_find_hostile_slot(w, target_x, target_y);
    if (hostile_slot != NO_ACTOR_INDEX) {
        w->move_outcome = MOVE_OUTCOME_ENCOUNTER;
        w->encounter_actor_index = hostile_slot;
    } else {
        actor = actor_find_at(w, target_x, target_y);
        if (actor) {
            telemetry_emit(EVENT_ACTOR_COLLISION, target_x, target_y,
                           (uint8_t)actor->id, 0);
            return MOVE_RESULT_BLOCKED;
        }
        w->move_outcome = (w->map[target_y][target_x] == TILE_EXIT) ? MOVE_OUTCOME_EXIT
                                                                    : MOVE_OUTCOME_NORMAL;
    }

    w->move_target_x = target_x;
    w->move_target_y = target_y;
    w->move_progress = 0;
    w->move_state = MOVE_STATE_MOVING;
    return MOVE_RESULT_MOVED;
}

WorldMoveResult world_update_move(World *w, const GameState *state)
{
    uint8_t target_x, target_y;

    if (!w) return MOVE_RESULT_NONE;
    if (w->move_state != MOVE_STATE_MOVING) return MOVE_RESULT_NONE;

    w->move_progress++;
    if (w->move_progress >= MOVE_FRAMES) {
        /* Commit: resolve the move's outcome against the target tile. */
        target_x = w->move_target_x;
        target_y = w->move_target_y;
        w->move_progress = 0;
        w->move_state = MOVE_STATE_IDLE;

        if (w->move_outcome == MOVE_OUTCOME_EXIT) {
            /* Generic scene exit: the scene definition owns destination +
             * spawn.  Like the legacy instant move, the player never commits
             * PLAYER_MOVED onto the gate; the map changes instead. */
            const SceneDefinition *def = scene_definition_for_map(w->map_id);
            const SceneExit *ex = scene_exit_at(def, target_x, target_y);
            if (ex) {
                world_change_map(w, scene_id_to_map(ex->target_scene),
                                 ex->spawn_x, ex->spawn_y, state);
                return MOVE_RESULT_MAP_CHANGED;
            }
            return MOVE_RESULT_BLOCKED;
        } else if (w->move_outcome == MOVE_OUTCOME_ENCOUNTER) {
            /* The player does not occupy the enemy tile; battle starts from
             * the pre-move position (matches the legacy instant behavior).
             * The hostile slot was persisted by world_try_begin_move. */
            if (w->encounter_actor_index < MAX_WORLD_ACTORS) {
                telemetry_emit(EVENT_ACTOR_COLLISION, target_x, target_y,
                               (uint8_t)w->actors[w->encounter_actor_index].id, 0);
                telemetry_emit(EVENT_ENCOUNTER_STARTED,
                               (uint8_t)w->actors[w->encounter_actor_index].id, 0, 0, 0);
                return MOVE_RESULT_ENCOUNTER;
            }
            return MOVE_RESULT_BLOCKED;
        } else {
            telemetry_emit(EVENT_PLAYER_MOVED, w->player.position.x,
                           w->player.position.y, target_x, target_y);
            w->player.position.x = target_x;
            w->player.position.y = target_y;
            return MOVE_RESULT_MOVED;
        }
    }
    return MOVE_RESULT_MOVED;
}

/* Pixel helpers run banked (src/world/px_banked.c) -- pure arithmetic,
 * staged pointers, results through the shared byte below. */
uint8_t g_px_result;

static uint8_t px_dispatch(uint8_t variant, const void *p)
{
    g_bk_call_bank = 2;
    g_bk_call_target = (uint16_t)&world_px_banked;
    g_bk_byte_a = variant;
    g_bk_ptr_a = (void *)p;
    banked_call_run();
    return g_px_result;
}

uint8_t world_player_px(const World *w)
{
    return w ? px_dispatch(0, w) : 0;
}

uint8_t world_player_py(const World *w)
{
    return w ? px_dispatch(1, w) : 0;
}

uint8_t world_actor_px(const WorldActorRuntime *a)
{
    return a ? px_dispatch(2, a) : 0;
}

uint8_t world_actor_py(const WorldActorRuntime *a)
{
    return a ? px_dispatch(3, a) : 0;
}

void world_on_battle_end(Game *g, bool victory)
{
    World *w;
    uint8_t idx;
    uint16_t actor_id;
    if (!g) return;
    w = &g->world;

    idx = w->encounter_actor_index;
    w->encounter_actor_index = NO_ACTOR_INDEX;
    if (idx == NO_ACTOR_INDEX) return;

    if (victory) {
        WorldActorRuntime *act = &w->actors[idx];
        actor_id = act->actor_id;
        act->active = 0;
        act->hp = 0;
        act->flags = ACTOR_STATE_NONE;
        telemetry_emit(EVENT_ENTITY_DEFEATED, (uint8_t)act->id, 0, 0, 0);
        if (act->reward_currency != 0 && act->gold_reward != 0) {
            currency_add(&g->state, (CurrencyId)act->reward_currency, act->gold_reward);
        }
        if (actor_id != 0) {
            game_world_set_actor_state(&g->state, actor_id, ACTOR_STATE_DEFEATED);
        }
        event_resolve_actor_defeated(g, actor_id, act->id);

        /* Loot drop (docs/loot.md §8/§17, Phase 2): roll one combat card
         * from the enemy family's profile pool.  50% gate via a single
         * rng bit; consumes the shared game RNG (deterministic per seed).
         * The game layer supplies the pool; loot.c owns the roll. */
        if (rng_next() & 1) {
            uint8_t pool_len = 0;
            const CardId *pool = game_loot_pool_for_battle(act->battle_type,
                                                           &pool_len);
            (void)loot_roll_combat(&g->state.loot, pool, pool_len);
        }
    }
}

void world_on_battle_fled(Game *g)
{
    /* Body runs banked (src/world/fled_banked.c) through the WRAM
     * trampoline -- pure WRAM reads/writes, no staging args needed. */
    if (!g) return;
    g_bk_call_bank = 2;
    g_bk_call_target = (uint16_t)&world_on_battle_fled_banked;
    g_bk_ptr_a = (void *)g;
    banked_call_run();
}

static const uint8_t s_patrol_circle[4] = { 0x36, 0x1A, 0x29, 0x05 };
static const uint8_t s_patrol_line[8] = { 0x01, 0x15, 0x19, 0x05, 0x24, 0x35, 0x36, 0x25 };

WorldMoveResult world_update_actors(World *w)
{
    uint8_t slot;
    WorldActorRuntime *a;
    uint8_t target_x, target_y, facing;
    uint8_t entry;

    if (!w) return MOVE_RESULT_NONE;

    for (slot = 0; slot < MAX_WORLD_ACTORS; slot++) {
        a = &w->actors[slot];
        if (!a->active || a->ai_type == AI_NONE) {
            continue;
        }

        if (a->move_state) {
            a->move_progress++;
            if (a->move_progress >= 8) {
                a->x = a->move_target_x;
                a->y = a->move_target_y;
                a->move_state = 0;
                a->move_progress = 0;
                a->ai_timer = PATROL_STEP_INTERVAL;
                telemetry_emit(EVENT_ACTOR_STATE_CHANGE, (uint8_t)a->id, a->x, a->y, a->facing);
            }
            continue;
        }

        if (a->ai_timer > 0) {
            a->ai_timer--;
            continue;
        }

        entry = (a->ai_type == AI_PATROL_CIRCLE) ?
            s_patrol_circle[a->ai_step & 3] :
            s_patrol_line[a->ai_step & 7];
        facing = (uint8_t)(entry >> 4);
        target_x = (uint8_t)(a->spawn_x + (entry & 3) - 1);
        target_y = (uint8_t)(a->spawn_y + ((entry >> 2) & 3) - 1);

        if (target_x == a->x && target_y == a->y) {
            a->facing = facing;
            a->ai_step++;
            a->ai_timer = PATROL_STEP_INTERVAL;
            continue;
        }

        if (!world_is_walkable(w, target_x, target_y) ||
            w->map[target_y][target_x] == TILE_EXIT ||
            actor_find_at(w, target_x, target_y) != NULL ||
            actor_find_hostile_slot(w, target_x, target_y) != NO_ACTOR_INDEX) {
            a->ai_timer = PATROL_STEP_INTERVAL;
            continue;
        }

        if (target_x == w->player.position.x && target_y == w->player.position.y) {
            w->encounter_actor_index = slot;
            telemetry_emit(EVENT_ACTOR_COLLISION, target_x, target_y, (uint8_t)a->id, 0);
            telemetry_emit(EVENT_ENCOUNTER_STARTED, (uint8_t)a->id, 0, 0, 0);
            return MOVE_RESULT_ENCOUNTER;
        }

        a->ai_step++;
        a->move_state = 1;
        a->move_target_x = target_x;
        a->move_target_y = target_y;
        a->move_progress = 0;
        a->facing = facing;
    }

    return MOVE_RESULT_NONE;
}

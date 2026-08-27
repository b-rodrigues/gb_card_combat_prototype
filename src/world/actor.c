#include "actor.h"
#include "world.h"
#include "game_ids.h"
#include "banked.h"
#include <stddef.h>

/* ── Actor engine ──────────────────────────────────────────────────
 * Per-scene actor definitions live in banked ROM (GAME_CONTENT_BANK).
 * actor_load_scene() stages the bank-2 loader body
 * (src/world/actor_load_banked.c), which reads the registered tables
 * directly and spawns hostiles into World.actors runtime slots and
 * friendlies into g_static_actors[].  Gameplay lookups are pure WRAM. */

const WorldActorTable *g_actor_registry = NULL;
uint8_t g_actor_registry_count = 0;
static uint8_t g_actor_bank = 2;

WorldActorDefinition g_static_actors[7];
uint8_t g_static_actor_count = 0;

void actor_register_tables(const WorldActorTable *tables, uint8_t count, uint8_t bank)
{
    g_actor_registry = tables;
    g_actor_registry_count = count;
    g_actor_bank = bank;
}

uint8_t actor_find_hostile_slot(const World *world, uint8_t x, uint8_t y)
{
    uint8_t i;
    const WorldActorRuntime *a;
    if (!world) return NO_ACTOR_INDEX;
    for (i = 0; i < MAX_WORLD_ACTORS; i++) {
        a = &world->actors[i];
        if (!a->active) continue;
        if (a->x == x && a->y == y) return i;
        if (a->move_state && a->move_target_x == x && a->move_target_y == y) return i;
    }
    return NO_ACTOR_INDEX;
}

const WorldActorDefinition *actor_find_at(const World *world, uint8_t x, uint8_t y)
{
    uint8_t i;
    (void)world;
    for (i = 0; i < g_static_actor_count; i++) {
        if (g_static_actors[i].x == x && g_static_actors[i].y == y) {
            return &g_static_actors[i];
        }
    }
    return NULL;
}

ActorEngageResult actor_engage(const WorldActorDefinition *actor, DialogueState *dialogue)
{
    if (!actor) return ENGAGE_NONE;
    if (actor->flags & ACTOR_FLAG_HOSTILE) {
        return ENGAGE_BATTLE;
    }

    if (actor->interaction == INTERACTION_DIALOGUE) {
        dialogue_start_def(dialogue, actor->dialogue_id);
        return ENGAGE_DIALOGUE;
    }

    if (actor->interaction == INTERACTION_SHOP) {
        return ENGAGE_SHOP;
    }

    if (actor->interaction == INTERACTION_SAVE) {
        return ENGAGE_SAVE;
    }

    return ENGAGE_NONE;
}

static const char *actor_name_for_visual(uint8_t visual)
{
    if (visual == 'V') return "BAT";
    if (visual == 'L') return "LORD OF SLIMES";
    if (visual == 'W') return "WIZARD";
    return "SLIME";
}

void actor_load_scene(World *world, MapId map_id, const GameState *state)
{
    uint8_t i;

    /* Body runs banked (src/world/actor_load_banked.c): the registered
     * tables live in the same ROM bank, so the body reads them directly
     * with no staging copies (AGENTS.md 52.11.1). */
    g_bk_call_bank = 2;
    g_bk_call_target = (uint16_t)&actor_load_scene_banked;
    g_bk_ptr_a = (void *)world;
    g_bk_ptr_b = (void *)state;
    g_bk_byte_a = (uint8_t)map_id;
    banked_call_run();

    /* display_name literals must live in the fixed bank: derive them
     * here, after the trampoline returns (the body leaves the field
     * untouched rather than pointing it into bank 2). */
    for (i = 0; i < MAX_WORLD_ACTORS; i++) {
        if (world->actors[i].active) {
            world->actors[i].display_name =
                actor_name_for_visual(world->actors[i].visual);
        }
    }
}

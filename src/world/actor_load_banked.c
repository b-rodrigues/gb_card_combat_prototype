#pragma bank 2

#include "actor.h"
#include "world.h"
#include "rpg/state.h"
#include "banked.h"

/* ── Scene actor loader, bank-2 body ────────────────────────────────
 * Dispatched by actor_load_scene() (src/world/actor.c) through the WRAM
 * trampoline.  The registered definition tables live in THIS bank, so
 * the body reads them directly -- the old fixed-bank implementation's
 * per-row banked_copy staging disappears entirely.
 *
 * Self-contained: reads only its own bank-local data, the staged
 * pointers/bytes, and WRAM (always mapped).  Never calls fixed-bank
 * code: the two tiny GameState lookups below are deliberate local
 * replicas of game_variable_get() / game_world_actor_is_defeated()
 * (src/rpg/state.c) -- KEEP IN SYNC with those if their storage layout
 * ever changes. */

extern WorldActorDefinition g_static_actors[7];
extern uint8_t g_static_actor_count;

static void actor_spawn(WorldActorRuntime *r, const WorldActorDefinition *def)
{
    /* NOTE: display_name is deliberately NOT set here -- the wrapper
     * derives it from `visual` AFTER the trampoline returns, because
     * these literals must live in the fixed bank (overworld_screen
     * dereferences the pointer while bank 1 is mapped). */
    r->actor_id = def->actor_id;
    r->id = def->id;
    r->active = 1;
    r->x = def->x;
    r->y = def->y;
    r->facing = def->facing;
    r->hp = def->hp;
    r->max_hp = def->max_hp;
    r->flags = ACTOR_STATE_NONE;
    r->gold_reward = def->gold_reward;
    r->reward_currency = def->reward_currency;
    /* display_name deliberately left unset: the wrapper derives it from
     * `visual` after the trampoline returns, because these literals must
     * live in the fixed bank (overworld_screen dereferences the pointer
     * while bank 1 is mapped). */
    r->visual = def->visual;
    r->sprite_kind = def->sprite_kind;
    r->spawn_x = def->x;
    r->spawn_y = def->y;
    r->ai_type = def->ai_type;
    r->ai_step = 0;
    r->ai_timer = PATROL_STEP_INTERVAL;
    r->move_state = 0;
    r->move_target_x = def->x;
    r->move_target_y = def->y;
    r->move_progress = 0;
    r->battle_type = (uint8_t)def->battle_id;
}

void actor_load_scene_banked(void)
{
    World *world = (World *)g_bk_ptr_a;
    const GameState *state = (const GameState *)g_bk_ptr_b;
    MapId map_id = (MapId)g_bk_byte_a;
    uint8_t i, d, slot;

    if (!world) return;

    for (slot = 0; slot < MAX_WORLD_ACTORS; slot++) {
        world->actors[slot].active = 0;
    }
    g_static_actor_count = 0;

    if (!g_actor_registry) return;

    for (i = 0; i < g_actor_registry_count; i++) {
        const WorldActorTable *tbl = &g_actor_registry[i];
        if (tbl->map_id != map_id) continue;

        slot = 0;
        for (d = 0; d < tbl->count; d++) {
            const WorldActorDefinition *def = &tbl->defs[d];

            /* Replica of game_world_actor_is_defeated() (src/rpg/state.c):
             * linear scan of the persistent world actor state. */
            if (def->actor_id != 0) {
                bool defeated = false;
                uint8_t w;
                for (w = 0; w < state->world.count; w++) {
                    if (state->world.actors[w].actor_id == def->actor_id) {
                        defeated = (state->world.actors[w].state ==
                                    (uint8_t)ACTOR_STATE_DEFEATED);
                        break;
                    }
                }
                if (defeated) continue;
            }

            /* Replica of game_variable_get() (src/rpg/state.c):
             * variables.values[] is 1-indexed by VariableId. */
            if (def->spawn_variable != 0 &&
                (!(def->spawn_variable >= 1 &&
                   def->spawn_variable <= MAX_STATE_VARIABLES) ||
                 state->variables.values[def->spawn_variable - 1] !=
                     def->spawn_value)) {
                continue;
            }

            if (def->flags & ACTOR_FLAG_HOSTILE) {
                if (slot < MAX_WORLD_ACTORS) {
                    actor_spawn(&world->actors[slot], def);
                    slot++;
                }
            } else if (g_static_actor_count < 6) {
                /* Field-wise copy: struct assignment lowers to
                 * __memcpy, which lives in the fixed bank and is
                 * unreachable while bank 2 is mapped (see
                 * status_content.c for the identical pattern). */
                g_static_actors[g_static_actor_count].actor_id = def->actor_id;
                g_static_actors[g_static_actor_count].id = def->id;
                g_static_actors[g_static_actor_count].x = def->x;
                g_static_actors[g_static_actor_count].y = def->y;
                g_static_actors[g_static_actor_count].facing = def->facing;
                g_static_actors[g_static_actor_count].flags = def->flags;
                g_static_actors[g_static_actor_count].visual = def->visual;
                g_static_actors[g_static_actor_count].sprite_kind =
                    def->sprite_kind;
                g_static_actors[g_static_actor_count].display_name =
                    def->display_name;
                g_static_actors[g_static_actor_count].interaction =
                    def->interaction;
                g_static_actors[g_static_actor_count].shop_id = def->shop_id;
                g_static_actors[g_static_actor_count].dialogue_id =
                    def->dialogue_id;
                g_static_actors[g_static_actor_count].battle_id =
                    def->battle_id;
                g_static_actors[g_static_actor_count].ai_type = def->ai_type;
                g_static_actors[g_static_actor_count].hp = def->hp;
                g_static_actors[g_static_actor_count].max_hp = def->max_hp;
                g_static_actors[g_static_actor_count].gold_reward =
                    def->gold_reward;
                g_static_actors[g_static_actor_count].reward_currency =
                    def->reward_currency;
                g_static_actors[g_static_actor_count].spawn_variable =
                    def->spawn_variable;
                g_static_actors[g_static_actor_count].spawn_value =
                    def->spawn_value;
                g_static_actor_count++;
            }
        }
        break;
    }
}

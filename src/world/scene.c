#include "scene.h"
#include "actor.h"
#include "banked.h"

/* ── Scene data (resident in ROM Bank 2) ─────────────────────────── */

extern const SceneExit g_all_exits[];
extern const SceneDefinition g_scenes[];

static SceneDefinition s_scene_scratch;
static SceneExit s_exit_scratch;

const SceneDefinition *scene_definition_for_map(MapId map_id)
{
    if (map_id > MAP_SOUTH_FIELD) return NULL;
    banked_copy(5, &s_scene_scratch, &g_scenes[map_id], sizeof(SceneDefinition));
    return &s_scene_scratch;
}

static MapId s_cached_tileset_map = (MapId)0xFF;
static WorldTilesetKind s_cached_tileset_kind = WORLD_TILESET_FOREST;

WorldTilesetKind scene_get_tileset(MapId map_id)
{
    const SceneDefinition *def;
    if (map_id == s_cached_tileset_map) {
        return s_cached_tileset_kind;
    }
    s_cached_tileset_map = map_id;
    def = scene_definition_for_map(map_id);
    s_cached_tileset_kind = def ? def->tileset : WORLD_TILESET_FOREST;
    return s_cached_tileset_kind;
}

const SceneExit *scene_exit_at(const SceneDefinition *def, uint8_t x, uint8_t y)
{
    uint8_t i;
    if (!def) return NULL;
    for (i = 0; i < def->exit_count; i++) {
        banked_copy(5, &s_exit_scratch, &def->exits[i], sizeof(SceneExit));
        if (s_exit_scratch.gate_x == x && s_exit_scratch.gate_y == y) {
            return &s_exit_scratch;
        }
    }
    return NULL;
}

/* scene_load_tiles() is a fixed-bank wrapper around the banked body in
 * src/world/scene_load.c (ROM bank 5).  The wrapper stages the World pointer
 * and map id into the _DATA globals (banked.c) and runs the banked no-arg
 * function through the WRAM banked-call trampoline (crt0.s). */
void scene_load_tiles(World *w, MapId map_id)
{
    g_bk_call_bank = 5;
    g_bk_call_target = (uint16_t)&scene_load_tiles_banked;
    g_bk_ptr_a = (void *)w;
    g_bk_byte_a = (uint8_t)map_id;
    banked_call_run();
}

/* scene_spawn() is a fixed-bank wrapper around the banked body in
 * src/world/scene_load.c (ROM bank 5).  The wrapper stages the map id into
 * the _DATA globals (banked.c) and runs the banked no-arg function through
 * the WRAM banked-call trampoline (crt0.s); the spawn comes back in
 * g_bk_byte_b/c/d. */
void scene_spawn(MapId map_id)
{
    g_bk_call_bank = 5;
    g_bk_call_target = (uint16_t)&scene_spawn_banked;
    g_bk_byte_a = (uint8_t)map_id;
    banked_call_run();
}

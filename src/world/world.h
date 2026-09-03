#ifndef WORLD_H
#define WORLD_H

#include "entity.h"
#include "rpg/state.h"
#include <stdbool.h>

/* Hard caps for the tile buffer: a scene may be any size up to these.
 * The overworld camera windows a WORLD_VIEW_W x WORLD_VIEW_H view out of
 * the scene and scrolls it with the scroll offset; the 40-col cap
 * exercises the tilemap ring-buffer wrap (the 32x32 BG tilemap is 32
 * wide). */
#define WORLD_WIDTH  40
#define WORLD_HEIGHT 24

/* Overworld camera view window in tiles.  The camera (World.scroll_x/y)
 * keeps the player inside this window, scrolling as the player crosses its
 * edge and clamping at the scene bounds (world width/height). */
#define WORLD_VIEW_W 20
#define WORLD_VIEW_H 18

/* Maximum concurrent hostile actors in a scene (compile-time constant). */
#define MAX_WORLD_ACTORS 4
#define NO_ACTOR_INDEX   0xFF

/* Frames to walk a single tile (1 px/frame of an 8px tile).  Movement is
 * animated: a tile commit happens MOVE_FRAMES frames after the move starts,
 * at which point PLAYER_MOVED / exits / encounters are resolved. */
#define MOVE_FRAMES 8

typedef enum {
    MOVE_STATE_IDLE   = 0,
    MOVE_STATE_MOVING = 1
} MoveState;

/* What the commit of a started move must resolve.  Set by world_try_begin_move
 * from the target tile's contents, consumed by world_update_move. */
typedef enum {
    MOVE_OUTCOME_NONE      = 0,
    MOVE_OUTCOME_NORMAL    = 1,
    MOVE_OUTCOME_EXIT      = 2,
    MOVE_OUTCOME_ENCOUNTER = 3
} MoveOutcome;

typedef enum {
    MOVE_RESULT_NONE        = 0,
    MOVE_RESULT_BLOCKED     = 1,
    MOVE_RESULT_MOVED       = 2,
    MOVE_RESULT_MAP_CHANGED = 3,
    MOVE_RESULT_ENCOUNTER   = 4
} WorldMoveResult;

typedef enum {
    MAP_FIELD         = 0,
    MAP_TOWN          = 1,
    MAP_FOREST        = 2,
    MAP_MOUNTAIN_PASS = 3,
    MAP_CASTLE        = 4,
    MAP_SOUTH_FIELD   = 5
} MapId;

/* A single generic exit tile type; the scene definition owns the
 * destination/spawn/visual of each exit. */
typedef enum {
    TILE_FLOOR              = 0,
    TILE_WALL               = 1,
    TILE_EXIT               = 2,
    TILE_BUILDING           = 3,
    TILE_STUMP_TL           = 4,
    TILE_STUMP_TR           = 5,
    TILE_STUMP_BL           = 6,
    TILE_STUMP_BR           = 7,
    TILE_DUNGEON_FLOOR      = 8,
    TILE_DUNGEON_WALL       = 9,
    TILE_DUNGEON_CRACK      = 10,
    TILE_DUNGEON_PILLAR     = 11,
    TILE_DUNGEON_ARCH       = 12,
    TILE_DUNGEON_DECOR      = 13,
    TILE_DUNGEON_SOLID      = 14,
    /* Houses walls */
    TILE_HOUSES_WALL_00     = 15,
    TILE_HOUSES_WALL_01     = 16,
    TILE_HOUSES_WALL_02     = 17,
    TILE_HOUSES_WALL_03     = 18,
    TILE_HOUSES_WALL_04     = 19,
    TILE_HOUSES_WALL_05     = 20,
    TILE_HOUSES_WALL_06     = 21,
    TILE_HOUSES_WALL_07     = 22,
    TILE_HOUSES_WALL_08     = 23,
    TILE_HOUSES_WALL_09     = 24,
    TILE_HOUSES_WALL_10     = 25,
    TILE_HOUSES_WALL_11     = 26,
    TILE_HOUSES_WALL_12     = 27,
    TILE_HOUSES_WALL_13     = 28,
    /* Houses walls (continued) */
    TILE_HOUSES_WALL_14     = 29,
    TILE_HOUSES_WALL_15     = 30,
    TILE_HOUSES_WALL_16     = 31,
    TILE_HOUSES_WALL_17     = 32,
    TILE_HOUSES_WALL_18     = 33,
    TILE_HOUSES_WALL_19     = 34,
    TILE_HOUSES_WALL_20     = 35,
    TILE_HOUSES_WALL_21     = 36,
    TILE_HOUSES_WALL_22     = 37,
    TILE_HOUSES_WALL_23     = 38,
    TILE_HOUSES_WALL_24     = 39,
    TILE_HOUSES_WALL_25     = 40,
    TILE_HOUSES_WALL_26     = 41,
    TILE_HOUSES_WALL_27     = 42,
    /* Houses roofs */
    TILE_HOUSES_ROOF_00     = 43,
    TILE_HOUSES_ROOF_01     = 44,
    TILE_HOUSES_ROOF_02     = 45,
    TILE_HOUSES_ROOF_03     = 46,
    TILE_HOUSES_ROOF_04     = 47,
    TILE_HOUSES_ROOF_05     = 48,
    TILE_HOUSES_ROOF_06     = 49,
    TILE_HOUSES_ROOF_07     = 50,
    TILE_HOUSES_ROOF_08     = 51,
    TILE_HOUSES_ROOF_09     = 52,
    TILE_HOUSES_ROOF_10     = 53,
    TILE_HOUSES_ROOF_11     = 54,
    TILE_HOUSES_ROOF_12     = 55,
    TILE_HOUSES_ROOF_13     = 56,
    /* Houses floors */
    TILE_HOUSES_FLOOR_00    = 57,
    TILE_HOUSES_FLOOR_01    = 58,
    TILE_HOUSES_FLOOR_02    = 59,
    TILE_HOUSES_FLOOR_03    = 60,
    TILE_HOUSES_FLOOR_04    = 61,
    TILE_HOUSES_FLOOR_05    = 62,
    TILE_HOUSES_FLOOR_06    = 63,
    TILE_HOUSES_FLOOR_07    = 64,
    TILE_HOUSES_FLOOR_08    = 65,
    TILE_HOUSES_FLOOR_09    = 66,
    TILE_HOUSES_FLOOR_10    = 67,
    TILE_HOUSES_FLOOR_11    = 68,
    TILE_HOUSES_FLOOR_12    = 69,
    TILE_HOUSES_FLOOR_13    = 70,
    /* Houses doors */
    TILE_HOUSES_DOOR_00     = 71,
    TILE_HOUSES_DOOR_01     = 72,
    TILE_HOUSES_DOOR_02     = 73,
    TILE_HOUSES_DOOR_03     = 74,
    TILE_HOUSES_DOOR_04     = 75,
    TILE_HOUSES_DOOR_05     = 76,
    /* Houses windows */
    TILE_HOUSES_WINDOW_00   = 77,
    TILE_HOUSES_WINDOW_01   = 78,
    TILE_HOUSES_WINDOW_02   = 79,
    TILE_HOUSES_WINDOW_03   = 80,
    TILE_HOUSES_WINDOW_04   = 81,
    TILE_HOUSES_WINDOW_05   = 82,
    /* Nature ground */
    TILE_NATURE_GROUND_00   = 83,
    TILE_NATURE_GROUND_01   = 84,
    TILE_NATURE_GROUND_02   = 85,
    TILE_NATURE_GROUND_03   = 86,
    TILE_NATURE_GROUND_04   = 87,
    TILE_NATURE_GROUND_05   = 88,
    TILE_NATURE_GROUND_06   = 89,
    TILE_NATURE_GROUND_07   = 90,
    TILE_NATURE_GROUND_08   = 91,
    TILE_NATURE_GROUND_09   = 92,
    TILE_NATURE_GROUND_10   = 93,
    TILE_NATURE_GROUND_11   = 94,
    TILE_NATURE_GROUND_12   = 95,
    TILE_NATURE_GROUND_13   = 96,
    /* Nature vegetation */
    TILE_NATURE_VEG_00      = 97,
    TILE_NATURE_VEG_01      = 98,
    TILE_NATURE_VEG_02      = 99,
    TILE_NATURE_VEG_03      = 100,
    TILE_NATURE_VEG_04      = 101,
    TILE_NATURE_VEG_05      = 102,
    TILE_NATURE_VEG_06      = 103,
    TILE_NATURE_VEG_07      = 104,
    TILE_NATURE_VEG_08      = 105,
    TILE_NATURE_VEG_09      = 106,
    TILE_NATURE_VEG_10      = 107,
    TILE_NATURE_VEG_11      = 108,
    TILE_NATURE_VEG_12      = 109,
    TILE_NATURE_VEG_13      = 110,
    /* Objects furniture */
    TILE_OBJECTS_FURN_00    = 111,
    TILE_OBJECTS_FURN_01    = 112,
    TILE_OBJECTS_FURN_02    = 113,
    TILE_OBJECTS_FURN_03    = 114,
    TILE_OBJECTS_FURN_04    = 115,
    TILE_OBJECTS_FURN_05    = 116,
    TILE_OBJECTS_FURN_06    = 117,
    TILE_OBJECTS_FURN_07    = 118,
    TILE_OBJECTS_FURN_08    = 119,
    TILE_OBJECTS_FURN_09    = 120,
    TILE_OBJECTS_FURN_10    = 121,
    TILE_OBJECTS_FURN_11    = 122,
    TILE_OBJECTS_FURN_12    = 123,
    TILE_OBJECTS_FURN_13    = 124,
    /* Structures fences */
    TILE_STRUCT_FENCE_00    = 125,
    TILE_STRUCT_FENCE_01    = 126,
    TILE_STRUCT_FENCE_02    = 127,
    TILE_STRUCT_FENCE_03    = 128,
    TILE_STRUCT_FENCE_04    = 129,
    TILE_STRUCT_FENCE_05    = 130,
    TILE_STRUCT_FENCE_06    = 131,
    TILE_STRUCT_FENCE_07    = 132,
    /* Structures props */
    TILE_STRUCT_PROP_00     = 133,
    TILE_STRUCT_PROP_01     = 134,
    TILE_STRUCT_PROP_02     = 135,
    TILE_STRUCT_PROP_03     = 136,
    TILE_STRUCT_PROP_04     = 137,
    TILE_STRUCT_PROP_05     = 138,
    TILE_STRUCT_PROP_06     = 139,
    TILE_STRUCT_PROP_07     = 140,
    /* Desolate landscape */
    TILE_DESOLATE_WALL_00    = 141,
    TILE_DESOLATE_WALL_01    = 142,
    TILE_DESOLATE_WALL_02    = 143,
    TILE_DESOLATE_WALL_03    = 144,
    TILE_DESOLATE_WALL_04    = 145,
    TILE_DESOLATE_WALL_05    = 146,
    TILE_DESOLATE_WALL_06    = 147,
    TILE_DESOLATE_WALL_07    = 148,
    TILE_DESOLATE_WALL_08    = 149,
    TILE_DESOLATE_WALL_09    = 150,
    TILE_DESOLATE_WALL_10    = 151,
    TILE_DESOLATE_WALL_11    = 152,
    TILE_DESOLATE_TREE_TL    = 153,
    TILE_DESOLATE_TREE_TR    = 154,
    TILE_DESOLATE_ROCK_TL    = 155,
    TILE_DESOLATE_ROCK_TR    = 156,
    TILE_DESOLATE_WALL_12    = 157,
    TILE_DESOLATE_WALL_13    = 158,
    TILE_DESOLATE_WALL_14    = 159,
    TILE_DESOLATE_WALL_15    = 160,
    TILE_DESOLATE_WALL_16    = 161,
    TILE_DESOLATE_WALL_17    = 162,
    TILE_DESOLATE_FLOOR_00   = 163,
    TILE_DESOLATE_FLOOR_01   = 164,
    TILE_DESOLATE_FLOOR_02   = 165,
    TILE_DESOLATE_FLOOR_03   = 166,
    TILE_DESOLATE_WALL_18    = 167,
    TILE_DESOLATE_WALL_19    = 168,
    TILE_DESOLATE_TREE_BL    = 169,
    TILE_DESOLATE_TREE_BR    = 170,
    TILE_DESOLATE_ROCK_BL    = 171,
    TILE_DESOLATE_ROCK_BR    = 172,
    TILE_DESOLATE_FLOOR_PLAIN = 173,
    TILE_DESOLATE_HERO_01    = 174,
    TILE_DESOLATE_HERO_02    = 175,
    TILE_DESOLATE_KOBOLD_01  = 176,
    TILE_DESOLATE_KOBOLD_02  = 177,
    TILE_DESOLATE_FIRE_01    = 178,
    TILE_DESOLATE_FIRE_02    = 179,
    TILE_DESOLATE_MERCHANT   = 180,
    TILE_DESOLATE_STAIRCASE  = 181,
    /* Desolate Landscape (level-editor) tiles: 48 tiles in sheet scan order,
     * mapping 1:1 to the 48-tile desolate VRAM block (RPG_TILE_BASE_DESOLATE).
     * TILE_DESOLATE_LANDSCAPE_<i> renders VRAM block i. value = 182 + i. */
    TILE_DESOLATE_LANDSCAPE_00 = 182,
    TILE_DESOLATE_LANDSCAPE_01 = 183,
    TILE_DESOLATE_LANDSCAPE_02 = 184,
    TILE_DESOLATE_LANDSCAPE_03 = 185,
    TILE_DESOLATE_LANDSCAPE_04 = 186,
    TILE_DESOLATE_LANDSCAPE_05 = 187,
    TILE_DESOLATE_LANDSCAPE_06 = 188,
    TILE_DESOLATE_LANDSCAPE_07 = 189,
    TILE_DESOLATE_LANDSCAPE_08 = 190,
    TILE_DESOLATE_LANDSCAPE_09 = 191,
    TILE_DESOLATE_LANDSCAPE_10 = 192,
    TILE_DESOLATE_LANDSCAPE_11 = 193,
    TILE_DESOLATE_LANDSCAPE_12 = 194,
    TILE_DESOLATE_LANDSCAPE_13 = 195,
    TILE_DESOLATE_LANDSCAPE_14 = 196,
    TILE_DESOLATE_LANDSCAPE_15 = 197,
    TILE_DESOLATE_LANDSCAPE_16 = 198,
    TILE_DESOLATE_LANDSCAPE_17 = 199,
    TILE_DESOLATE_LANDSCAPE_18 = 200,
    TILE_DESOLATE_LANDSCAPE_19 = 201,
    TILE_DESOLATE_LANDSCAPE_20 = 202,
    TILE_DESOLATE_LANDSCAPE_21 = 203,
    TILE_DESOLATE_LANDSCAPE_22 = 204,
    TILE_DESOLATE_LANDSCAPE_23 = 205,
    TILE_DESOLATE_LANDSCAPE_24 = 206,
    TILE_DESOLATE_LANDSCAPE_25 = 207,
    TILE_DESOLATE_LANDSCAPE_26 = 208,
    TILE_DESOLATE_LANDSCAPE_27 = 209,
    TILE_DESOLATE_LANDSCAPE_28 = 210,
    TILE_DESOLATE_LANDSCAPE_29 = 211,
    TILE_DESOLATE_LANDSCAPE_30 = 212,
    TILE_DESOLATE_LANDSCAPE_31 = 213,
    TILE_DESOLATE_LANDSCAPE_32 = 214,
    TILE_DESOLATE_LANDSCAPE_33 = 215,
    TILE_DESOLATE_LANDSCAPE_34 = 216,
    TILE_DESOLATE_LANDSCAPE_35 = 217,
    TILE_DESOLATE_LANDSCAPE_36 = 218,
    TILE_DESOLATE_LANDSCAPE_37 = 219,
    TILE_DESOLATE_LANDSCAPE_38 = 220,
    TILE_DESOLATE_LANDSCAPE_39 = 221,
    TILE_DESOLATE_LANDSCAPE_40 = 222,
    TILE_DESOLATE_LANDSCAPE_41 = 223,
    TILE_DESOLATE_LANDSCAPE_42 = 224,
    TILE_DESOLATE_LANDSCAPE_43 = 225,
    TILE_DESOLATE_LANDSCAPE_44 = 226,
    TILE_DESOLATE_LANDSCAPE_45 = 227,
    TILE_DESOLATE_LANDSCAPE_46 = 228,
    TILE_DESOLATE_LANDSCAPE_47 = 229
} TileType;

/* Frames between autonomous patrol steps for hostile actors (~0.53 seconds). */
#define PATROL_STEP_INTERVAL 32

/* How a hostile/visual actor is drawn in the overworld.  The choice is a
 * per-actor property selected by the level editor's object sprite
 * (overworld_sprite / animation_frames) and carried end-to-end from the
 * level JSON through compile.py into the ROM.  Defined here so both
 * WorldActorRuntime (this header) and WorldActorDefinition (actor.h, which
 * includes world.h) can use it without a circular include. */
typedef enum {
    SPRITE_KIND_ASCII = 0,  /* render the ASCII visual via the console font */
    SPRITE_KIND_KOBOLD = 1, /* 2-frame slime-as-kobold OBJ sprite */
    SPRITE_KIND_BAT = 2,    /* 2-frame per-map bat OBJ sprite */
    SPRITE_KIND_BOSS = 3    /* 2x2 castle boss drawn as background tiles */
} ActorSpriteKind;

/* Mutable runtime state for a spawned World Actor.  Static actor
 * configuration lives in WorldActorDefinition; hostile actors are spawned
 * into World.actors by actor_load_scene().  actor_id is the persistent
 * ActorId copied from the definition (0 for non-persistent). */
typedef struct {
    uint16_t actor_id;
    EntityId id;
    uint8_t active;
    uint8_t x;
    uint8_t y;
    uint8_t facing;
    uint8_t hp;
    uint8_t max_hp;
    uint8_t flags;               /* runtime state flags (future) */
    uint8_t gold_reward;         /* copied from the definition */
    uint8_t reward_currency;     /* copied from the definition */
    const char *display_name;    /* copied from the definition */
    uint8_t visual;              /* ASCII char: 'S', 'B', etc. */
    ActorSpriteKind sprite_kind; /* how this actor is drawn (from def) */
    uint8_t spawn_x;             /* patrol anchor origin */
    uint8_t spawn_y;
    uint8_t ai_type;             /* ActorAiType */
    uint8_t ai_step;             /* step index in patrol cycle */
    uint8_t ai_timer;            /* countdown to next patrol step */
    uint8_t move_state;          /* 0 = idle, 1 = moving between tiles */
    uint8_t move_target_x;
    uint8_t move_target_y;
    uint8_t move_progress;       /* 0..7 sub-tile pixels */
    uint8_t battle_type;         /* BattleId */
} WorldActorRuntime;

typedef struct {
    uint8_t width;
    uint8_t height;
    MapId map_id;
    uint8_t encounter_actor_index;   /* slot in actors[], or NO_ACTOR_INDEX */
    bool map_changed;
    Entity player;
    WorldActorRuntime actors[MAX_WORLD_ACTORS];
    uint8_t map[WORLD_HEIGHT][WORLD_WIDTH];

    /* Overworld camera in PIXELS (top-left of the view window).  The camera
     * follows the player's pixel position smoothly (world_update_scroll);
     * SCX/SCY are set from these each frame and the tilemap window is drawn
     * around scroll_x/y (= camera_px/8).  Runtime only, never persistent. */
    uint8_t camera_px_x;
    uint8_t camera_px_y;

    /* Overworld camera tile origin (camera_px/8), the top-left tile of the
     * view window into the scene.  Derived by world_update_scroll and
     * exposed to the snapshot as scroll_x/scroll_y. */
    uint8_t scroll_x;
    uint8_t scroll_y;

    /* Movement animation state (runtime only, never persistent).  When
     * move_state == MOVE_STATE_MOVING, the player is animating from
     * player.position toward move_target over MOVE_FRAMES; the tile
     * position only commits at the end of the walk.  The renderer derives
     * the sub-tile pixel position from (position, target, progress). */
    uint8_t move_state;      /* MoveState */
    uint8_t move_target_x;   /* target tile committed at the end of the walk */
    uint8_t move_target_y;
    uint8_t move_progress;   /* 0..MOVE_FRAMES, sub-tile pixel offset */
    uint8_t move_outcome;    /* MoveOutcome resolved at commit */
} World;

void world_init(World *w, const GameState *state);
void world_load_map(World *w, MapId map_id, const GameState *state);
void world_change_map(World *w, MapId map_id, uint8_t spawn_x, uint8_t spawn_y,
                      const GameState *state);
bool world_is_walkable(const World *w, uint8_t x, uint8_t y);

/* Overworld camera: keep the player centred in the WORLD_VIEW_W x
 * WORLD_VIEW_H view, clamped at the scene bounds (scenes smaller than the
 * view never scroll; near a scene edge the player moves off-centre while
 * the camera stays at the boundary).  The camera glides smoothly in pixels
 * (SCX/SCY); scroll_x/y (= camera_px/8) drive the tile window and the
 * snapshot.  Called once per overworld frame. */
void world_update_scroll(World *w);

/* Animated movement: world_try_begin_move validates the target and starts
 * the MOVE_FRAMES animation (returns BLOCKED if the tile cannot be walked);
 * world_update_move advances it one frame and resolves the commit (tile
 * move, exit, or encounter).  Returns a WorldMoveResult for the caller.
 * The renderer derives the player's pixel position from the move state via
 * world_player_px/player_py. */
WorldMoveResult world_try_begin_move(World *w, int8_t dx, int8_t dy,
                                     const GameState *state);
WorldMoveResult world_update_move(World *w, const GameState *state);

/* Advance autonomous patrol AI for all active hostile actors in the scene.
 * If an actor steps into the player, sets encounter_actor_index and returns
 * MOVE_RESULT_ENCOUNTER. */
WorldMoveResult world_update_actors(World *w);

/* Renderer pixel position (tile*8 plus the sub-tile walk progress) of the
 * player sprite.  Valid whenever the player is not animating a move. */
/* Bank-3 pixel-interpolation body (src/world/px_banked.c); dispatched by
 * the four world_*_px/py wrappers. */
void world_px_banked(void);

uint8_t world_player_px(const World *w);
uint8_t world_player_py(const World *w);

uint8_t world_actor_px(const WorldActorRuntime *a);
uint8_t world_actor_py(const WorldActorRuntime *a);

void world_on_battle_end(Game *g, bool victory);
void world_patrol_slot_banked(void);

/* End a battle by fleeing: the enemy stays on the map at the HP it had when
 * the hero ran (written back into the runtime actor); no reward, defeat or
 * quest progress is applied. */
void world_on_battle_fled(Game *g);
/* Bank-2 body (src/world/fled_banked.c) dispatched by the wrapper. */
void world_on_battle_fled_banked(void);

#endif /* WORLD_H */

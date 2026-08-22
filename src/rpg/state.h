#ifndef RPG_STATE_H
#define RPG_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include "entity.h"
#include "screen.h"
#include "rpg/deck.h"

/* Fixed-size capacity limits for the Game Boy. No dynamic allocation. */
#define MAX_PARTY_MEMBERS       4
#define MAX_STATE_FLAGS         64
#define MAX_STATE_VARIABLES     16
#define MAX_PERSISTENT_ACTORS   16
#define MAX_CURRENCIES          4
#define MAX_PROGRESSION_TARGETS 8

/* Generic, saveable identifiers. The game layer decides what a given
 * ID means; the state layer only stores and retrieves it. */
typedef uint16_t FlagId;
typedef uint16_t VariableId;
typedef uint16_t ActorId;
typedef uint16_t CurrencyId;

/* Party member identity. */
typedef enum {
    CHARACTER_NONE = 0,
    CHARACTER_HERO = 1
} CharacterId;

/* Persistent actor lifecycle state. */
typedef enum {
    ACTOR_STATE_ALIVE = 0,
    ACTOR_STATE_DEFEATED = 1
} ActorStateId;

/* Mutable persistent scene/position state. */
typedef struct {
    SceneId scene_id;
    uint8_t player_x;
    uint8_t player_y;
    uint8_t player_facing;
} SceneState;

typedef struct {
    CharacterId id;
    uint8_t hp;
    uint8_t max_hp;
} CharacterState;

typedef struct {
    CharacterState members[MAX_PARTY_MEMBERS];
    uint8_t count;
} PartyState;

/* Bit-packed flag storage; flag N lives in byte (N-1)/8, bit (N-1)%8. */
typedef struct {
    uint8_t bytes[MAX_STATE_FLAGS / 8];
} FlagState;

typedef struct {
    int16_t values[MAX_STATE_VARIABLES];
} VariableState;

typedef struct {
    int16_t amount[MAX_CURRENCIES];
} CurrencyState;

/* Progression targets are generic and resolved to static game content. */
typedef enum {
    PROG_TYPE_NONE = 0,
    PROG_TYPE_HERO = 1,
    PROG_TYPE_CARD = 2,
    PROG_TYPE_COMPANION = 3
} ProgressionTargetType;

typedef struct {
    uint8_t type;
    uint16_t id;
} ProgressionTarget;

typedef struct {
    uint8_t level;
    uint16_t progress;
} ProgressionState;

typedef struct {
    ProgressionTarget target;
    ProgressionState state;
} ProgressionEntry;

typedef struct {
    uint8_t count;
    ProgressionEntry entries[MAX_PROGRESSION_TARGETS];
} ProgressionStore;

/* Persistent world actor state: survives scene reloads. */
typedef struct {
    ActorId actor_id;
    uint8_t state;
} PersistentActorState;

typedef struct {
    PersistentActorState actors[MAX_PERSISTENT_ACTORS];
    uint8_t count;
} WorldState;

/* Canonical, potentially-saveable persistent game state. */
typedef struct {
    SceneState scene;
    PartyState party;
    CardState cards;
    FlagState flags;
    VariableState variables;
    CurrencyState currency;
    WorldState world;
    ProgressionStore progression;
} GameState;

void game_state_zero(GameState *state);

bool game_flag_is_set(const GameState *state, FlagId flag);
void game_flag_set(GameState *state, FlagId flag);
void game_flag_clear(GameState *state, FlagId flag);

int16_t game_variable_get(const GameState *state, VariableId variable);
void game_variable_set(GameState *state, VariableId variable, int16_t value);
void game_variable_add(GameState *state, VariableId variable, int16_t amount);

bool game_world_actor_is_defeated(const GameState *state, ActorId actor_id);
void game_world_set_actor_state(GameState *state, ActorId actor_id,
                                ActorStateId actor_state);

#endif /* RPG_STATE_H */

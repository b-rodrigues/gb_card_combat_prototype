#include "story.h"
#include "rpg/state.h"

void story_init(uint8_t flag_count)
{
    (void)flag_count;
}

bool story_has_flag(const GameState *state, FlagId flag_id)
{
    return game_flag_is_set(state, flag_id);
}

void story_set_flag(GameState *state, FlagId flag_id)
{
    game_flag_set(state, flag_id);
}

void story_clear_flag(GameState *state, FlagId flag_id)
{
    game_flag_clear(state, flag_id);
}

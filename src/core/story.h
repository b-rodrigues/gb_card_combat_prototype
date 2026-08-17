#ifndef STORY_H
#define STORY_H

#include <stdint.h>
#include <stdbool.h>
#include "rpg/state.h"

/* Story flags are a named sub-set of GameState.flags (FlagId).  The engine
 * is generic: the exclusive upper bound on valid flag ids is provided by the
 * game layer via story_init() (e.g. STORY_FLAG_ID_COUNT in game_ids.h). */

#define story_init(flag_count) ((void)0)
#define story_has_flag(state, flag_id) game_flag_is_set((state), (flag_id))
#define story_set_flag(state, flag_id) game_flag_set((state), (flag_id))
#define story_clear_flag(state, flag_id) game_flag_clear((state), (flag_id))

#endif /* STORY_H */

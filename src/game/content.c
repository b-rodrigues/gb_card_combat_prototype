#include "content.h"
#include "game_ids.h"
#include "story.h"
#include "event.h"
#include "dialogue.h"
#include "actor.h"
#include "rpg/party.h"
#include "rpg/deck.h"
#include "rpg/loot.h"
#include "core/game.h"
#include "banked.h"

#define HERO_START_HP    10
#define HERO_START_GOLD  20

/* ── Victory loot drop (docs/loot.md §17/§34.5) ─────────────────────
 * Thin fixed-bank wrapper: the whole decision (gate, profile pick,
 * roll, encode) runs as ONE bank-2 body (loot_drop_banked.c); only
 * this staging call stays fixed.  Returns the derived CardId, or 0 =
 * no drop this victory. */
uint8_t game_loot_drop(uint8_t battle_type)
{
    g_bk_call_bank = 2;
    g_bk_call_target = (uint16_t)&game_loot_drop_banked;
    g_bk_byte_a = battle_type;
    banked_call_run();
    return g_loot_id;
}

void game_content_init(void)
{
    story_init(STORY_FLAG_ID_COUNT);
    game_events_register();
    game_dialogue_register();
    game_actors_register();
    game_cards_register();
    game_quest_register();
}

void game_new_game(GameState *state)
{
    if (!state) return;
    game_state_zero(state);

    state->scene.scene_id = SCENE_FIELD;
    state->scene.player_x = 4;
    state->scene.player_y = 4;
    state->scene.player_facing = (uint8_t)DIRECTION_DOWN;

    state->party.count = 1;
    state->party.members[0].id = CHARACTER_HERO;
    state->party.members[0].hp = HERO_START_HP;
    state->party.members[0].max_hp = HERO_START_HP;

    state->variables.values[VARIABLE_ID_CHAPTER - 1] = 1;
    state->currency.amount[CURRENCY_ID_GOLD - 1] = HERO_START_GOLD;

    /* Starter deck (docs/deck-management.md §1): 10 cards — 4x SW3, 3x SH2,
     * 3x FI4.  The original five are decked first so the opening battle hand
     * (SW SW SH SH FI) is unchanged; the extras only deepen the draw pile.
     * Granted as real owned state via the silent mutators so battles draw
     * from the player's actual deck from turn one. */
    deck_collection_add(&state->cards, CARD_IRON_SWORD, 4);
    deck_collection_add(&state->cards, CARD_WOODEN_SHIELD, 3);
    deck_collection_add(&state->cards, CARD_FIRE_TOME, 3);
    deck_collection_add(&state->cards, CARD_POISON_DAGGER, 2);
    deck_add_card(&state->cards, CARD_IRON_SWORD);
    deck_add_card(&state->cards, CARD_IRON_SWORD);
    deck_add_card(&state->cards, CARD_WOODEN_SHIELD);
    deck_add_card(&state->cards, CARD_WOODEN_SHIELD);
    deck_add_card(&state->cards, CARD_FIRE_TOME);
    deck_add_card(&state->cards, CARD_IRON_SWORD);
    deck_add_card(&state->cards, CARD_WOODEN_SHIELD);
    deck_add_card(&state->cards, CARD_FIRE_TOME);
    deck_add_card(&state->cards, CARD_FIRE_TOME);
    deck_add_card(&state->cards, CARD_POISON_DAGGER);
    deck_add_card(&state->cards, CARD_POISON_DAGGER);
    deck_add_card(&state->cards, CARD_IRON_SWORD);
}

void game_on_level_up(GameState *state, ProgressionTarget target,
                      const ProgressionAddResult *result)
{
    CharacterState *hero;
    uint8_t gained;

    if (!state || !result || !result->crossed) return;
    gained = (uint8_t)(result->level_after - result->level_before);
    if (gained == 0) return;

    if (target.type == PROG_TYPE_HERO) {
        hero = party_get_member(&state->party, CHARACTER_HERO);
        if (hero) {
            hero->max_hp = (uint8_t)(hero->max_hp + (gained << 1));
            hero->hp = hero->max_hp;
        }
    }
}

ScreenId game_screen_after_victory(const Game *g)
{
    if (!g) return SCREEN_OVERWORLD;
    if (game_variable_get(&g->state, VARIABLE_ID_ENDING_SHOWN) != 0) {
        return SCREEN_ENDING;
    }
    return SCREEN_OVERWORLD;
}

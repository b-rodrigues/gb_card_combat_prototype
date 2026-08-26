#include "game.h"
#include "screen.h"
#include "battle.h"
#include "telemetry.h"
#include "audio.h"
#include "content.h"
#include "ui.h"
#include "rpg/cards.h"
#include "rpg/loot.h"

void battle_screen_update(Game *g)
{
    /* SDCC pointer-cache workaround (see battle_update in battle.c): the
     * optimizer reuses branch-local cached &g->field stack slots across
     * joins, producing wild stores/calls when a different path filled the
     * slot.  volatile forces every access through a real address. */
    volatile Game *vg = g;
    Battle *b;
    if (!g) return;
    b = &g->battle;

    if (b->battle_over) {
        /* Grant combat loot exactly once, when the VICTORY screen first
         * appears (docs/loot.md §34.5) -- the drop was ROLLED at battle
         * start and waits in g_loot_id.  world_on_battle_end no-ops on
         * repeat calls (encounter index cleared); the exit path calls it
         * again harmlessly.  card_get_def re-populates g_card_scratch so
         * the bank-2 HUD can render the identity name.  No drop ->
         * silence. */
        if (b->result == BATTLE_RESULT_VICTORY &&
            vg->world.encounter_actor_index != NO_ACTOR_INDEX) {
            world_on_battle_end(g, true);
            if (g_loot_id != CARD_NONE) {
                (void)card_get_def(g_loot_id);
                b->msg_id = 4;      /* never expires during RESULT */
                b->msg_ttl = 0;
                b->dirty |= BATTLE_DIRTY_MSG;
            }
        }
        if (input_pressed(INPUT_A) || input_pressed(INPUT_START)) {
            if (b->result == BATTLE_RESULT_VICTORY || b->result == BATTLE_RESULT_FLED) {
                vg->world.player.hp = b->player.hp;
                vg->state.party.members[0].hp = b->player.hp;
                audio_play_music(MUSIC_OVERWORLD);
                telemetry_emit(EVENT_MUSIC_CHANGED, MUSIC_OVERWORLD, 0, 0, 0);
                if (b->result == BATTLE_RESULT_VICTORY) {
                    world_on_battle_end(g, true);
                    screen_change(g, game_screen_after_victory(g));
                } else {
                    world_on_battle_fled(g);
                    screen_change(g, SCREEN_OVERWORLD);
                }
            } else {
                world_on_battle_end(g, false);
                vg->game_over_choice = 0;
                screen_change(g, SCREEN_GAME_OVER);
            }
        }
        return;
    }

    if (b->phase == BATTLE_PHASE_PLAYER_SELECT ||
        b->phase == BATTLE_PHASE_PLAYER_DEFEND) {
        if (input_pressed(INPUT_START)) {
            item_screen_reset(g);
            screen_change(g, SCREEN_ITEM);
        } else if (input_pressed(INPUT_LEFT)) {
            battle_cursor_move(b, -1);
        } else if (input_pressed(INPUT_RIGHT)) {
            battle_cursor_move(b, 1);
        } else if (input_pressed(INPUT_UP)) {
            battle_target_move(b, -1);
        } else if (input_pressed(INPUT_DOWN)) {
            battle_target_move(b, 1);
        } else if (input_pressed(INPUT_A)) {
            battle_card_select(b);
        } else if (input_pressed(INPUT_B)) {
            battle_card_undo(b);
        } else if (input_pressed(INPUT_SELECT)) {
            battle_execute_combo(b);
        }
    }

    battle_update(b);
}

void battle_screen_render(Game *g)
{
    /* SDCC pointer-cache workaround: see battle_screen_update above. */
    volatile Game *vg = g;
    RenderCache *rc;
    uint8_t timer_bar;
    Battle *b;

    if (!g) return;
    rc = &g->render_cache;
    b = &g->battle;
    timer_bar = ui_calc_timer_bar(b->timer_ticks);

    if (!rc->valid || rc->prev_screen != SCREEN_BATTLE) {
        ui_lcd_off();
        ui_draw_battle_full(b);
        ui_lcd_on();
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)SCREEN_BATTLE, 0, 0, 0);
        vg->render_cache.valid = true;
        vg->render_cache.prev_screen = SCREEN_BATTLE;
    } else if (b->dirty) {
        ui_update_battle(b);
    } else if (vg->render_cache.prev_battle_timer_bar != timer_bar) {
        ui_draw_battle_timer(b);
    }
    b->dirty = 0;
    vg->render_cache.prev_battle_timer_bar = timer_bar;
}

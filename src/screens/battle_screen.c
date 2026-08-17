#include "game.h"
#include "screen.h"
#include "battle.h"
#include "telemetry.h"
#include "audio.h"
#include "content.h"
#include "ui.h"

void battle_screen_update(Game *g)
{
    if (!g) return;

    if (g->battle.battle_over) {
        if (input_pressed(INPUT_A) || input_pressed(INPUT_START)) {
            if (g->battle.result == BATTLE_RESULT_VICTORY || g->battle.result == BATTLE_RESULT_FLED) {
                g->world.player.hp = g->battle.player.hp;
                g->state.party.members[0].hp = g->battle.player.hp;
                audio_play_music(MUSIC_OVERWORLD);
                telemetry_emit(EVENT_MUSIC_CHANGED, MUSIC_OVERWORLD, 0, 0, 0);
                if (g->battle.result == BATTLE_RESULT_VICTORY) {
                    world_on_battle_end(g, true);
                    screen_change(g, game_screen_after_victory(g));
                } else {
                    world_on_battle_fled(g);
                    screen_change(g, SCREEN_OVERWORLD);
                }
            } else {
                world_on_battle_end(g, false);
                g->game_over_choice = 0;
                screen_change(g, SCREEN_GAME_OVER);
            }
        }
        return;
    }

    if (g->battle.phase == BATTLE_PHASE_PLAYER_SELECT ||
        g->battle.phase == BATTLE_PHASE_PLAYER_DEFEND) {
        if (input_pressed(INPUT_START)) {
            g->item_menu_index = 0;
            g->item_menu_tab = 0;
            screen_change(g, SCREEN_ITEM);
        } else if (input_pressed(INPUT_LEFT)) {
            battle_cursor_move(&g->battle, -1);
        } else if (input_pressed(INPUT_RIGHT)) {
            battle_cursor_move(&g->battle, 1);
        } else if (input_pressed(INPUT_UP)) {
            battle_target_move(&g->battle, -1);
        } else if (input_pressed(INPUT_DOWN)) {
            battle_target_move(&g->battle, 1);
        } else if (input_pressed(INPUT_A)) {
            battle_card_select(&g->battle);
        } else if (input_pressed(INPUT_B)) {
            battle_card_undo(&g->battle);
        } else if (input_pressed(INPUT_SELECT)) {
            battle_execute_combo(&g->battle);
        }
    }

    battle_update(&g->battle);
}

void battle_screen_render(Game *g)
{
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
        rc->valid = true;
        rc->prev_screen = SCREEN_BATTLE;
    } else if (b->dirty) {
        ui_lcd_off();
        ui_update_battle(b);
        ui_lcd_on();
    } else if (rc->prev_battle_timer_bar != timer_bar) {
        ui_draw_battle_timer(b);
    }
    b->dirty = 0;
    rc->prev_battle_timer_bar = timer_bar;
}

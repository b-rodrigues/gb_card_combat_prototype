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

static uint8_t calc_timer_bar(uint16_t t)
{
    uint8_t bar = 0;
    while (t > 0) {
        bar++;
        if (t <= BATTLE_TIMER_BAR_DIVISOR) break;
        t -= BATTLE_TIMER_BAR_DIVISOR;
    }
    return (bar > 20) ? 20 : bar;
}

void battle_screen_render(Game *g)
{
    RenderCache *rc;
    uint8_t timer_bar;
    uint8_t content_dirty;

    if (!g) return;
    rc = &g->render_cache;
    timer_bar = calc_timer_bar(g->battle.timer_ticks);

    content_dirty = (rc->prev_battle_phase != g->battle.phase ||
                     rc->prev_player_hp != g->battle.player.hp ||
                     rc->prev_enemy_hp != g->battle.enemy.hp ||
                     rc->prev_battle_result != g->battle.result ||
                     rc->prev_battle_cursor != g->battle.cursor_pos ||
                     rc->prev_battle_combo_count != g->battle.combo_count);

    if (!rc->valid || rc->prev_screen != SCREEN_BATTLE) {
        ui_lcd_off();
        ui_draw_battle_full(&g->battle);
        ui_lcd_on();
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)SCREEN_BATTLE, 0, 0, 0);
        rc->valid = true;
        rc->prev_screen = SCREEN_BATTLE;
    } else if (content_dirty) {
        ui_update_battle(&g->battle);
    } else if (rc->prev_battle_timer_bar != timer_bar) {
        /* Only the countdown bar moved -- redraw just that row
         * incrementally, without toggling the LCD off/on (the old full
         * redraw on every tick made the screen black), so the timer
         * drains smoothly. */
        ui_draw_battle_timer(&g->battle);
    } else {
        return;
    }

    rc->prev_battle_turn = g->battle.turn;
    rc->prev_battle_phase = g->battle.phase;
    rc->prev_player_hp = g->battle.player.hp;
    rc->prev_enemy_hp = g->battle.enemy.hp;
    rc->prev_battle_result = g->battle.result;
    rc->prev_battle_cursor = g->battle.cursor_pos;
    rc->prev_battle_combo_count = g->battle.combo_count;
    rc->prev_battle_timer_bar = timer_bar;
}

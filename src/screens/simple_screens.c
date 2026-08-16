#include "game.h"
#include "screen.h"
#include "telemetry.h"
#include "ui.h"

void thanks_screen_update(Game *g)
{
    (void)g;
}

void thanks_screen_render(Game *g)
{
    RenderCache *rc;
    if (!g) return;
    rc = &g->render_cache;

    if (!rc->valid || rc->prev_screen != SCREEN_THANKS) {
        ui_draw_thanks();
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)SCREEN_THANKS, 0, 0, 0);
        rc->valid = true;
        rc->prev_screen = SCREEN_THANKS;
    }
}

void ending_screen_update(Game *g)
{
    if (g && (input_pressed(INPUT_A) || input_pressed(INPUT_START))) {
        game_restart(g);
    }
}

void ending_screen_render(Game *g)
{
    RenderCache *rc;
    if (!g) return;
    rc = &g->render_cache;
    if (!rc->valid || rc->prev_screen != SCREEN_ENDING) {
        ui_clear_screen();
        ui_draw_text_line(0, 1, "      THE END       ", 20);
        ui_draw_hline(2, '-');
        ui_draw_text_line(0, 4, "The Hero cleared", 16);
        ui_draw_text_line(0, 5, "the land of slimes!", 19);
        ui_draw_text_line(0, 7, "Peace has returned!", 19);
        ui_draw_text_line(0, 9, "Thanks for playing!", 19);
        ui_draw_text_line(0, 14, "[A] RESTART", 11);
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)SCREEN_ENDING, 0, 0, 0);
        rc->valid = true;
        rc->prev_screen = SCREEN_ENDING;
    }
}

void game_over_screen_update(Game *g)
{
    if (!g) return;

    if (input_pressed(INPUT_UP) || input_pressed(INPUT_DOWN)) {
        g->game_over_choice = !g->game_over_choice;
    }
    if (input_pressed(INPUT_A) || input_pressed(INPUT_START)) {
        if (g->game_over_choice == 0) {
            game_restart(g);
        } else {
            screen_change(g, SCREEN_THANKS);
        }
    }
}

void game_over_screen_render(Game *g)
{
    RenderCache *rc;
    if (!g) return;
    rc = &g->render_cache;

    if (!rc->valid || rc->prev_screen != SCREEN_GAME_OVER ||
        g->game_over_choice != rc->prev_game_over_choice) {
        ui_draw_game_over(g->game_over_choice);
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)SCREEN_GAME_OVER, 0, 0, 0);
        rc->valid = true;
        rc->prev_screen = SCREEN_GAME_OVER;
        rc->prev_game_over_choice = g->game_over_choice;
    }
}

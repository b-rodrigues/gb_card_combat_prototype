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
        ui_clear_screen();
        ui_draw_text_line(1, 8, "THANKS FOR PLAYING!", 19);
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
    uint8_t i;
    static const char * const s_lines[7] = {
        "      THE END",
        "--------------------",
        "The Hero cleared",
        "the land of slimes!",
        "Peace has returned!",
        "Thanks for playing!",
        "[A] RESTART"
    };
    static const uint8_t s_rows[7] = { 1, 2, 4, 5, 7, 9, 14 };

    if (!g) return;
    rc = &g->render_cache;
    if (!rc->valid || rc->prev_screen != SCREEN_ENDING) {
        ui_clear_screen();
        for (i = 0; i < 7; i++) {
            ui_draw_text_line(0, s_rows[i], s_lines[i], 20);
        }
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
        ui_clear_screen();
        ui_draw_text_line(4, 3, "GAME OVER", 9);
        ui_draw_text_line(3, 8, "CONTINUE?", 9);
        ui_draw_text_line(0, 11, (g->game_over_choice == 0) ? "> YES" : "  YES", 5);
        ui_draw_text_line(0, 12, (g->game_over_choice != 0) ? "> NO" : "  NO", 4);
        ui_draw_text_line(1, 16, "[A] CONFIRM", 11);
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)SCREEN_GAME_OVER, 0, 0, 0);
        rc->valid = true;
        rc->prev_screen = SCREEN_GAME_OVER;
        rc->prev_game_over_choice = g->game_over_choice;
    }
}

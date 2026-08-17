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

static const char * const s_ending_text[6] = {
    "The Hero cleared",
    "the land of slimes!",
    "",
    "Peace has returned!",
    "",
    "Thanks for playing!"
};

void ending_screen_render(Game *g)
{
    RenderCache *rc;
    uint8_t i;
    if (!g) return;
    rc = &g->render_cache;
    if (!rc->valid || rc->prev_screen != SCREEN_ENDING) {
        ui_clear_screen();
        ui_draw_text_line(6, 1, "THE END", 7);
        ui_draw_hline(2, '-');
        for (i = 0; i < 6; i++) {
            ui_draw_text_line(0, (uint8_t)(4 + i), s_ending_text[i], 19);
        }
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

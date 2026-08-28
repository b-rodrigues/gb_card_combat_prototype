#include "game.h"
#include "screen.h"
#include "telemetry.h"
#include "ui.h"
#include "audio.h"
#include "banked.h"

/* ASCII intro sequence shown when starting a NEW GAME from the title
 * screen.  Three scripted slides (the waking whale, the closed sky, the
 * slime blight); A/START advances, the final slide drops into a fresh
 * overworld playthrough.  Rendering lives in the bank-2 intro_content
 * body (title_content.c) to keep the fixed bank small. */

#define INTRO_SLIDE_COUNT 3

void intro_screen_render(Game *g)
{
    RenderCache *rc;
    if (!g) return;
    rc = &g->render_cache;
    if (!rc->valid || rc->prev_screen != SCREEN_INTRO) {
        ui_clear_screen();
        g_bk_call_bank = 2;
        g_bk_call_target = (uint16_t)&intro_content_render;
        g_bk_byte_a = g->intro_slide;
        banked_call_run();
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)SCREEN_INTRO, 0, 0, 0);
        rc->valid = true;
        rc->prev_screen = SCREEN_INTRO;
    }
}

void intro_screen_update(Game *g)
{
    if (!g) return;
    if (input_pressed(INPUT_A) || input_pressed(INPUT_START)) {
        audio_play_sfx(0);
        if (g->intro_slide + 1 >= INTRO_SLIDE_COUNT) {
            g->intro_slide = 0;
            game_restart(g);
        } else {
            g->intro_slide++;
            /* Force a redraw of the new slide. */
            g->render_cache.valid = false;
        }
    }
}

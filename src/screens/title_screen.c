#include "game.h"
#include "screen.h"
#include "telemetry.h"
#include "ui.h"
#include "audio.h"
#include "banked.h"

/* Classic title screen.  The heavy logo/menu rendering lives in a bank-2
 * body (banked.h constraint, AGENTS.md 52.11.1): the fixed _CODE/_HOME
 * area is at its size limit (see make memmap), so this file only runs a
 * thin input/transition wrapper around title_content_render().
 *
 * Flow: PRESS START -> NEW GAME / CONTINUE / SOUND.  SOUND toggles
 * g_sound_enabled (ON/OFF).  NEW GAME -> intro.  CONTINUE -> SAVE_LOAD
 * (load mode). */

void title_screen_render(Game *g)
{
    RenderCache *rc;
    if (!g) return;
    rc = &g->render_cache;
    if (!rc->valid || rc->prev_screen != SCREEN_TITLE) {
        ui_clear_screen();
        g_bk_call_bank = 2;
        g_bk_call_target = (uint16_t)&title_content_render;
        g_bk_byte_a = g->title_menu_showing;
        g_bk_byte_b = g->title_menu_index;
        banked_call_run();
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)SCREEN_TITLE, 0, 0, 0);
        rc->valid = true;
        rc->prev_screen = SCREEN_TITLE;
    }
}

void title_screen_update(Game *g)
{
    uint8_t max_idx = 2;

    if (!g) return;

    if (!g->title_menu_showing) {
        /* PRESS START splash: any start/a advances to the menu.  The
         * prompt blinks on the frame counter (every ~32 frames). */
        if (input_pressed(INPUT_START) || input_pressed(INPUT_A)) {
            g->title_menu_showing = 1;
            g->title_menu_index = 0;
            audio_play_sfx(0);
        }
        return;
    }

    if (input_pressed(INPUT_UP)) {
        if (g->title_menu_index == 0) g->title_menu_index = max_idx;
        else g->title_menu_index--;
        audio_play_sfx(0);
    }
    if (input_pressed(INPUT_DOWN)) {
        if (g->title_menu_index == max_idx) g->title_menu_index = 0;
        else g->title_menu_index++;
        audio_play_sfx(0);
    }

    if (input_pressed(INPUT_A) || input_pressed(INPUT_START)) {
        switch (g->title_menu_index) {
        case 0: /* NEW GAME */
            audio_play_sfx(1);
            telemetry_emit(EVENT_NEW_GAME_STARTED, 0, 0, 0, 0);
            g->intro_slide = 0;
            screen_change(g, SCREEN_INTRO);
            break;
        case 1: /* CONTINUE */
            audio_play_sfx(1);
            telemetry_emit(EVENT_GAME_CONTINUED, 0, 0, 0, 0);
            g->save_slot_mode = 0; /* LOAD */
            g->save_slot_index = 0;
            screen_change(g, SCREEN_SAVE_LOAD);
            break;
        default: /* SOUND */
            g_sound_enabled = ~g_sound_enabled & 1;
            audio_play_sfx(0);
            telemetry_emit(EVENT_SOUND_TOGGLED, g_sound_enabled, 0, 0, 0);
            break;
        }
    }
}

#include "game.h"
#include "screen.h"

/* Studio splash shown once at boot, before the title screen.  The heavy
 * text draw lives in the bank-4 splash_content body (title_content.c) to
 * keep the fixed _CODE/_HOME bank small (AGENTS.md 52.11.1); the render
 * wrapper reuses the shared slide renderer (intro_screen.c).  The splash
 * auto-advances after SPLASH_FRAMES, or immediately on START/A. */
#define SPLASH_FRAMES 150

void splash_screen_render(Game *g)
{
    slide_screen_render(g, SCREEN_SPLASH, (uint16_t)&splash_content_render, 0);
}

void splash_screen_update(Game *g)
{
    if (!g) return;
    if (g->frame >= SPLASH_FRAMES ||
        input_pressed(INPUT_START) || input_pressed(INPUT_A)) {
        screen_change(g, SCREEN_TITLE);
    }
}

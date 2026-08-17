#include "game.h"
#include "screen.h"
#include "dialogue.h"
#include "interaction.h"
#include "telemetry.h"

void dialogue_screen_update(Game *g)
{
    if (!g || !g->dialogue.active) return;

    if (input_pressed(INPUT_A) || input_pressed(INPUT_START)) {
        if (!dialogue_next(&g->dialogue)) {
            interaction_on_dialogue_end(&g->dialogue, &g->state);
            screen_change(g, SCREEN_OVERWORLD);
        }
    }
}

void dialogue_screen_render(Game *g)
{
    RenderCache *rc;
    uint8_t first_enter;

    if (!g) return;
    rc = &g->render_cache;

    first_enter = (!rc->valid || rc->prev_screen != SCREEN_DIALOGUE);
    if (!first_enter &&
        g->dialogue.current_line == rc->prev_dialogue_line &&
        g->dialogue.id == rc->prev_dialogue_id) {
        return;
    }

    ui_lcd_off();
    if (first_enter && rc->prev_screen != SCREEN_OVERWORLD) {
        ui_draw_world_full(&g->world);
    }
    ui_draw_dialogue(&g->dialogue, g->world.scroll_x, g->world.scroll_y);
    ui_hud_hide();
    ui_lcd_on();

    if (first_enter) {
        ui_sprite_move((uint8_t)(world_player_px(&g->world) - g->world.camera_px_x),
                       (uint8_t)(world_player_py(&g->world) - g->world.camera_px_y));
        ui_draw_actors_sprites(&g->world);
        rc->valid = true;
        rc->prev_screen = SCREEN_DIALOGUE;
        rc->prev_dialogue_active = true;
    }

    telemetry_emit(EVENT_RENDER_DIALOGUE, (uint8_t)g->dialogue.id,
                   g->dialogue.current_line, 0, 0);
    rc->prev_dialogue_line = g->dialogue.current_line;
    rc->prev_dialogue_id = g->dialogue.id;
}

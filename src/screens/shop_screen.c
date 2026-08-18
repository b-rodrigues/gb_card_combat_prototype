#include "game.h"
#include "screen.h"
#include "telemetry.h"
#include "rpg/items.h"
#include "rpg/currency.h"
#include "menu.h"
#include "shops.h"
#include "game_ids.h"
#include "ui.h"

#define SHOP_MSG_NONE 0
#define SHOP_MSG_BOUGHT 1
#define SHOP_MSG_NO_GOLD 2

/* The active shop is chosen by the actor that was engaged (g->shop_id).
 * Cursor state lives in g->item_menu_index. */
static const ShopDefinition *shop_active(const Game *g)
{
    return game_shop_for_id(g ? g->shop_id : 1);
}

static void shop_draw(Game *g)
{
    const ShopDefinition *def = shop_active(g);
    char str[7];
    uint8_t i, y;

    menu_draw_frame("SHOP");
    ui_draw_text_line(0, 3, "GOLD:", 5);
    ui_format_int(currency_get(&g->state, CURRENCY_ID_GOLD), str);
    ui_draw_text_line(5, 3, str, 14);

    if (!def) {
        ui_draw_text_line(0, 5, "(nothing)", 9);
        ui_draw_text_line(0, 7, "[B] Leave", 9);
        return;
    }

    for (i = 0; i < def->count; i++) {
        const ItemDefinition *item = item_get_def(def->items[i]);
        y = (uint8_t)(5 + i);
        ui_draw_text_line(0, y, (g->item_menu_index == i) ? ">" : " ", 1);
        ui_draw_text_line(1, y, item ? item->name : "???", 10);
        ui_format_int(item ? (int16_t)item->price : 0, str);
        ui_draw_text_line(12, y, str, 4);
        ui_draw_text_line(16, y, "G", 1);
    }

    ui_draw_text_line(0, (uint8_t)(6 + def->count), "[A] Buy  [B] Leave", 18);
    if (g->shop_message != SHOP_MSG_NONE) {
        ui_draw_text_line(0, (uint8_t)(8 + def->count),
                          (g->shop_message == SHOP_MSG_BOUGHT) ? "Bought!" : "Not enough gold!", 16);
    }
}

void shop_screen_update(Game *g)
{
    const ShopDefinition *def;
    uint8_t count;

    if (!g) return;
    def = shop_active(g);
    count = def ? def->count : 0;

    if (count > 0) {
        if (input_pressed(INPUT_UP) && g->item_menu_index > 0) {
            g->item_menu_index--;
            g->render_cache.valid = false;
        } else if (input_pressed(INPUT_DOWN) && (uint8_t)(g->item_menu_index + 1) < count) {
            g->item_menu_index++;
            g->render_cache.valid = false;
        } else if (input_pressed(INPUT_A) && g->item_menu_index < count) {
            ItemId item_id = def->items[g->item_menu_index];
            g->shop_message = (item_purchase(&g->state, item_id) == ITEM_PURCHASE_OK)
                              ? SHOP_MSG_BOUGHT : SHOP_MSG_NO_GOLD;
            g->render_cache.valid = false;
        }
    }
    if (input_pressed(INPUT_B) || input_pressed(INPUT_START)) {
        g->shop_message = SHOP_MSG_NONE;
        g->item_menu_index = 0;
        screen_change(g, SCREEN_OVERWORLD);
    }
}

void shop_screen_render(Game *g)
{
    RenderCache *rc;

    if (!g) return;
    rc = &g->render_cache;

    if (!rc->valid || rc->prev_screen != SCREEN_SHOP) {
        shop_draw(g);
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)SCREEN_SHOP, 0, 0, 0);
        rc->valid = true;
        rc->prev_screen = SCREEN_SHOP;
    }
}

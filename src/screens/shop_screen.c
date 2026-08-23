#include "game.h"
#include "screen.h"
#include "telemetry.h"
#include "rpg/cards.h"
#include "rpg/currency.h"
#include "rpg/deck.h"
#include "menu.h"
#include "shops.h"
#include "game_ids.h"
#include "ui.h"

#define SHOP_MSG_NONE 0
#define SHOP_MSG_BOUGHT 1
#define SHOP_MSG_NO_GOLD 2
#define SHOP_MSG_MAX_COPIES 3

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
        const CardDefinition *card = card_get_def(def->items[i]);
        char code[6];
        y = (uint8_t)(5 + i);
        ui_draw_text_line(0, y, (g->item_menu_index == i) ? ">" : " ", 1);
        if (card) {
            /* Label derives from the card definition, never a string. */
            ui_card_code_str(card->battle_type, card->power, code);
            ui_draw_text_line(1, y, code, 4);
        } else {
            ui_draw_text_line(1, y, "???", 3);
        }
        ui_format_int(card ? (int16_t)card->price : 0, str);
        ui_draw_text_line(12, y, str, 4);
        ui_draw_text_line(16, y, "G", 1);
    }

    ui_draw_text_line(0, (uint8_t)(6 + def->count), "[A] Buy  [B] Leave", 18);
    if (g->shop_message != SHOP_MSG_NONE) {
        ui_draw_text_line(0, (uint8_t)(8 + def->count),
                          (g->shop_message == SHOP_MSG_BOUGHT) ? "Bought!" :
                          (g->shop_message == SHOP_MSG_MAX_COPIES) ? "Too many!" : "Not enough!", 12);
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
            CardId card_id = def->items[g->item_menu_index];
            const CardDefinition *card = card_get_def(card_id);
            uint8_t price = card ? card->price : 0;
            if (currency_get(&g->state, CURRENCY_ID_GOLD) < price) {
                g->shop_message = SHOP_MSG_NO_GOLD;
                telemetry_emit(EVENT_CARD_PURCHASE_FAILED, (uint8_t)card_id, 1, 0, 0);
            } else {
                if (!deck_collection_add(&g->state.cards, card_id, 1)) {
                    g->shop_message = SHOP_MSG_MAX_COPIES;
                    telemetry_emit(EVENT_CARD_PURCHASE_FAILED, (uint8_t)card_id, 2, 0, 0);
                } else {
                    currency_add(&g->state, CURRENCY_ID_GOLD, -(int16_t)price);
                    telemetry_emit(EVENT_CARD_PURCHASED, (uint8_t)card_id, (uint8_t)price, 0, 0);
                    g->shop_message = SHOP_MSG_BOUGHT;
                }
            }
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

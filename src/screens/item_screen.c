#include "game.h"
#include "screen.h"
#include "telemetry.h"
#include "rpg/cards.h"
#include "rpg/deck.h"
#include "quest.h"
#include "ui.h"
#include "menu.h"

#define TAB_CARDS 0
#define TAB_DECK  1
#define TAB_QUEST 2
#define NUM_TABS  3

static void quest_draw_status(Game *g, const QuestDefinition *q, uint8_t y)
{
    QuestStatus st;
    char b[6];
    uint8_t val;
    if (!q) return;
    st = quest_status(&g->state, q);

    if (st == QUEST_STATUS_NOT_STARTED) {
        ui_draw_text_line(0, y, "not started", 11);
    } else if (st == QUEST_STATUS_ACTIVE) {
        if (q->progress_variable != 0) {
            val = (uint8_t)game_variable_get(&g->state, q->progress_variable);
            ui_draw_text_line(0, y, q->progress_label ? q->progress_label : "", 8);
            b[0] = ':';
            b[1] = ' ';
            b[2] = (char)('0' + val);
            b[3] = '/';
            b[4] = (char)('0' + (uint8_t)q->progress_target);
            ui_draw_text_line(8, y, b, 5);
        } else {
            ui_draw_text_line(0, y, "active", 6);
        }
    } else {
        ui_draw_text_line(0, y, "complete - ", 11);
        ui_draw_text_line(11, y, q->complete_note ? q->complete_note : "", 9);
    }
}

static void draw_quest(Game *g)
{
    uint8_t i, y = 7;
    const QuestDefinition *q;
    for (i = 0; i < quest_count() && y < 15; i++) {
        q = quest_at(i);
        if (!q) break;
        ui_draw_text_line(0, y, q->name, 20);
        y++;
        quest_draw_status(g, q, y);
        y++;
    }
    ui_draw_text_line(0, 16, "[B] CLOSE", 10);
}

static void draw_card_detail(Game *g)
{
    const CardDefinition *def;
    CardId id;
    uint8_t n;

    n = (g->item_menu_tab == TAB_CARDS) ?
        g->state.cards.collection.count : g->state.cards.deck.count;
    if (n == 0) return;

    id = (g->item_menu_tab == TAB_CARDS) ?
        g->state.cards.collection.entries[g->item_menu_index].id :
        g->state.cards.deck.cards[g->item_menu_index];
    def = card_get_def(id);
    if (!def) return;

    ui_draw_text_line(0, 14, "PWR", 3);
    ui_draw_num2(4, 14, def->power);
    ui_draw_text_line(8, 14, "COST", 4);
    ui_draw_num2(13, 14, def->cost);
    ui_draw_text_line(16, 14, "U", 1);
    if (def->uses_per_battle == 0)
        ui_draw_text_line(18, 14, "-", 1);
    else
        ui_draw_num2(18, 14, def->uses_per_battle);
}

static void draw_card_list(Game *g)
{
    uint8_t n, i, y;
    const CardDefinition *def;
    CardId id;

    n = (g->item_menu_tab == TAB_CARDS) ?
        g->state.cards.collection.count : g->state.cards.deck.count;
    for (i = 0; i < n && i < 8; i++) {
        id = (g->item_menu_tab == TAB_CARDS) ?
            g->state.cards.collection.entries[i].id : g->state.cards.deck.cards[i];
        def = card_get_def(id);
        y = (uint8_t)(6 + i);
        if (i == g->item_menu_index) ui_draw_text_line(0, y, ">", 1);
        ui_draw_text_line(2, y, def ? def->name : "???", 8);
    }
    draw_card_detail(g);
    if (g->item_menu_tab == TAB_CARDS)
        ui_draw_text_line(0, 15, "[A]ADD  [B]CLOSE", 16);
    else
        ui_draw_text_line(0, 15, "[A]REMOVE [B]CLOSE", 19);
}

void item_screen_update(Game *g)
{
    uint8_t total;

    if (input_pressed(INPUT_SELECT) || input_pressed(INPUT_RIGHT)) {
        g->item_menu_tab = (uint8_t)((g->item_menu_tab + 1) % NUM_TABS);
        g->item_menu_index = 0;
        g->render_cache.valid = false;
        return;
    }
    if (input_pressed(INPUT_LEFT)) {
        g->item_menu_tab = (uint8_t)((g->item_menu_tab + NUM_TABS - 1) % NUM_TABS);
        g->item_menu_index = 0;
        g->render_cache.valid = false;
        return;
    }
    if (input_pressed(INPUT_B)) {
        g->item_menu_index = 0;
        g->item_menu_tab = TAB_CARDS;
        screen_change(g, g->prev_screen);
        return;
    }

    if (g->item_menu_tab == TAB_CARDS) {
        total = g->state.cards.collection.count;
    } else if (g->item_menu_tab == TAB_DECK) {
        total = g->state.cards.deck.count;
    } else {
        return;
    }
    if (total == 0) return;

    if (input_pressed(INPUT_UP)) {
        g->item_menu_index = (g->item_menu_index > 0) ?
            (uint8_t)(g->item_menu_index - 1) : (uint8_t)(total - 1);
        g->render_cache.valid = false;
    } else if (input_pressed(INPUT_DOWN)) {
        g->item_menu_index = (g->item_menu_index < (uint8_t)(total - 1)) ?
            (uint8_t)(g->item_menu_index + 1) : 0;
        g->render_cache.valid = false;
    } else if (input_pressed(INPUT_A)) {
        if (g->item_menu_tab == TAB_CARDS) {
            CardState *cs = &g->state.cards;
            if (g->item_menu_index < cs->collection.count) {
                if (deck_add_card(cs, cs->collection.entries[g->item_menu_index].id))
                    telemetry_emit(EVENT_CARD_ADDED_TO_DECK,
                                   cs->collection.entries[g->item_menu_index].id, 0, 0, 0);
            }
        } else {
            CardState *cs = &g->state.cards;
            if (g->item_menu_index < cs->deck.count) {
                CardId rm = cs->deck.cards[g->item_menu_index];
                if (deck_remove_card(cs, rm)) {
                    telemetry_emit(EVENT_CARD_REMOVED_FROM_DECK, rm, 0, 0, 0);
                    if (g->item_menu_index >= cs->deck.count && g->item_menu_index > 0)
                        g->item_menu_index--;
                }
            }
        }
        g->render_cache.valid = false;
    }
}

void item_screen_render(Game *g)
{
    RenderCache *rc;
    if (!g) return;
    rc = &g->render_cache;
    if (!rc->valid || rc->prev_screen != SCREEN_ITEM) {
        if (g->item_menu_tab == TAB_QUEST) menu_draw_frame("QUESTS");
        else menu_draw_frame("CARDS");
        ui_draw_text_line(0, 2, "CARDS DECKQUEST   ", 17);
        ui_draw_text_line((uint8_t)(g->item_menu_tab * 5), 3, "^", 1);
        if (g->item_menu_tab == TAB_QUEST) draw_quest(g);
        else draw_card_list(g);
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)SCREEN_ITEM, 0, 0, 0);
        rc->valid = true;
        rc->prev_screen = SCREEN_ITEM;
    }
}

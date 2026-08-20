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

#define FILTER_ALL      0xFF
#define SORT_NONE       0
#define SORT_TYPE       1
#define SORT_POWER      2
#define SORT_COST       3
#define SORT_POWER_DESC 4
#define SORT_COST_DESC  5

/* Filtered/sorted view over the active tab's source array.  s_view_indices
 * holds source indices; s_view_count is the number of visible cards. */
static uint8_t s_view_indices[MAX_DECK_CARDS];
static uint8_t s_view_count;

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

/* Source array of the active tab: the collection for CARDS, the deck for
 * DECK.  QUEST has no card list. */
static uint8_t source_count(Game *g)
{
    return (g->item_menu_tab == TAB_CARDS) ?
        g->state.cards.collection.count : g->state.cards.deck.count;
}

static CardId card_id_at(Game *g, uint8_t i)
{
    if (g->item_menu_tab == TAB_CARDS)
        return g->state.cards.collection.entries[i].id;
    return g->state.cards.deck.cards[i];
}

static bool filter_matches(Game *g, CardId id)
{
    const CardDefinition *def;
    if (g->item_menu_filter == FILTER_ALL) return true;
    def = card_get_def(id);
    if (!def) return false;
    return def->type == g->item_menu_filter;
}

static uint8_t sort_value(const CardDefinition *def, uint8_t sort)
{
    switch (sort) {
        case SORT_TYPE:  return def->type;
        case SORT_POWER:
        case SORT_POWER_DESC: return def->power;
        case SORT_COST:
        case SORT_COST_DESC:  return def->cost;
        default: return 0;
    }
}

static void sort_view(Game *g)
{
    uint8_t i, j, tmp, va, vb;
    uint8_t desc = (g->item_menu_sort == SORT_POWER_DESC ||
                    g->item_menu_sort == SORT_COST_DESC);

    for (i = 1; i < s_view_count; i++) {
        tmp = s_view_indices[i];
        va = sort_value(card_get_def(card_id_at(g, tmp)), g->item_menu_sort);
        j = i;
        while (j > 0) {
            vb = sort_value(card_get_def(card_id_at(g, s_view_indices[j - 1])),
                            g->item_menu_sort);
            if (desc ? (vb < va) : (vb > va)) {
                s_view_indices[j] = s_view_indices[j - 1];
                j--;
            } else {
                break;
            }
        }
        s_view_indices[j] = tmp;
    }
}

static void build_view(Game *g)
{
    uint8_t total, i;
    s_view_count = 0;
    total = source_count(g);
    for (i = 0; i < total; i++) {
        if (filter_matches(g, card_id_at(g, i)))
            s_view_indices[s_view_count++] = i;
    }
    if (g->item_menu_sort != SORT_NONE && s_view_count > 1)
        sort_view(g);
}

static void draw_filter_sort(Game *g)
{
    const char *f = "ALL";
    const char *s = "OFF";
    uint8_t fv = g->item_menu_filter;
    uint8_t sv = g->item_menu_sort;

    if (fv == CARD_TYPE_ATTACK) f = "ATK";
    else if (fv == CARD_TYPE_DEFENSE) f = "DEF";
    else if (fv == CARD_TYPE_HEAL) f = "HEL";
    else if (fv == CARD_TYPE_STATUS) f = "STS";
    else if (fv == CARD_TYPE_UTILITY) f = "UTL";

    if (sv == SORT_TYPE) s = "TYPE";
    else if (sv == SORT_POWER) s = "PWR+";
    else if (sv == SORT_COST) s = "CST+";
    else if (sv == SORT_POWER_DESC) s = "PWR-";
    else if (sv == SORT_COST_DESC) s = "CST-";

    ui_draw_text_line(0, 5, "F:", 2);
    ui_draw_text_line(2, 5, f, 3);
    ui_draw_text_line(8, 5, "S:", 2);
    ui_draw_text_line(10, 5, s, 4);
}

static void cycle_filter(Game *g)
{
    if (g->item_menu_filter == FILTER_ALL)
        g->item_menu_filter = CARD_TYPE_ATTACK;
    else if (g->item_menu_filter == CARD_TYPE_ATTACK)
        g->item_menu_filter = CARD_TYPE_DEFENSE;
    else if (g->item_menu_filter == CARD_TYPE_DEFENSE)
        g->item_menu_filter = CARD_TYPE_HEAL;
    else if (g->item_menu_filter == CARD_TYPE_HEAL)
        g->item_menu_filter = CARD_TYPE_STATUS;
    else if (g->item_menu_filter == CARD_TYPE_STATUS)
        g->item_menu_filter = CARD_TYPE_UTILITY;
    else
        g->item_menu_filter = FILTER_ALL;
}

static void cycle_sort(Game *g)
{
    g->item_menu_sort = (uint8_t)((g->item_menu_sort + 1) % (SORT_COST_DESC + 1));
}

static void draw_card_detail(Game *g)
{
    const CardDefinition *def;
    uint8_t idx;

    if (s_view_count == 0 || g->item_menu_index >= s_view_count) return;
    idx = s_view_indices[g->item_menu_index];
    def = card_get_def(card_id_at(g, idx));
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
    uint8_t i, y, src, pos;
    const CardDefinition *def;

    for (i = 0; i < 8; i++) {
        y = (uint8_t)(6 + i);
        pos = (uint8_t)(g->item_menu_scroll + i);
        if (pos < s_view_count) {
            src = s_view_indices[pos];
            def = card_get_def(card_id_at(g, src));
            if (pos == g->item_menu_index) ui_draw_text_line(0, y, ">", 1);
            ui_draw_text_line(2, y, def ? def->name : "???", 8);
        }
    }
    draw_card_detail(g);
    if (g->item_menu_tab == TAB_CARDS)
        ui_draw_text_line(0, 15, "[A]ADD  [B]CLOSE", 16);
    else
        ui_draw_text_line(0, 15, "[A]REMOVE [B]CLOSE", 19);
}

static void scroll_to_index(Game *g)
{
    if (g->item_menu_index < g->item_menu_scroll)
        g->item_menu_scroll = g->item_menu_index;
    else if (g->item_menu_index >= (uint8_t)(g->item_menu_scroll + 8))
        g->item_menu_scroll = (uint8_t)(g->item_menu_index - 7);
}

void item_screen_reset(Game *g)
{
    g->item_menu_index = 0;
    g->item_menu_tab = TAB_CARDS;
    g->item_menu_scroll = 0;
    g->item_menu_filter = FILTER_ALL;
    g->item_menu_sort = SORT_NONE;
}

void item_screen_update(Game *g)
{
    uint8_t total;

    if (input_pressed(INPUT_RIGHT)) {
        g->item_menu_tab = (uint8_t)((g->item_menu_tab + 1) % NUM_TABS);
        g->item_menu_index = 0;
        g->item_menu_scroll = 0;
        g->render_cache.valid = false;
        return;
    }
    if (input_pressed(INPUT_LEFT)) {
        g->item_menu_tab = (uint8_t)((g->item_menu_tab + NUM_TABS - 1) % NUM_TABS);
        g->item_menu_index = 0;
        g->item_menu_scroll = 0;
        g->render_cache.valid = false;
        return;
    }
    if (input_pressed(INPUT_B)) {
        item_screen_reset(g);
        screen_change(g, g->prev_screen);
        return;
    }

    if (g->item_menu_tab == TAB_QUEST) return;

    if (input_pressed(INPUT_SELECT)) {
        cycle_filter(g);
        g->item_menu_index = 0;
        g->item_menu_scroll = 0;
        g->render_cache.valid = false;
        return;
    }
    if (input_pressed(INPUT_START)) {
        cycle_sort(g);
        g->item_menu_index = 0;
        g->item_menu_scroll = 0;
        g->render_cache.valid = false;
        return;
    }

    build_view(g);
    total = s_view_count;
    if (total == 0) return;

    if (input_pressed(INPUT_UP)) {
        g->item_menu_index = (g->item_menu_index > 0) ?
            (uint8_t)(g->item_menu_index - 1) : (uint8_t)(total - 1);
        scroll_to_index(g);
        g->render_cache.valid = false;
    } else if (input_pressed(INPUT_DOWN)) {
        g->item_menu_index = (g->item_menu_index < (uint8_t)(total - 1)) ?
            (uint8_t)(g->item_menu_index + 1) : 0;
        scroll_to_index(g);
        g->render_cache.valid = false;
    } else if (input_pressed(INPUT_A)) {
        if (g->item_menu_tab == TAB_CARDS) {
            CardState *cs = &g->state.cards;
            uint8_t src = s_view_indices[g->item_menu_index];
            if (src < cs->collection.count) {
                if (deck_add_card(cs, cs->collection.entries[src].id))
                    telemetry_emit(EVENT_CARD_ADDED_TO_DECK,
                                   cs->collection.entries[src].id, 0, 0, 0);
            }
        } else {
            CardState *cs = &g->state.cards;
            uint8_t src = s_view_indices[g->item_menu_index];
            if (src < cs->deck.count) {
                CardId rm = cs->deck.cards[src];
                if (deck_remove_card(cs, rm)) {
                    telemetry_emit(EVENT_CARD_REMOVED_FROM_DECK, rm, 0, 0, 0);
                }
            }
            build_view(g);
            if (g->item_menu_index >= s_view_count && g->item_menu_index > 0)
                g->item_menu_index--;
            if (g->item_menu_scroll > 0 &&
                (uint8_t)(g->item_menu_scroll + 8) > s_view_count)
                g->item_menu_scroll = (s_view_count > 8) ?
                    (uint8_t)(s_view_count - 8) : 0;
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
        if (g->item_menu_tab == TAB_QUEST) {
            draw_quest(g);
        } else {
            build_view(g);
            draw_filter_sort(g);
            draw_card_list(g);
        }
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)SCREEN_ITEM, 0, 0, 0);
        rc->valid = true;
        rc->prev_screen = SCREEN_ITEM;
    }
}

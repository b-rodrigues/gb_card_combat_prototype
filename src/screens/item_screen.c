#include "game.h"
#include "screen.h"
#include "telemetry.h"
#include "rpg/cards.h"
#include "rpg/deck.h"
#include "quest.h"
#include "ui.h"
#include "menu.h"
#include "card.h"

#define TAB_CARDS 0
#define TAB_QUEST 1
#define NUM_TABS  2

/* Cursor index 0 on the CARDS tab is the FILTER/SORT row; card entries
 * start at 1. */
#define ROW_TOP     0
#define FIRST_CARD  1
#define VISIBLE_CARDS 5

#define MODE_LIST         0
#define MODE_CARD_DETAIL  1
#define MODE_QUEST_DETAIL 2
#define MODE_PICKER       3

#define MSG_NONE       0
#define MSG_DECK_FULL  1

#define FILTER_ALL      0xFF
#define SORT_NONE       0
#define SORT_TYPE       1
#define SORT_POWER      2
#define SORT_COST       3
#define SORT_POWER_DESC 4
#define SORT_COST_DESC  5

/* Filtered/sorted view over the deckable collection entries.  s_view_indices
 * holds collection indices; s_view_count is the number of visible cards. */
static uint8_t s_view_indices[MAX_CARD_COLLECTION];
static uint8_t s_view_count;

static bool card_deckable(const CardDefinition *def)
{
    return def && def->type != CARD_TYPE_SPECIAL;
}

static CardId view_card_id(Game *g, uint8_t pos)
{
    return g->state.cards.collection.entries[s_view_indices[pos]].id;
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
        va = sort_value(card_get_def(view_card_id(g, tmp)), g->item_menu_sort);
        j = i;
        while (j > 0) {
            vb = sort_value(card_get_def(view_card_id(g, s_view_indices[j - 1])),
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
    const CardDefinition *def;

    s_view_count = 0;
    total = g->state.cards.collection.count;
    for (i = 0; i < total; i++) {
        def = card_get_def(g->state.cards.collection.entries[i].id);
        if (!card_deckable(def)) continue;
        if (filter_matches(g, g->state.cards.collection.entries[i].id))
            s_view_indices[s_view_count++] = i;
    }
    if (g->item_menu_sort != SORT_NONE && s_view_count > 1)
        sort_view(g);
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

/* Shared label tables (fixed bank, same file — safe per AGENTS.md §54.6). */
static const char *const s_card_type_names[CARD_TYPE_COUNT] = {
    "ATK", "DEF", "HEL", "STS", "UTL", "SPL"
};

/* Indexed by BattleCardType (src/battle/card.h). */
static const char *const s_bt_codes[5] = {
    "SW", "SH", "BO", "FI", "HE"
};

static const char *card_type_name(uint8_t t)
{
    return (t < CARD_TYPE_COUNT) ? s_card_type_names[t] : "SPL";
}

static const char *filter_name(uint8_t f)
{
    return (f <= CARD_TYPE_UTILITY) ? s_card_type_names[f] : "ALL";
}

static const char *sort_name(uint8_t s)
{
    switch (s) {
        case SORT_TYPE:       return "TYPE";
        case SORT_POWER:      return "PWR+";
        case SORT_COST:       return "CST+";
        case SORT_POWER_DESC: return "PWR-";
        case SORT_COST_DESC:  return "CST-";
        default:              return "OFF";
    }
}

static const char *battle_type_code(uint8_t bt)
{
    return (bt <= BATTLE_CARD_TYPE_HEAL) ? s_bt_codes[bt] : "??";
}

static void scroll_to_index(Game *g)
{
    uint8_t pos;
    if (g->item_menu_index <= FIRST_CARD) {
        g->item_menu_scroll = 0;
        return;
    }
    pos = (uint8_t)(g->item_menu_index - FIRST_CARD);
    if (pos < g->item_menu_scroll)
        g->item_menu_scroll = pos;
    else if (pos > (uint8_t)(g->item_menu_scroll + VISIBLE_CARDS - 1))
        g->item_menu_scroll = (uint8_t)(pos - (VISIBLE_CARDS - 1));
}

static void draw_card_pair(Game *g, uint8_t y, uint8_t pos)
{
    const CardDefinition *def;
    const char *bt;
    char code[4];
    CardId id;
    uint8_t in_deck;

    id = view_card_id(g, pos);
    def = card_get_def(id);
    if (!def) return;

    if ((uint8_t)(pos + FIRST_CARD) == g->item_menu_index)
        ui_draw_text_line(0, y, ">", 1);
    bt = battle_type_code(def->battle_type);
    code[0] = bt[0];
    code[1] = bt[1];
    code[2] = (char)('0' + (def->power % 10));
    code[3] = '\0';
    ui_draw_text_line(2, y, code, 3);
    /* Membership glyph is the decked-copy count (0..n), so partial stacks
     * (starter 2x SW3/SH2, herb up to 3) are visible. */
    in_deck = deck_count_in_deck(&g->state.cards.deck, id);
    code[0] = (char)('0' + (in_deck % 10));
    code[1] = '\0';
    ui_draw_text_line(19, y, code, 1);
    ui_draw_text_line(2, (uint8_t)(y + 1),
                      card_get_description(def->battle_type), 17);
}

static void draw_cards_list(Game *g)
{
    uint8_t i, y, pos;

    build_view(g);
    y = 5;
    if (g->item_menu_scroll == 0) {
        if (g->item_menu_index == ROW_TOP)
            ui_draw_text_line(0, y, ">", 1);
        ui_draw_text_line(2, y, "* FILTER/SORT *", 15);
        y = 6;
    }
    for (i = 0; i < VISIBLE_CARDS && y <= 15; i++) {
        pos = (uint8_t)(g->item_menu_scroll + i);
        if (pos >= s_view_count) break;
        draw_card_pair(g, y, pos);
        y = (uint8_t)(y + 2);
    }

    if (g->item_menu_message == MSG_DECK_FULL)
        ui_draw_text_line(0, 16, "DECK FULL", 9);
    else
        ui_draw_text_line(0, 16, "A:ADD SEL:INFO", 14);
}

static void quest_draw_status_line(Game *g, const QuestDefinition *q, uint8_t y)
{
    QuestStatus st;
    char b[6];
    uint8_t val;
    if (!q) return;
    st = quest_status(&g->state, q);

    if (st == QUEST_STATUS_NOT_STARTED) {
        ui_draw_text_line(2, y, "not started", 11);
    } else if (st == QUEST_STATUS_ACTIVE) {
        if (q->progress_variable != 0) {
            val = (uint8_t)game_variable_get(&g->state, q->progress_variable);
            ui_draw_text_line(2, y, q->progress_label ? q->progress_label : "", 8);
            b[0] = ':';
            b[1] = ' ';
            b[2] = (char)('0' + val);
            b[3] = '/';
            b[4] = (char)('0' + (uint8_t)q->progress_target);
            ui_draw_text_line(10, y, b, 5);
        } else {
            ui_draw_text_line(2, y, "active", 6);
        }
    } else {
        /* Two segments from x1 so an 8-char note still fits the 20-col row. */
        ui_draw_text_line(1, y, "complete - ", 11);
        ui_draw_text_line(12, y, q->complete_note ? q->complete_note : "", 8);
    }
}

static void draw_quest(Game *g)
{
    uint8_t i, y = 5;
    const QuestDefinition *q;
    for (i = 0; i < quest_count() && y < 15; i++) {
        q = quest_at(i);
        if (!q) break;
        if (g->item_menu_index == i) ui_draw_text_line(0, y, ">", 1);
        ui_draw_text_line(2, y, q->name, 18);
        y++;
        quest_draw_status_line(g, q, y);
        y++;
    }
}

static void draw_card_detail_page(Game *g)
{
    const CardDefinition *def;
    CardId id;
    char b[21];
    uint8_t y = 5;

    if (g->item_menu_index < FIRST_CARD ||
        (uint8_t)(g->item_menu_index - FIRST_CARD) >= s_view_count)
        return;
    id = view_card_id(g, (uint8_t)(g->item_menu_index - FIRST_CARD));
    def = card_get_def(id);
    if (!def) return;

    ui_draw_text_line(0, y, def->name, 9);
    y += 2;
    ui_draw_text_line(0, y, "TYPE", 4);
    ui_draw_text_line(6, y, card_type_name(def->type), 3);
    y += 2;
    /* Composed rows keep the fixed-bank call-site cost down. */
    b[0]='P'; b[1]='W'; b[2]='R'; b[3]=' '; b[4]=' '; b[5]=(char)('0'+(def->power%10));
    b[6]=' '; b[7]='C'; b[8]='O'; b[9]='S'; b[10]='T'; b[11]=' ';
    b[12]=(char)('0'+(def->cost%10)); b[13]='\0';
    ui_draw_text_line(0, y, b, 13);
    y += 2;
    b[0]='U'; b[1]='S'; b[2]='E'; b[3]='S'; b[4]=' ';
    b[5]=def->uses_per_battle ? (char)('0'+(def->uses_per_battle%10)) : '-';
    b[6]='/'; b[7]='B'; b[8]='T'; b[9]='L'; b[10]=' ';
    b[11]='M'; b[12]='X'; b[13]='C'; b[14]='P'; b[15]=' ';
    b[16]=def->max_copies ? (char)('0'+(def->max_copies%10)) : '-';
    b[17]='\0';
    ui_draw_text_line(0, y, b, 17);
    y += 2;
    b[0]='O'; b[1]='W'; b[2]='N'; b[3]=' '; b[4]=' ';
    b[5]=(char)('0'+(deck_collection_count(&g->state.cards, id)%10));
    b[6]=' '; b[7]='D'; b[8]='E'; b[9]='C'; b[10]='K'; b[11]=' ';
    b[12]=(char)('0'+(deck_count_in_deck(&g->state.cards.deck, id)%10));
    b[13]='\0';
    ui_draw_text_line(0, y, b, 13);
    y += 2;
    ui_draw_text_line(0, y, "PRICE", 5);
    ui_draw_num2(6, y, def->price);
}

static void draw_picker(Game *g)
{
    ui_draw_text_line(0, (uint8_t)(7 + (uint8_t)(g->item_menu_pick_row * 2)),
                      ">", 1);
    ui_draw_text_line(2, 7, "FILTER", 6);
    ui_draw_text_line(12, 7, filter_name(g->item_menu_filter), 3);
    ui_draw_text_line(2, 9, "SORT", 4);
    ui_draw_text_line(12, 9, sort_name(g->item_menu_sort), 4);
    ui_draw_text_line(0, 14, "LR CYCLE  A:OK B:NO", 19);
}

void item_screen_reset(Game *g)
{
    g->item_menu_index = ROW_TOP;
    g->item_menu_tab = TAB_CARDS;
    g->item_menu_scroll = 0;
    g->item_menu_filter = FILTER_ALL;
    g->item_menu_sort = SORT_NONE;
    g->item_menu_mode = MODE_LIST;
    g->item_menu_message = MSG_NONE;
}

static void close_menu(Game *g)
{
    item_screen_reset(g);
    screen_change(g, g->prev_screen);
}

void item_screen_update(Game *g)
{
    uint8_t total;

    /* Transient rejection messages expire on their own (~0.75s). */
    if (g->item_menu_msg_ttl > 0) {
        g->item_menu_msg_ttl--;
        if (g->item_menu_msg_ttl == 0) {
            g->item_menu_message = MSG_NONE;
            g->render_cache.valid = false;
        }
    }

    if (input_pressed(INPUT_START)) {
        if (g->item_menu_mode == MODE_LIST) {
            close_menu(g);
        } else {
            g->item_menu_mode = MODE_LIST;
            g->render_cache.valid = false;
        }
        return;
    }
    if (input_pressed(INPUT_B)) {
        switch (g->item_menu_mode) {
            case MODE_CARD_DETAIL:
            case MODE_QUEST_DETAIL:
            case MODE_PICKER:
                if (g->item_menu_mode == MODE_PICKER) {
                    g->item_menu_filter = g->item_menu_prev_filter;
                    g->item_menu_sort = g->item_menu_prev_sort;
                    build_view(g);
                }
                g->item_menu_mode = MODE_LIST;
                break;
            default:
                if (g->item_menu_tab == TAB_CARDS &&
                    g->item_menu_index > ROW_TOP) {
                    /* Two-step close: first B jumps to the top row. */
                    g->item_menu_index = ROW_TOP;
                    scroll_to_index(g);
                } else {
                    close_menu(g);
                }
                break;
        }
        g->render_cache.valid = false;
        return;
    }

    if (g->item_menu_tab == TAB_QUEST) {
        if (g->item_menu_mode == MODE_QUEST_DETAIL) return;
        if (input_pressed(INPUT_SELECT)) {
            g->item_menu_mode = MODE_QUEST_DETAIL;
            g->render_cache.valid = false;
            return;
        }
        build_view(g);
        total = quest_count();
        if (total == 0) return;
        if (input_pressed(INPUT_UP)) {
            g->item_menu_index = (g->item_menu_index > 0) ?
                (uint8_t)(g->item_menu_index - 1) : (uint8_t)(total - 1);
            g->render_cache.valid = false;
        } else if (input_pressed(INPUT_DOWN)) {
            g->item_menu_index = (g->item_menu_index < (uint8_t)(total - 1)) ?
                (uint8_t)(g->item_menu_index + 1) : 0;
            g->render_cache.valid = false;
        }
        return;
    }

    /* ── CARDS tab ─────────────────────────────────────────────── */
    switch (g->item_menu_mode) {
        case MODE_CARD_DETAIL:
            if (input_pressed(INPUT_SELECT)) {
                g->item_menu_mode = MODE_LIST;
                g->render_cache.valid = false;
            }
            return;
        case MODE_PICKER:
            if (input_pressed(INPUT_UP) || input_pressed(INPUT_DOWN)) {
                g->item_menu_pick_row =
                    (uint8_t)(g->item_menu_pick_row == 0 ? 1 : 0);
                g->render_cache.valid = false;
            } else if (input_pressed(INPUT_LEFT) || input_pressed(INPUT_RIGHT)) {
                if (g->item_menu_pick_row == 0) cycle_filter(g);
                else cycle_sort(g);
                build_view(g);
                g->render_cache.valid = false;
            } else if (input_pressed(INPUT_A)) {
                g->item_menu_mode = MODE_LIST;
                g->render_cache.valid = false;
            }
            return;
        default:
            break;
    }

    /* Tab switch (LIST mode only — submenus consume LR first). */
    if (input_pressed(INPUT_LEFT) || input_pressed(INPUT_RIGHT)) {
        g->item_menu_tab = (g->item_menu_tab == TAB_CARDS) ?
            TAB_QUEST : TAB_CARDS;
        g->item_menu_index = ROW_TOP;
        g->item_menu_scroll = 0;
        g->render_cache.valid = false;
        return;
    }

    /* One fresh view per frame for every LIST-mode path below. */
    build_view(g);

    if (input_pressed(INPUT_SELECT)) {
        if (g->item_menu_index >= FIRST_CARD &&
            (uint8_t)(g->item_menu_index - FIRST_CARD) < s_view_count) {
            g->item_menu_mode = MODE_CARD_DETAIL;
            g->render_cache.valid = false;
        }
        return;
    }
    if (input_pressed(INPUT_A)) {
        if (g->item_menu_index == ROW_TOP) {
            g->item_menu_mode = MODE_PICKER;
            g->item_menu_pick_row = 0;
            g->item_menu_prev_filter = g->item_menu_filter;
            g->item_menu_prev_sort = g->item_menu_sort;
        } else {
            CardState *cs = &g->state.cards;
            CardId id;
            if ((uint8_t)(g->item_menu_index - FIRST_CARD) >= s_view_count)
                return;
            id = view_card_id(g, (uint8_t)(g->item_menu_index - FIRST_CARD));
            /* Count-up-then-clear: A adds one copy; once the card is fully
             * decked (add rejected for per-card reasons, not a full deck),
             * A clears every decked copy of it. */
            if (deck_add_card(cs, id)) {
                telemetry_emit(EVENT_CARD_ADDED_TO_DECK, id, 0, 0, 0);
            } else if (cs->deck.count >= MAX_DECK_CARDS) {
                g->item_menu_message = MSG_DECK_FULL;
                g->item_menu_msg_ttl = 45;
            } else {
                while (deck_remove_card(cs, id))
                    telemetry_emit(EVENT_CARD_REMOVED_FROM_DECK, id, 0, 0, 0);
            }
        }
        g->render_cache.valid = false;
        return;
    }

    total = (uint8_t)(s_view_count + 1); /* + the FILTER/SORT row */
    if (total <= 1) return;

    if (input_pressed(INPUT_UP)) {
        g->item_menu_index = (g->item_menu_index > 0) ?
            (uint8_t)(g->item_menu_index - 1) : (uint8_t)(total - 1);
        scroll_to_index(g);
        g->render_cache.valid = false;
    } else if (input_pressed(INPUT_DOWN)) {
        g->item_menu_index = (g->item_menu_index < (uint8_t)(total - 1)) ?
            (uint8_t)(g->item_menu_index + 1) : ROW_TOP;
        scroll_to_index(g);
        g->render_cache.valid = false;
    }
}

void item_screen_render(Game *g)
{
    RenderCache *rc;
    if (!g) return;
    rc = &g->render_cache;
    if (!rc->valid || rc->prev_screen != SCREEN_ITEM) {
        if (g->item_menu_tab == TAB_QUEST)
            menu_draw_frame("QUESTS");
        else
            menu_draw_frame("CARDS");
        ui_draw_text_line(0, 2, "CARDS QUEST", 11);
        ui_draw_text_line((uint8_t)(g->item_menu_tab * 6), 3, "^", 1);

        if (g->item_menu_tab == TAB_QUEST) {
            if (g->item_menu_mode == MODE_QUEST_DETAIL)
                ui_draw_text_line(0, 8, "no details yet", 14);
            else
                draw_quest(g);
        } else {
            switch (g->item_menu_mode) {
                case MODE_CARD_DETAIL: draw_card_detail_page(g); break;
                case MODE_PICKER:      draw_picker(g); break;
                default:               draw_cards_list(g); break;
            }
        }

        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)SCREEN_ITEM, 0, 0, 0);
        rc->valid = true;
        rc->prev_screen = SCREEN_ITEM;
    }
}

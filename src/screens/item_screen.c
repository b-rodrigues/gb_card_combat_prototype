#include "game.h"
#include "screen.h"
#include "telemetry.h"
#include "rpg/items.h"
#include "rpg/inventory.h"
#include "rpg/currency.h"
#include "rpg/progression.h"
#include "menu.h"
#include "game_ids.h"
#include "quest.h"
#include "ui.h"

#define MENU_TAB_ITEM   0
#define MENU_TAB_EQUIP  1
#define MENU_TAB_QUEST  2
#define MENU_TAB_STATUS 3

static bool menu_item_visible(uint8_t tab, const ItemDefinition *def)
{
    if (!def) return false;
    return (tab == MENU_TAB_ITEM) ? (def->kind == ITEM_KIND_CONSUMABLE) : (def->kind == ITEM_KIND_WEAPON);
}

static uint8_t get_inv_slot(const InventoryState *inv, uint8_t tab, uint8_t sel_idx, uint8_t *out_total)
{
    uint8_t i, n = 0, found = 0xFF;
    for (i = 0; i < inv->count && i < MAX_INVENTORY_ITEMS; i++) {
        const ItemDefinition *def = item_get_def(inv->entries[i].item_id);
        if (menu_item_visible(tab, def)) {
            if (n == sel_idx) found = i;
            n++;
        }
    }
    if (out_total) *out_total = n;
    return found;
}

static void menu_draw_tabs(Game *g)
{
    ui_draw_text_line(0, 2, "ITEM EQUIPQUESTSTAT ", 20);
    ui_draw_text_line((uint8_t)(g->item_menu_tab * 5), 3, "^", 1);
    ui_draw_hline(4, '-');
}

static void menu_draw_status(Game *g, char *buf)
{
    const CharacterState *hero = &g->state.party.members[0];
    ProgressionState *ps = progression_get(&g->state, PROG_TYPE_HERO, 1);
    const ItemDefinition *wd;

    ui_draw_text_line(0, 5, "HERO", 4);
    ui_draw_text_line(0, 6, "HP:   /   ", 10);
    ui_draw_num2(4, 6, hero->hp);
    ui_draw_num2(7, 6, hero->max_hp);

    ui_draw_text_line(0, 7, "GOLD:", 5);
    ui_format_int(currency_get(&g->state, CURRENCY_ID_GOLD), buf);
    ui_draw_text_line(6, 7, buf, 10);

    ui_draw_text_line(0, 9, "LEVEL:", 6);
    ui_draw_num2(6, 9, (uint8_t)(ps ? ps->level : 1));

    ui_draw_text_line(0, 10, "PROGRESS:", 9);
    ui_format_int((int16_t)(ps ? ps->progress : 0), buf);
    ui_draw_text_line(10, 10, buf, 6);

    ui_draw_text_line(0, 11, "WEAPON:", 7);
    wd = (g->state.equipment.weapon != ITEM_NONE) ? item_get_def(g->state.equipment.weapon) : NULL;
    ui_draw_text_line(8, 11, wd ? wd->name : "NONE", 8);
}

static void quest_draw_status(Game *g, const QuestDefinition *q, uint8_t y)
{
    QuestStatus st;
    if (!q) return;
    st = quest_status(&g->state, q);

    if (st == QUEST_STATUS_NOT_STARTED) {
        ui_draw_text_line(0, y, "not started", 11);
    } else if (st == QUEST_STATUS_ACTIVE) {
        if (q->progress_variable != 0) {
            char b[6];
            uint8_t val = (uint8_t)game_variable_get(&g->state, q->progress_variable);
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

static void menu_draw_quest(Game *g, char *buf)
{
    uint8_t i, y = 7;
    const QuestDefinition *q;
    (void)buf;

    for (i = 0; i < quest_count() && y < 15; i++) {
        q = quest_at(i);
        if (!q) break;
        ui_draw_text_line(0, y++, q->name, 20);
        quest_draw_status(g, q, y++);
    }
}

static const char *tab_title(uint8_t tab)
{
    if (tab == MENU_TAB_EQUIP) return "EQUIP";
    if (tab == MENU_TAB_QUEST) return "QUESTS";
    if (tab == MENU_TAB_STATUS) return "STATUS";
    return "ITEMS";
}

static void menu_draw(Game *g)
{
    const InventoryState *inv = &g->state.inventory;
    char buf[7];
    uint8_t i, y, vis_count = 0, scroll = 0;

    menu_draw_frame(tab_title(g->item_menu_tab));
    menu_draw_tabs(g);

    if (g->item_menu_tab >= MENU_TAB_QUEST) {
        if (g->item_menu_tab == MENU_TAB_STATUS) {
            menu_draw_status(g, buf);
        } else {
            menu_draw_quest(g, buf);
        }
        ui_draw_text_line(0, 16, "[B] CLOSE", 10);
        return;
    }

    if (g->item_menu_index >= 8) {
        scroll = (uint8_t)(g->item_menu_index - 7);
    }

    for (i = 0; i < inv->count && i < MAX_INVENTORY_ITEMS; i++) {
        const ItemDefinition *def = item_get_def(inv->entries[i].item_id);
        if (menu_item_visible(g->item_menu_tab, def)) {
            if (vis_count >= scroll && (uint8_t)(vis_count - scroll) < 8) {
                y = (uint8_t)(5 + (vis_count - scroll));
                ui_draw_text_line(0, y, (g->item_menu_index == vis_count) ? ">" : " ", 1);
                ui_draw_text_line(1, y, def->name, 8);
                if (def->kind == ITEM_KIND_CONSUMABLE) {
                    ui_draw_text_line(10, y, "x", 1);
                    ui_draw_num2(11, y, inv->entries[i].quantity);
                } else if (g->state.equipment.weapon == inv->entries[i].item_id) {
                    ui_draw_text_line(10, y, "EQUIPPED", 8);
                } else {
                    ui_draw_text_line(10, y, "EQUIP", 5);
                }
            }
            vis_count++;
        }
    }
    if (vis_count == 0) {
        ui_draw_text_line(0, 5, "(no items)", 10);
    }
    ui_draw_text_line(0, 16, (g->item_menu_tab == MENU_TAB_ITEM) ? "[A] USE  " : "[A] EQUIP ", 10);
    ui_draw_text_line(10, 16, "[B] CLOSE", 10);
}

void item_screen_update(Game *g)
{
    uint8_t vis_count, ei;
    if (input_pressed(INPUT_SELECT) || input_pressed(INPUT_RIGHT)) {
        g->item_menu_tab = (uint8_t)((g->item_menu_tab + 1) & 3);
        g->item_menu_index = 0;
        g->render_cache.valid = false;
        return;
    }
    if (input_pressed(INPUT_LEFT)) {
        g->item_menu_tab = (uint8_t)((g->item_menu_tab + 3) & 3);
        g->item_menu_index = 0;
        g->render_cache.valid = false;
        return;
    }

    if (input_pressed(INPUT_B)) {
        g->item_menu_index = 0;
        g->item_menu_tab = MENU_TAB_ITEM;
        screen_change(g, g->prev_screen);
        return;
    }

    if (g->item_menu_tab >= MENU_TAB_QUEST) return;

    ei = get_inv_slot(&g->state.inventory, g->item_menu_tab, g->item_menu_index, &vis_count);
    if (vis_count > 0) {
        if (input_pressed(INPUT_UP) && g->item_menu_index > 0) {
            g->item_menu_index--;
            g->render_cache.valid = false;
        }
        if (input_pressed(INPUT_DOWN) && (uint8_t)(g->item_menu_index + 1) < vis_count) {
            g->item_menu_index++;
            g->render_cache.valid = false;
        }
        if (input_pressed(INPUT_A) && ei != 0xFF) {
            ItemId id = g->state.inventory.entries[ei].item_id;
            if (g->item_menu_tab == MENU_TAB_ITEM) {
                if (item_use(&g->state, id, CHARACTER_HERO)) {
                    if (g->prev_screen == SCREEN_BATTLE) {
                        g->battle.player.hp = g->state.party.members[0].hp;
                        g->battle.turn = BATTLE_TURN_ENEMY_DELAY;
                        g->battle.delay_timer = 20;
                    }
                }
            } else {
                item_equip(&g->state, id);
            }
            get_inv_slot(&g->state.inventory, g->item_menu_tab, 0, &vis_count);
            if (g->item_menu_index >= vis_count) {
                g->item_menu_index = (uint8_t)(vis_count > 0 ? vis_count - 1 : 0);
            }
            g->render_cache.valid = false;
        }
    }
}

void item_screen_render(Game *g)
{
    RenderCache *rc;
    if (!g) return;
    rc = &g->render_cache;

    if (!rc->valid || rc->prev_screen != SCREEN_ITEM) {
        menu_draw(g);
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)SCREEN_ITEM, 0, 0, 0);
        rc->valid = true;
        rc->prev_screen = SCREEN_ITEM;
    }
}

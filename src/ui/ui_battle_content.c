#pragma bank 2
#pragma disable_warning 110

#include "ui.h"
#include "battle.h"
#include "card.h"
#include "banked.h"
#include <gb/gb.h>

/* ── Self-contained VRAM helpers (duplicated from ui.c so the banked module
 * never calls fixed-bank functions — see banked.h ABI constraint). ─────── */

extern uint8_t ui_font_tile_base;

static void battle_vram_sync_write(volatile uint8_t *dst, uint8_t tile)
{
    if (LCDC_REG & 0x80) {
        while (!(STAT_REG & 0x02));
        while (STAT_REG & 0x02);
    }
    *dst = tile;
}

static void battle_put_char(uint8_t x, uint8_t y, char ch)
{
    if (y < 18 && x < 20) {
        if (g_ui_screen_buf[y][x] != ch) {
            uint8_t tile = (uint8_t)(ui_font_tile_base + (uint8_t)(ch - ' '));
            volatile uint8_t *dst = (volatile uint8_t *)(0x9800 + ((uint16_t)y << 5) + x);
            VBK_REG = 0;
            battle_vram_sync_write(dst, tile);
            g_ui_screen_buf[y][x] = ch;
        }
    }
}

static void battle_draw_text_line(uint8_t x, uint8_t y, const char *text,
                                  uint8_t max_chars)
{
    uint8_t i;
    char ch;
    volatile uint8_t *dst;
    char *buf;
    uint8_t ended;

    if (y >= 18 || x >= 20) return;
    if ((uint8_t)(x + max_chars) > 20) max_chars = (uint8_t)(20 - x);

    VBK_REG = 0;
    ended = (text == NULL);
    dst = (volatile uint8_t *)(0x9800 + ((uint16_t)y << 5) + x);
    buf = &g_ui_screen_buf[y][x];

    for (i = 0; i < max_chars; i++) {
        if (!ended) {
            ch = text[i];
            if (ch == '\0') {
                ended = 1;
                ch = ' ';
            }
        } else {
            ch = ' ';
        }
        if (*buf != ch) {
            uint8_t tile = (uint8_t)(ui_font_tile_base + (uint8_t)(ch - ' '));
            battle_vram_sync_write(dst, tile);
            *buf = ch;
        }
        dst++;
        buf++;
    }
}

static void battle_draw_num2(uint8_t x, uint8_t y, uint8_t val)
{
    uint8_t d = 0;
    if (val > 99) val = 99;
    while (val >= 10) { val -= 10; d++; }
    battle_put_char(x, y, d ? (char)('0' + d) : ' ');
    battle_put_char((uint8_t)(x + 1), y, (char)('0' + val));
}

/* ── Card helpers (inlined from card.c — banked code cannot call fixed). ── */

static const char *battle_card_type_code(uint8_t type)
{
    static const char codes[] = "SW\0SH\0BO\0FI\0HE\0??";
    return (type < 5) ? (codes + (type * 3)) : (codes + 15);
}

static const char *battle_card_get_description(uint8_t type)
{
    switch (type) {
        case BATTLE_CARD_TYPE_SWORD:  return "Sword: physical";
        case BATTLE_CARD_TYPE_SHIELD: return "Shield: block dmg";
        case BATTLE_CARD_TYPE_BOW:    return "Bow: ranged dmg";
        case BATTLE_CARD_TYPE_FIRE:   return "Fire: magic dmg";
        case BATTLE_CARD_TYPE_HEAL:   return "Heal: restore HP";
        default: return "";
    }
}

/* ── Battle rendering helpers ─────────────────────────────────────────── */

static void battle_draw_card_at(uint8_t x, uint8_t y, Card card)
{
    const char *code = battle_card_type_code(card.type);
    battle_put_char(x, y, code[0]);
    battle_put_char((uint8_t)(x + 1), y, code[1]);
    battle_put_char((uint8_t)(x + 2), y, (char)('0' + card.value));
}

static void battle_draw_enemy_columns(const Battle *battle)
{
    uint8_t k, x = 0;
    const Combatant *e;
    bool blink_name = (battle->phase == BATTLE_PHASE_PLAYER_DEFEND) &&
                      (((battle->timer_ticks >> 4) & 1) == 0);

    for (k = 0; k < MAX_BATTLE_ENEMIES; k++, x = (uint8_t)(x + 7)) {
        if (k < battle->enemy_count && battle->enemies[k].hp != 0) {
            e = &battle->enemies[k];
            if (blink_name && k == battle->attacking_enemy_idx) {
                battle_draw_text_line(x, 2, NULL, 6);
            } else {
                battle_draw_text_line(x, 2, e->name[0] ? e->name : "ENEMY", 6);
            }
            battle_draw_num2(x, 3, e->hp);
            battle_put_char((uint8_t)(x + 2), 3, '/');
            battle_draw_num2((uint8_t)(x + 3), 3, e->max_hp);
            battle_put_char((uint8_t)(x + 5), 3, ' ');
            if (k == battle->target_idx &&
                (battle->phase == BATTLE_PHASE_PLAYER_SELECT || battle->phase == BATTLE_PHASE_PLAYER_DEFEND)) {
                battle_draw_text_line(x, 4, "  ^   ", 6);
                continue;
            }
        } else {
            battle_draw_text_line(x, 2, NULL, 6);
            battle_draw_text_line(x, 3, NULL, 6);
        }
        battle_draw_text_line(x, 4, NULL, 6);
    }
}

static void battle_draw_hero_row(const Battle *battle)
{
    battle_draw_text_line(0, 6, "HERO        HP:", 15);
    battle_draw_num2(15, 6, battle->player.hp);
    battle_put_char(17, 6, '/');
    battle_draw_num2(18, 6, battle->player.max_hp);
}

static const char *battle_combo_hand_name(const Battle *b)
{
    uint8_t i, j, prev, t0, v, straight, same, n;

    n = b->combo_count;
    if (n < 2 || n > 5) return "";
    prev = b->hand[b->selected_indices[0]].value;
    t0 = b->hand[b->selected_indices[0]].type;
    straight = same = 1;
    for (i = 1; i < n; i++) {
        v = b->hand[b->selected_indices[i]].value;
        if (v != (uint8_t)(prev + 1)) straight = 0;
        if (b->hand[b->selected_indices[i]].type != t0) same = 0;
        prev = v;
    }
    if (straight) return "STRAIGHT";
    if (same) return "FLUSH";
    for (i = 0; i < n; i++) {
        for (j = (uint8_t)(i + 1); j < n; j++) {
            if (b->hand[b->selected_indices[i]].value == b->hand[b->selected_indices[j]].value) return "PAIR";
        }
    }
    return "";
}

static void battle_draw_battle_combo(const Battle *battle)
{
    const char *name = battle_combo_hand_name(battle);

    battle_draw_text_line(0, 13, "COMBO:", 6);
    if (name[0] != '\0') {
        battle_draw_text_line(7, 13, " ", 1);
        battle_draw_text_line(8, 13, name, 12);
    } else {
        battle_draw_text_line(7, 13, NULL, 13);
    }
}

static void battle_draw_battle_hand(const Battle *battle)
{
    uint8_t i;
    for (i = 0; i < BATTLE_HAND_SIZE; i++) {
        uint8_t col = (uint8_t)(i << 2);
        uint8_t k;
        char marker = ' ';
        for (k = 0; k < battle->combo_count; k++) {
            if (battle->selected_indices[k] == i) {
                marker = (char)('0' + (k + 1));
                break;
            }
        }
        battle_draw_card_at(col, 14, battle->hand[i]);
        if (i == battle->cursor_pos) {
            battle_put_char((uint8_t)(col + 1), 15, '^');
        } else {
            battle_put_char((uint8_t)(col + 1), 15, marker);
        }
    }
}

static void battle_draw_banner_line(uint8_t y, const char *text, uint8_t width)
{
    uint8_t len = 0, x;
    if (text) {
        while (len < width && text[len] != '\0') len++;
    }
    x = (width - len) / 2;
    battle_draw_text_line(0, y, NULL, width);
    battle_draw_text_line(x, y, text, (uint8_t)(width - x));
}

/* ── Banked entry point ────────────────────────────────────────────────── */

void ui_update_battle_banked(void)
{
    const Battle *battle = (const Battle *)g_bk_ptr_a;
    uint8_t d;
    const char *turn_banner = "";
    const char *desc_msg = "";

    if (!battle) return;
    d = battle->dirty ? battle->dirty : BATTLE_DIRTY_ALL;

    if (d & (BATTLE_DIRTY_BANNER | BATTLE_DIRTY_DESC)) {
        if (battle->result == BATTLE_RESULT_VICTORY) {
            turn_banner = "VICTORY!";
        } else if (battle->result == BATTLE_RESULT_DEFEAT) {
            turn_banner = "DEFEATED!";
        } else if (battle->result == BATTLE_RESULT_FLED) {
            turn_banner = "FLED!";
        } else {
            if (battle->phase == BATTLE_PHASE_PLAYER_SELECT) {
                turn_banner = "PLAYER TURN";
                desc_msg = battle_card_get_description(battle->hand[battle->cursor_pos].type);
            } else if (battle->phase == BATTLE_PHASE_PLAYER_ANIM) {
                turn_banner = battle->last_combo.is_straight ? "STRAIGHT COMBO!" : "PLAYER ATTACK!";
            } else if (battle->phase == BATTLE_PHASE_ENEMY_TELEGRAPH) {
                turn_banner = "ENEMY ATTACK!";
            } else if (battle->phase == BATTLE_PHASE_PLAYER_DEFEND) {
                turn_banner = "DEFENSE TURN";
                desc_msg = battle_card_get_description(battle->hand[battle->cursor_pos].type);
            } else if (battle->phase == BATTLE_PHASE_DEFENSE_RESOLVE) {
                turn_banner = "BLOCKED ATTACK!";
            }
        }
    }

    if (d & BATTLE_DIRTY_BANNER) battle_draw_banner_line(0, turn_banner, 20);
    if (d & (BATTLE_DIRTY_ENEMIES | BATTLE_DIRTY_BLINK)) battle_draw_enemy_columns(battle);
    if (d & BATTLE_DIRTY_HERO) battle_draw_hero_row(battle);
    if (d & BATTLE_DIRTY_COMBO) battle_draw_battle_combo(battle);
    if (d & BATTLE_DIRTY_HAND) battle_draw_battle_hand(battle);
    if (d & BATTLE_DIRTY_DESC) battle_draw_text_line(0, 16, desc_msg, 20);
}

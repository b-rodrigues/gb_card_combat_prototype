#pragma bank 3
#pragma disable_warning 110

#include "ui.h"
#include "battle.h"
#include "card.h"
#include "banked.h"
#include "rpg/status.h"
#include <gb/gb.h>

/* ── Self-contained VRAM helpers (duplicated from ui.c so the banked module
 * never calls fixed-bank functions — see banked.h ABI constraint). ─────── */

extern uint8_t ui_font_tile_base;

/* Loot reveal (docs/loot.md §34.5): the dropped card's synthesized
 * definition sits in the shared WRAM scratch (rpg/cards.c) -- the
 * fixed side re-populates it via card_get_def() when the VICTORY
 * screen appears; readable here from any bank. */
extern CardDefinition g_card_scratch;

/* VRAM is accessible in PPU Modes 0-2 and writes are IGNORED in Mode 3
 * (Pan Docs, Accessing_VRAM_and_OAM).  Wait while Mode is 2 or 3, so the
 * store lands in Mode 0 (HBlank) or 1 (VBlank); a store issued at the very
 * end of Mode 0 rolls into Mode 2, which is still accessible, so the store
 * cannot be eclipsed by the PPU.  The previous double-wait (wait UNTIL
 * Mode 2 starts, then UNTIL it ends) placed every store at the START of
 * Mode 3 on LCD-on frames -- writes silently dropped while
 * g_ui_screen_buf was still updated, and the put_char skip-guard then
 * never re-wrote the cell (battle hand cards stuck at "SW0" on real
 * boot; the SameBoy harness runs with the LCD off/vsync skipped and could
 * not see it).  di/ei keeps the 256 Hz timer ISR (AGENTS.md 35) from
 * eclipsing the wait->store window (the Pan Docs interrupt caveat); IE is
 * clear under the harness, so ei() is a no-op there.  Unconditionally
 * re-enabling IME via ei is safe because interrupts are always active during
 * normal play and stubbed under the harness. */
static void battle_vram_sync_write(volatile uint8_t *dst, uint8_t tile)
{
    if (LCDC_REG & 0x80) {
        __asm
            di
        __endasm;
        while (STAT_REG & 0x02);
        *dst = tile;
        __asm
            ei
        __endasm;
    } else {
        *dst = tile;
    }
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

/* Effect color for a battle card (mirrors ui_color_class; banked code must
 * not call the fixed-bank helper).  status_id = on-hit rider element,
 * is_heal = ring/heal role. */
static uint8_t battle_color_class(uint8_t status_id, uint8_t is_heal)
{
    if (is_heal) return UI_COLOR_HEAL;
    if (status_id == STATUS_BURN) return UI_COLOR_FIRE;
    if (status_id == STATUS_FREEZE) return UI_COLOR_ICE;
    if (status_id == STATUS_POISON) return UI_COLOR_POISON;
    return UI_COLOR_NONE;
}

/* CGB per-tile palette span (self-contained mirror of ui_color_span, gated
 * on g_is_cgb; keep in sync with ui_color_span_banked in ui_color_banked.c).
 * Banked body, so it cannot call fixed-bank helpers. */
static void battle_color_span(uint8_t x, uint8_t y, uint8_t len, uint8_t palette)
{
    uint8_t i;
    volatile uint8_t *dst;

    if (!g_is_cgb) return;
    if (y >= 18 || x >= 32) return;
    if ((uint8_t)(x + len) > 32) len = (uint8_t)(32 - x);

    dst = (volatile uint8_t *)(0x9800 + ((uint16_t)y << 5) + x);
    VBK_REG = 1;
    for (i = 0; i < len; i++) {
        battle_vram_sync_write(&dst[i], (uint8_t)(palette & 0x07));
    }
    VBK_REG = 0;
}

/* Name color for a combatant by its active statuses (status effects):
 * priority FREEZE (blue) > BURN (red) > POISON (purple), matching the
 * battle-card element colors.  Reads s_battle_status (WRAM) directly --
 * banked code may not call the fixed-bank status_slots() helper. */
static uint8_t battle_status_color(const StatusSlots *slots)
{
    uint8_t i;
    if (slots == (const StatusSlots *)0) return UI_COLOR_NONE;
    for (i = 0; i < slots->count; i++) {
        if (slots->slot[i].id == STATUS_FREEZE) return UI_COLOR_ICE;
    }
    for (i = 0; i < slots->count; i++) {
        if (slots->slot[i].id == STATUS_BURN) return UI_COLOR_FIRE;
    }
    for (i = 0; i < slots->count; i++) {
        if (slots->slot[i].id == STATUS_POISON) return UI_COLOR_POISON;
    }
    return UI_COLOR_NONE;
}

/* ── Card helpers (inlined from card.c — banked code cannot call fixed). ── */

static const char *battle_card_type_code(uint8_t type)
{
    static const char codes[] = "SW\0SH\0BO\0HE\0DA\0??";
    return (type < 5) ? (codes + (type * 3)) : (codes + 15);
}

static const char *battle_card_get_description(uint8_t type)
{
    switch (type) {
        case BATTLE_CARD_TYPE_SWORD:  return "Sword: physical";
        case BATTLE_CARD_TYPE_SHIELD: return "Shield: block dmg";
        case BATTLE_CARD_TYPE_BOW:    return "Bow: ranged dmg";
        case BATTLE_CARD_TYPE_HEAL:   return "Heal: restore HP";
        case BATTLE_CARD_TYPE_DAGGER: return "Dagger: poison";
        default: return "";
    }
}

/* ── Battle rendering helpers ─────────────────────────────────────────── */

static void battle_draw_card_at(uint8_t x, uint8_t y, uint8_t type, uint8_t value)
{
    const char *code;
    if (type == BATTLE_CARD_TYPE_EMPTY) {
        battle_draw_text_line(x, y, NULL, 3);
        return;
    }
    code = battle_card_type_code(type);
    battle_put_char(x, y, code[0]);
    battle_put_char((uint8_t)(x + 1), y, code[1]);
    battle_put_char((uint8_t)(x + 2), y, (char)('0' + value));
}

static void battle_draw_enemy_columns(const volatile Battle *battle)
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
                battle_color_span(x, 2, 6, UI_COLOR_NONE);
            } else {
                battle_draw_text_line(x, 2, e->name[0] ? e->name : "ENEMY", 6);
                battle_color_span(x, 2, 6,
                                  battle_status_color(&s_battle_status[k + 1]));
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

static void battle_draw_hero_row(const volatile Battle *battle)
{
    battle_draw_text_line(0, 6, "HERO        HP:", 15);
    battle_color_span(0, 6, 4, battle_status_color(&s_battle_status[0]));
    battle_draw_num2(15, 6, battle->player.hp);
    battle_put_char(17, 6, '/');
    battle_draw_num2(18, 6, battle->player.max_hp);
}

/* Draw-pile counter under the hero HP: shows how many cards are still in
 * the deck, so the player can see a reshuffle coming.  Same formula as the
 * harness's battle_draw_remaining semantic.  The pile only changes at deal,
 * turn-start draws and reshuffles -- all sites set BATTLE_DIRTY_ALL, so
 * firing on BATTLE_DIRTY_HERO can never leave this line stale. */
static void battle_draw_deck_line(const volatile Battle *battle)
{
    battle_draw_text_line(13, 7, "DECK:", 5);
    battle_draw_num2(18, 7,
                     (uint8_t)(battle->deck.count - battle->deck.draw_idx));
}

/* Tier display names (docs/combo-system.md hand table).  Rows live in
 * bank-local literals; the classifier itself is the single source of
 * truth in combo_content.c -- this only renders ComboResult.tier. */
static const char *battle_combo_tier_name(uint8_t tier)
{
    switch (tier) {
        case HAND_PAIR:           return "PAIR";
        case HAND_TWO_PAIR:       return "TWO PAIR";
        case HAND_THREE_KIND:     return "THREE KIND";
        case HAND_STRAIGHT:       return "STRAIGHT";
        case HAND_FLUSH:          return "FLUSH";
        case HAND_FULL_HOUSE:     return "FULL HOUSE";
        case HAND_FOUR_KIND:      return "FOUR KIND";
        case HAND_STRAIGHT_FLUSH: return "STR FLUSH";
        case HAND_FIVE_KIND:      return "FIVE KIND";
        default:                  return "";
    }
}

/* Live COMBO-row classification buffers: file-static so the HUD never
 * grows stack under the harness (SP=0xFFFE, AGENTS.md 52.14). */
static uint8_t s_pv_vals[5];
static uint8_t s_pv_types[5];
/* Selection-marker scratch: file-static to survive timer ISR re-entrancy
 * on real hardware (the ISR runs on the main stack). */
static char s_sel_marker;

static const char *battle_combo_pending_name(const volatile Battle *battle)
{
    uint8_t i, w = 0;
    uint8_t defend = (battle->phase == BATTLE_PHASE_PLAYER_DEFEND);
    uint8_t ring_pos = 0xFF;
    uint8_t saved, tier, best, v;

    if (battle->combo_count == 0) return "";
    for (i = 0; i < battle->combo_count; i++) {
        const Card *c = &battle->hand[battle->selected_indices[i]];
        /* Defend preview mirrors combo_resolve_banked: SHIELD cards AND
         * rings enter the hand; everything else is ignored. */
        if (defend && c->type != BATTLE_CARD_TYPE_SHIELD && !c->ring) continue;
        s_pv_vals[w] = c->value;
        s_pv_types[w] = c->type;
        if (c->ring && ring_pos == 0xFF) ring_pos = w;
        w++;
    }
    if (ring_pos != 0xFF) {
        /* Ring JOKER (docs/loot.md §34.3): the ring substitutes freely,
         * so preview the BEST tier over values 1..10 -- the full legal
         * value range, which covers the ring's raw value, so no separate
         * baseline classify is needed.  Gameplay allows at most one ring
         * per selection (battle_card_select), so tracking the first ring
         * is exact. */
        best = HAND_NONE;
        for (v = 1; v <= 10; v++) {
            saved = s_pv_vals[ring_pos];
            s_pv_vals[ring_pos] = v;
            tier = combo_classify(s_pv_vals, s_pv_types, w);
            if (tier > best) best = tier;
            s_pv_vals[ring_pos] = saved;
        }
        tier = best;
    } else {
        tier = combo_classify(s_pv_vals, s_pv_types, w);
    }
    return battle_combo_tier_name(tier);
}

static void battle_draw_battle_combo(const volatile Battle *battle)
{
    /* Real-time preview: the row tracks the PENDING selection as cards
     * are added/removed and blanks once the hand resolves -- executed
     * results announce via the banner instead of sticking around. */
    const char *name = battle_combo_pending_name(battle);

    battle_draw_text_line(0, 13, "COMBO:", 6);
    if (name[0] != '\0') {
        battle_draw_text_line(7, 13, " ", 1);
        battle_draw_text_line(8, 13, name, 12);
    } else {
        battle_draw_text_line(7, 13, NULL, 13);
    }
}

static void battle_draw_battle_hand(const volatile Battle *battle)
{
    uint8_t i, k;
    uint8_t col, ctype, cvalue, cstat, cring, ceffect, ccolor;
    /* Snapshot ALL volatile struct fields into locals before the loops.
     * SDCC 4.4.1 caches &struct.field in stack slots (§52.19); the
     * battle_draw_card_at call + timer ISR (di/wait/ei on real hardware)
     * can clobber those cached slots.  Reading into locals first keeps
     * every subsequent use out of those slots.  selected_indices is copied
     * into a small local buffer in a single loop (no nested ternary
     * cascade, which SDCC emitted as per-slot branches) so combo_count and
     * the hand are only read once. */
    uint8_t cc   = battle->combo_count;
    uint8_t cur  = battle->cursor_pos;
    uint8_t sel[BATTLE_HAND_SIZE];

    for (k = 0; k < BATTLE_HAND_SIZE; k++) {
        sel[k] = (k < cc) ? battle->selected_indices[k] : 0xFF;
    }

    for (i = 0; i < BATTLE_HAND_SIZE; i++) {
        col = (uint8_t)(i << 2);
        ctype   = battle->hand[i].type;
        cvalue  = battle->hand[i].value;
        cstat   = battle->hand[i].status_id;
        cring   = battle->hand[i].ring;
        ceffect = battle->hand[i].effect;

        s_sel_marker = ' ';
        for (k = 0; k < cc; k++) {
            if (sel[k] == i) {
                s_sel_marker = (char)('1' + k);
                break;
            }
        }
        battle_draw_card_at(col, 14, ctype, cvalue);
        /* Color the code + its selection digit by the card's effect.
         * Poison grey-out (status.h): greyed player cards render dim. */
        ccolor = battle_color_class(cstat,
                                    (cring != 0) ||
                                    (ctype == BATTLE_CARD_TYPE_HEAL) ||
                                    (ceffect == CARD_EFFECT_HEAL_HP));
        if ((s_grey_mask[0] & (uint8_t)(1u << i)) != 0) {
            ccolor = UI_COLOR_DIM;
        }
        battle_color_span(col, 14, 3, ccolor);
        if (i == cur) {
            battle_put_char((uint8_t)(col + 1), 15, '^');
            battle_color_span((uint8_t)(col + 1), 15, 1, 0);
        } else {
            battle_put_char((uint8_t)(col + 1), 15, s_sel_marker);
            battle_color_span((uint8_t)(col + 1), 15, 1,
                              (s_sel_marker != ' ') ? ccolor : 0);
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
    const volatile Battle *battle = (const Battle *)g_bk_ptr_a;
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
                turn_banner =
                    (battle->last_combo.tier == HAND_STRAIGHT ||
                     battle->last_combo.tier == HAND_STRAIGHT_FLUSH)
                        ? "STRAIGHT COMBO!" : "PLAYER ATTACK!";
            } else if (battle->phase == BATTLE_PHASE_ENEMY_TELEGRAPH) {
                turn_banner = "ENEMY ATTACK!";
            } else if (battle->phase == BATTLE_PHASE_PLAYER_DEFEND) {
                turn_banner = "DEFENSE TURN";
                desc_msg = battle_card_get_description(battle->hand[battle->cursor_pos].type);
            } else if (battle->phase == BATTLE_PHASE_DEFENSE_RESOLVE) {
                turn_banner = "BLOCKED ATTACK!";
            } else if (battle->phase == BATTLE_PHASE_SHUFFLE) {
                turn_banner = "RESHUFFLE!";
            }
        }
    }

    if (d & BATTLE_DIRTY_BANNER) battle_draw_banner_line(0, turn_banner, 20);
    if (d & (BATTLE_DIRTY_ENEMIES | BATTLE_DIRTY_BLINK)) battle_draw_enemy_columns(battle);
    if (d & BATTLE_DIRTY_HERO) {
        battle_draw_hero_row(battle);
        battle_draw_deck_line(battle);
    }
    if (d & BATTLE_DIRTY_COMBO) battle_draw_battle_combo(battle);
    if (d & BATTLE_DIRTY_HAND) battle_draw_battle_hand(battle);
    if (d & BATTLE_DIRTY_DESC) battle_draw_text_line(0, 16, desc_msg, 20);
    if (d & BATTLE_DIRTY_MSG) {
        if (battle->msg_id == 4) {
            /* Loot reveal (docs/loot.md §34.5): a two-line centered
             * "YOU FOUND:" block over the otherwise-blank rows 11-12,
             * with the card name colored by its effect. */
            uint8_t len = 0, x;
            uint8_t ncolor;
            battle_draw_banner_line(11, "YOU FOUND:", 20);
            battle_draw_banner_line(12, g_card_scratch.name, 20);
            while (len < 20 && g_card_scratch.name[len]) len++;
            x = (uint8_t)((20 - len) / 2);
            ncolor = battle_color_class(
                g_card_scratch.status_id,
                (g_card_scratch.battle_type == BATTLE_CARD_TYPE_HEAL) ||
                (g_card_scratch.effect == CARD_EFFECT_HEAL_HP));
            battle_color_span(x, 12, len, ncolor);
        } else {
            battle_draw_text_line(0, 12,
                (battle->msg_id == 1) ? "NO ENERGY!" :
                (battle->msg_id == 2) ? "OUT OF USES!" :
                (battle->msg_id == 3) ? "ONE RING!" : NULL, 12);
        }
    }
}

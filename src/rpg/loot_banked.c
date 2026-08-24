#pragma bank 2

#include "rpg/loot.h"
#include "banked.h"
#include "battle/card.h"
#include "rpg/cards.h"

/* ── Loot identity tables + banked bodies (docs/loot.md §34) ────────
 * Runs from ROM bank 2 via the trampoline.  Entry point:
 *   loot_synth_banked       id(byte_a) -> CardDefinition in g_card_scratch
 * Consumes/writes only WRAM and its own bank-local tables.
 * Extensible: append rows to grow materials/effects/weapons. */

#define STATUS_NONE    0
#define STATUS_POISON  1

static const struct {
    uint8_t power_bonus;   /* stacked onto weapon base power */
    char code[4];          /* abbreviated name prefix (§34.1) */
} s_materials[LOOT_NMATERIALS] = {
    { 0, "WD"  }, { 1, "BRN" }, { 2, "IRN" }, { 3, "MYT" }
};

/* Effect rows: status rider applied by battle on-hit.  fire/ice are
 * reserved-inert (STATUS_NONE) until their statuses exist -- they roll
 * like plain but keep their namespace and display name. */
static const struct {
    uint8_t status_id;
    uint8_t chance;        /* on-hit percent */
} s_effects[LOOT_NEFFECTS] = {
    { STATUS_NONE,   0 },       /* plain   */
    { STATUS_POISON, 50 },      /* poison  */
    { STATUS_NONE,   0 },       /* fire reserved */
    { STATUS_NONE,   0 }        /* ice reserved */
};

static const struct {
    uint8_t base_power;
    uint8_t cost;
    char symbol[3];        /* abbreviated name suffix (§34.1) */
} s_weapons[LOOT_NWEAPONS] = {
    { 3, 1, "SW" },   /* sword */
    { 2, 1, "SH" },   /* shield */
    { 2, 1, "BO" },   /* bow */
    { 1, 1, "DA" },   /* dagger */
    { 2, 1, "HE" },   /* ring (heal amount = power) */
    { 0, 0, "??" },   /* reserved */
    { 0, 0, "??" },   /* reserved */
    { 0, 0, "??" }    /* reserved */
};

/* Build the abbreviated name "[material] [effect ]weapon" into the
 * scratch def (§34.1): e.g. "WD SW", "MYT PSN DA".  Max 10 chars +
 * NUL -- fits CardDefinition.name[12]. */
static void synth_name(uint8_t material, uint8_t effect, uint8_t weapon)
{
    char *n = g_card_scratch.name;
    const char *m = s_materials[material].code;
    const char *w = s_weapons[weapon].symbol;

    while (*m) *n++ = *m++;
    *n++ = ' ';
    if (effect == EFF_POISON && weapon == WPN_DAGGER) {
        *n++ = 'P';
        *n++ = 'S';
        *n++ = 'N';
        *n++ = ' ';
    }
    while (*w) *n++ = *w++;
    *n = '\0';
}
void loot_synth_banked(void)
{
    CardId id = g_bk_byte_a;
    uint8_t material = loot_id_material(id);
    uint8_t effect = loot_id_effect(id);
    uint8_t weapon = loot_id_weapon(id);
    uint8_t *dst = (uint8_t *)&g_card_scratch;
    uint8_t n = sizeof(CardDefinition);
    uint8_t bonus = s_materials[material].power_bonus;

    while (n--) *dst++ = 0;   /* zero-fill, then set fields */

    g_card_scratch.id = id;
    g_card_scratch.type = CARD_TYPE_ATTACK;
    g_card_scratch.power = s_weapons[weapon].base_power + bonus;
    g_card_scratch.cost = s_weapons[weapon].cost;
    /* Centralized sell value (docs/loot.md §16/§34.6): weapon base +
     * material tier*2 (+2 poison rider).  Stored in the def's price
     * field so every consumer (shop SELL mode) reads it like any other
     * card price -- no separate formula anywhere else in the game. */
    g_card_scratch.price = (uint8_t)(s_weapons[weapon].base_power +
                                     ((uint8_t)material << 1) +
                                     ((effect == EFF_POISON &&
                                       weapon == WPN_DAGGER) ? 2 : 0));
    g_card_scratch.uses_per_battle = 0;          /* unlimited */
    g_card_scratch.max_copies = 0;               /* unlimited duplicates */

    switch (weapon) {
        case WPN_SWORD:
            g_card_scratch.battle_type = BATTLE_CARD_TYPE_SWORD;
            g_card_scratch.effect = CARD_EFFECT_DAMAGE_TARGET;
            break;
        case WPN_SHIELD:
            g_card_scratch.type = CARD_TYPE_DEFENSE;
            g_card_scratch.battle_type = BATTLE_CARD_TYPE_SHIELD;
            g_card_scratch.effect = CARD_EFFECT_BLOCK_DAMAGE;
            break;
        case WPN_BOW:
            g_card_scratch.battle_type = BATTLE_CARD_TYPE_BOW;
            g_card_scratch.effect = CARD_EFFECT_DAMAGE_TARGET;
            break;
        case WPN_DAGGER:
            g_card_scratch.battle_type = BATTLE_CARD_TYPE_DAGGER;
            g_card_scratch.effect = CARD_EFFECT_DAMAGE_TARGET;
            break;
        default:  /* ring: heals instead of dealing damage */
            g_card_scratch.type = CARD_TYPE_HEAL;
            g_card_scratch.battle_type = BATTLE_CARD_TYPE_HEAL;
            g_card_scratch.effect = CARD_EFFECT_HEAL_HP;
            break;
    }

    /* Effect rider: poison on daggers (fire/ice inherit their riders once
     * those statuses exist).  Written into the def so the existing
     * battle on-hit path consumes it unchanged. */
    if (effect == EFF_POISON && weapon == WPN_DAGGER) {
        g_card_scratch.status_id = s_effects[effect].status_id;
        g_card_scratch.status_chance = s_effects[effect].chance;
    }

    synth_name(material, effect, weapon);
}

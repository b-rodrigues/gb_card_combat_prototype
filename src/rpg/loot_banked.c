#pragma bank 2

#include "rpg/loot.h"
#include "banked.h"
#include "battle/card.h"
#include "rpg/cards.h"
#include "rpg/cards.h"

/* ── Loot identity tables + banked bodies (docs/loot.md §34) ────────
 * Runs from ROM bank 2 via the trampoline.  Two entry points:
 *   loot_roll_drop_banked   weapon(byte_a) -> derived id in g_loot_id
 *   loot_synth_banked       id(byte_a) -> CardDefinition in g_card_scratch
 * Both consume/write only WRAM and their own bank-local tables.
 * Extensible: append rows to grow materials/effects/weapons. */

#define STATUS_NONE    0
#define STATUS_POISON  1

static const struct {
    uint8_t power_bonus;   /* stacked onto weapon base power */
} s_materials[LOOT_NMATERIALS] = {
    { 0 }, { 1 }, { 2 }, { 3 }          /* wood bronze iron mythril */
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
    { STATUS_NONE,   0 },       /* ice reserved */
    { STATUS_NONE,   0 },       /* healing */
    { STATUS_NONE,   0 },       /* reserved */
    { STATUS_NONE,   0 },       /* reserved */
    { STATUS_NONE,   0 }        /* reserved */
};

static const struct {
    uint8_t base_power;
    uint8_t cost;
} s_weapons[LOOT_NWEAPONS] = {
    { 3, 1 },   /* sword */
    { 2, 1 },   /* shield */
    { 2, 1 },   /* bow */
    { 1, 1 },   /* dagger */
    { 2, 1 },   /* ring (heal amount = power) */
    { 0, 0 },   /* reserved */
    { 0, 0 },   /* reserved */
    { 0, 0 }    /* reserved */
};

/* Effect class -> CardEffectType, by weapon role:
 * attack weapons deal damage; shields block; rings heal. */
static uint8_t weapon_effect(uint8_t weapon)
{
    switch (weapon) {
        case WPN_SHIELD: return CARD_EFFECT_BLOCK_DAMAGE;
        case WPN_RING:   return CARD_EFFECT_HEAL_HP;
        default:         return CARD_EFFECT_DAMAGE_TARGET;
    }
}



/* Isolated xorshift: independent from the main game RNG so victory
 * drops never shift scenario-deterministic outcomes. */
static uint16_t s_loot_rng = 0x1234;

void loot_roll_drop_banked(void)
{
    uint8_t weapon = g_bk_byte_a;
    /* NO /, %, or * -- banked code cannot call SDCC div/mod/mul library */
    s_loot_rng ^= s_loot_rng << 7;
    s_loot_rng ^= s_loot_rng >> 9;
    s_loot_rng ^= s_loot_rng << 8;
    {
        uint8_t mat_roll = (uint8_t)s_loot_rng;
        uint8_t eff_roll = (uint8_t)(s_loot_rng >> 8);
        uint8_t material = (mat_roll < 128) ? MAT_WOOD :
                           (mat_roll < 192) ? MAT_BRONZE :
                           (mat_roll < 224) ? MAT_IRON : MAT_MYTHRIL;
        uint8_t effect = (eff_roll < 128 ||
                          weapon != WPN_DAGGER) ? EFF_PLAIN : EFF_POISON;
        /* derived id: BASE + mat*64 + eff*8 + wpn (shifts only) */
        uint16_t off = (((uint16_t)material << 6) +
                        ((uint16_t)effect << 3) + weapon);
        g_loot_id = (uint8_t)(LOOT_ID_BASE + (off & 0x3F));
    }
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
    g_card_scratch.uses_per_battle = 0;          /* unlimited */
    g_card_scratch.max_copies = 0;               /* unlimited duplicates */

    switch (weapon) {
        case WPN_SWORD:
            g_card_scratch.battle_type = BATTLE_CARD_TYPE_SWORD;
            g_card_scratch.effect = CARD_EFFECT_DAMAGE_TARGET;
            break;
        case WPN_SHIELD:
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
}

#pragma bank 2

#include "rpg/cards.h"
#include "battle/card.h"
#include "game_ids.h"

/* ── Card catalog (game content, banked ROM) ────────────────────── */
const CardDefinition g_cards[] = {
    { CARD_IRON_SWORD,    CARD_TYPE_ATTACK,  3, 1, 99, 1, CARD_EFFECT_DAMAGE_TARGET, BATTLE_CARD_TYPE_SWORD,    10, "IRON SW" },
    { CARD_WOODEN_SHIELD, CARD_TYPE_DEFENSE, 2, 1, 99, 1, CARD_EFFECT_BLOCK_DAMAGE,  BATTLE_CARD_TYPE_SHIELD,    0, "WD SHLD" },
    { CARD_HEALING_HERB,  CARD_TYPE_HEAL,    3, 1,  5, 3, CARD_EFFECT_HEAL_HP,       BATTLE_CARD_TYPE_HEAL,     20, "HERB"    },
    { CARD_FIRE_TOME,     CARD_TYPE_ATTACK,  4, 2,  3, 2, CARD_EFFECT_DAMAGE_TARGET, BATTLE_CARD_TYPE_FIRE,      0, "FIRE TM" },
    { CARD_POISON_DAGGER, CARD_TYPE_ATTACK,  2, 1, 99, 1, CARD_EFFECT_DAMAGE_TARGET, BATTLE_CARD_TYPE_SWORD,     0, "PST DAG" }
};

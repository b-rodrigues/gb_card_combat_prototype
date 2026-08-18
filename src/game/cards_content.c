#pragma bank 2

#include "rpg/cards.h"
#include "game_ids.h"

/* ── Card catalog (game content, banked ROM) ────────────────────── */
const CardDefinition g_cards[] = {
    { CARD_IRON_SWORD,    CARD_TYPE_ATTACK,  3, 99, 1, CARD_EFFECT_DAMAGE_TARGET, "IRON SW" },
    { CARD_WOODEN_SHIELD, CARD_TYPE_DEFENSE, 2, 99, 1, CARD_EFFECT_BLOCK_DAMAGE,  "WD SHLD" },
    { CARD_HEALING_HERB,  CARD_TYPE_HEAL,    3,  5, 3, CARD_EFFECT_HEAL_HP,       "HERB"    },
    { CARD_FIRE_TOME,     CARD_TYPE_ATTACK,  4,  3, 2, CARD_EFFECT_DAMAGE_TARGET, "FIRE TM" },
    { CARD_POISON_DAGGER, CARD_TYPE_ATTACK,  2, 99, 1, CARD_EFFECT_DAMAGE_TARGET, "PST DAG" }
};

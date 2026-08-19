#include "combatant.h"

void combatant_take_damage(Combatant *c, uint8_t damage)
{
    if (c) {
        c->hp = (damage >= c->hp) ? 0 : (uint8_t)(c->hp - damage);
    }
}

#include "combatant.h"

void combatant_init(Combatant *c, const char *name, uint8_t hp, uint8_t max_hp)
{
    if (c) {
        c->name = name;
        c->hp = hp;
        c->max_hp = max_hp;
    }
}

void combatant_take_damage(Combatant *c, uint8_t damage)
{
    if (c) {
        c->hp = (damage >= c->hp) ? 0 : (uint8_t)(c->hp - damage);
    }
}

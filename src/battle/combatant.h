#ifndef COMBATANT_H
#define COMBATANT_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t hp;
    uint8_t max_hp;
    uint8_t attack;
    const char *name;
} Combatant;

void combatant_init(Combatant *c, const char *name, uint8_t hp, uint8_t max_hp);
void combatant_take_damage(Combatant *c, uint8_t damage);
#define combatant_is_dead(c) (!(c) || (c)->hp == 0)

#endif /* COMBATANT_H */

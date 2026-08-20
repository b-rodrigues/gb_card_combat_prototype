#ifndef COMBATANT_H
#define COMBATANT_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t hp;
    uint8_t max_hp;
    const char *name;
} Combatant;

void combatant_take_damage(Combatant *c, uint8_t damage);

#endif /* COMBATANT_H */

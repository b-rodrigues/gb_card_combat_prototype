#ifndef RPG_PARTY_H
#define RPG_PARTY_H

#include <stdint.h>
#include <stdbool.h>
#include "rpg/state.h"

/* Look up a party member by character id, or NULL. */
CharacterState *party_get_member(PartyState *party, CharacterId id);

#endif /* RPG_PARTY_H */

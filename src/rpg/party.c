#include "rpg/party.h"
#include <stddef.h>

CharacterState *party_get_member(PartyState *party, CharacterId id)
{
    uint8_t i;
    if (!party) return NULL;
    for (i = 0; i < party->count; i++) {
        if (party->members[i].id == id) {
            return &party->members[i];
        }
    }
    return NULL;
}

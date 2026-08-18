#include "rpg/party.h"
#include <stddef.h>

void party_init(PartyState *party)
{
    uint8_t *p = (uint8_t *)party;
    uint8_t n = sizeof(PartyState);
    if (!party) return;
    while (n--) *p++ = 0;
}

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

void party_set_hp(PartyState *party, CharacterId id, uint8_t hp)
{
    CharacterState *m = party_get_member(party, id);
    if (!m) return;
    m->hp = hp;
}

uint8_t party_get_hp(const PartyState *party, CharacterId id)
{
    CharacterState *m = party_get_member((PartyState *)party, id);
    return m ? m->hp : 0;
}

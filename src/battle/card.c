#include "card.h"
#include "banked.h"

/* ── Battle card helpers ─────────────────────────────────────────────
 * Descriptions live in a packed ROM-bank-2 blob (src/battle/card_content.c)
 * and stage through banked_copy() into a WRAM scratch buffer, mirroring the
 * CardDefinition.name / SceneDefinition pattern -- the returned pointer is
 * valid until the next call.  Keeps ~130 B of strings + switch out of the
 * completely-full fixed bank (make memmap).  The sole caller (item_screen)
 * consumes the text immediately. */

extern const uint8_t s_card_desc_blob[CARD_DESC_TYPES * CARD_DESC_STRIDE];

static uint8_t s_desc_buf[CARD_DESC_STRIDE];

const char *card_get_description(uint8_t type)
{
    uint8_t off;
    if (type >= CARD_DESC_TYPES) {
        s_desc_buf[0] = '\0';
        return (const char *)s_desc_buf;
    }
    off = (uint8_t)((type << 4) + (type << 1)); /* type * 18 */
    banked_copy(2, s_desc_buf, &s_card_desc_blob[off], CARD_DESC_STRIDE);
    return (const char *)s_desc_buf;
}

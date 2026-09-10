#include "shops.h"
#include "game_ids.h"
#include "banked.h"
#include <stddef.h>

extern const ShopDefinition g_shops[];
extern const uint8_t g_shop_count;

static ShopDefinition s_shop_scratch;

const ShopDefinition *game_shop_for_id(uint8_t id)
{
    uint8_t i;

    /* The table is in bank 2; copy each row into WRAM scratch and match on
     * id.  Shops are few and small, so a linear scan is fine. */
    for (i = 0; i < g_shop_count; i++) {
        banked_copy(GAME_CONTENT_BANK, &s_shop_scratch, &g_shops[i],
                    sizeof(ShopDefinition));
        if (s_shop_scratch.id == id) return &s_shop_scratch;
    }
    return NULL;
}

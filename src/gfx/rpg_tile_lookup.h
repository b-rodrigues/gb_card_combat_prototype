#ifndef RPG_TILE_LOOKUP_H
#define RPG_TILE_LOOKUP_H

#include <stdint.h>

#define EXTERIOR_TILE_GRASS          0u
#define EXTERIOR_TILE_WALL           1u
#define EXTERIOR_TILE_EXIT_GATE      2u
#define EXTERIOR_TILE_BUILDING_WALL  3u

#define INTERIOR_TILE_FLOOR          0u
#define INTERIOR_TILE_WALL           1u
#define INTERIOR_TILE_DOOR           2u
#define INTERIOR_TILE_SOLID_PROP     3u

#define FOREST_TILE_FLOOR            0u
#define FOREST_TILE_TREE             1u
#define FOREST_TILE_GATE             2u
#define FOREST_TILE_STUMP_TL         3u
#define FOREST_TILE_STUMP_TR         4u
#define FOREST_TILE_STUMP_BL         5u
#define FOREST_TILE_STUMP_BR         6u
#define FOREST_TILE_STUMP_MINI       7u

#define RPG_TILE_BASE_WORLD          128u
#define RPG_TILE_BASE_EXTERIOR       128u
#define RPG_TILE_BASE_INTERIOR       128u
#define RPG_TILE_BASE_FOREST         128u
#define RPG_TILE_BASE_DESOLATE       128u

extern const uint8_t g_tileset_exterior[64];
extern const uint8_t g_tileset_interior[64];
extern const uint8_t g_tileset_forest[128];
extern const uint8_t g_tileset_desolate[768];
extern const uint8_t g_tileset_castle[432];
extern const uint8_t g_intrepid_font_tiles[1536];

static inline uint8_t rpg_lookup_tile_id(uint8_t tileset_kind, char glyph)
{
    if (tileset_kind == 15 /* WORLD_TILESET_CASTLE */) {
        if (glyph == '.') return (uint8_t)(RPG_TILE_BASE_WORLD + 18); /* Plain solid stone floor (0,2) */
        if (glyph == '#') return (uint8_t)(RPG_TILE_BASE_WORLD + 10); /* Castle wall */
        if (glyph == '>' || glyph == '<') return (uint8_t)(RPG_TILE_BASE_WORLD + 13); /* Archway gate */
        if (glyph == 'B' || glyph == '*') return (uint8_t)(RPG_TILE_BASE_WORLD + 16); /* Torch / prop */
        return 0;
    }
    if (tileset_kind == 14 /* WORLD_TILESET_DESOLATE */ && glyph == '.') {
        return (uint8_t)(RPG_TILE_BASE_DESOLATE + 32); /* Plain solid floor (0,2) */
    }
    if (tileset_kind == 2 /* WORLD_TILESET_FOREST */ && glyph >= '1' && glyph <= '4') {
        return (uint8_t)(RPG_TILE_BASE_WORLD + 3 + (glyph - '1'));
    }
    if (glyph == '.') return (uint8_t)(RPG_TILE_BASE_WORLD + 0);
    if (glyph == '#') return (uint8_t)(RPG_TILE_BASE_WORLD + 1);
    if (glyph == '>' || glyph == '<') return (uint8_t)(RPG_TILE_BASE_WORLD + 2);
    if (glyph == 'B' || glyph == '*') return (uint8_t)(RPG_TILE_BASE_WORLD + 3);
    return 0;
}

#endif /* RPG_TILE_LOOKUP_H */

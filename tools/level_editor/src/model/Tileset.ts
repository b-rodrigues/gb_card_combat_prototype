import castleJson from '../../tilesets/castle.json';
import combatJson from '../../tilesets/combat.json';
import forestJson from '../../tilesets/forest.json';
import desolateLandscapeJson from '../../tilesets/desolate_landscape.json';
import intrepidJson from '../../tilesets/intrepid.json';
import villageJson from '../../tilesets/village.json';

export interface TileDefinition {
  id: string;
  label: string;
  gb_constant: string;
  gb_tile_index?: number;
  walkable: boolean;
  color: string;
  ascii?: string;
  glyph?: string;
  image_url: string;
  category?: 'enemy' | 'npc' | 'terrain' | 'ui' | 'object' | 'wall' | 'nature' | 'building' | 'exit';
}

export interface TilesetDefinition {
  id: string;
  label: string;
  gb_tileset_kind: string;
  tiles: TileDefinition[];
}

export function parseTilesetJson(data: any): TilesetDefinition {
  return {
    id: data.id,
    label: data.label || data.id,
    gb_tileset_kind: data.gb_tileset_kind || 'WORLD_TILESET_DESOLATE',
    tiles: (data.tiles || []).map((t: any) => ({
      id: t.id,
      label: t.label || t.id,
      gb_constant: t.gb_constant || `TILE_${t.id.toUpperCase()}`,
      gb_tile_index: t.gb_tile_index,
      walkable: !!t.walkable,
      color: t.color || (t.walkable ? '#88c070' : '#205838'),
      ascii: t.ascii || (t.walkable ? '.' : '#'),
      glyph: (typeof t.glyph === 'string' && t.glyph.length === 1) ? t.glyph : undefined,
      image_url: t.image_url || `/tiles/${data.id}/${t.id}.png`,
      category: t.category || (t.walkable ? 'terrain' : 'terrain'),
    })),
  };
}

export const BUILTIN_TILESETS: Record<string, TilesetDefinition> = {
  castle: parseTilesetJson(castleJson),
  combat: parseTilesetJson(combatJson),
  forest: parseTilesetJson(forestJson),
  desolate_landscape: parseTilesetJson(desolateLandscapeJson),
  intrepid: parseTilesetJson(intrepidJson),
  village: parseTilesetJson(villageJson),
};

export const TILESET_CASTLE = BUILTIN_TILESETS.castle;
export const TILESET_COMBAT = BUILTIN_TILESETS.combat;
export const TILESET_FOREST = BUILTIN_TILESETS.forest;
export const TILESET_DESOLATE_LANDSCAPE = BUILTIN_TILESETS.desolate_landscape;
export const TILESET_INTREPID = BUILTIN_TILESETS.intrepid;
export const TILESET_VILLAGE = BUILTIN_TILESETS.village;

export function getTileset(id: string): TilesetDefinition {
  if (!BUILTIN_TILESETS[id]) {
    console.warn(`[Tileset] unknown tileset '${id}', falling back`);
  }
  return BUILTIN_TILESETS[id] || BUILTIN_TILESETS.desolate_landscape || BUILTIN_TILESETS.forest;
}

export function updateTilesetInMemory(tileset: TilesetDefinition): void {
  BUILTIN_TILESETS[tileset.id] = tileset;
}

export async function saveTilesetToServer(
  tileset: TilesetDefinition,
  images?: Record<string, string>
): Promise<{ success: boolean; path?: string; error?: string }> {
  try {
    const res = await fetch('/api/save-tileset', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        id: tileset.id,
        data: {
          id: tileset.id,
          label: tileset.label,
          gb_tileset_kind: tileset.gb_tileset_kind,
          tiles: tileset.tiles.map((t) => ({
            id: t.id,
            label: t.label,
            gb_constant: t.gb_constant,
            walkable: t.walkable,
            color: t.color,
            ascii: t.ascii,
            ...(t.glyph ? { glyph: t.glyph } : {}),
            image_url: t.image_url,
            category: t.category,
          })),
        },
        images,
      }),
    });
    const data = await res.json();
    if (!res.ok || !data.success) {
      throw new Error(data.error || `Server returned status ${res.status}`);
    }
    updateTilesetInMemory(tileset);
    return data;
  } catch (err: any) {
    return { success: false, error: err.message };
  }
}

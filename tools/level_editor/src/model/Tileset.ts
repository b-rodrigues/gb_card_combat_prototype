import forestJson from '../../tilesets/forest.json';
import exteriorJson from '../../tilesets/exterior.json';
import interiorJson from '../../tilesets/interior.json';
import dungeonJson from '../../tilesets/dungeon.json';
import housesWallsJson from '../../tilesets/houses_walls.json';
import housesRoofsJson from '../../tilesets/houses_roofs.json';
import housesFloorsJson from '../../tilesets/houses_floors.json';
import housesDoorsJson from '../../tilesets/houses_doors.json';
import housesWindowsJson from '../../tilesets/houses_windows.json';
import natureGroundJson from '../../tilesets/nature_ground.json';
import natureVegJson from '../../tilesets/nature_vegetation.json';
import objectsFurnitureJson from '../../tilesets/objects_furniture.json';
import structFencesJson from '../../tilesets/structures_fences.json';
import structPropsJson from '../../tilesets/structures_props.json';
import desolateJson from '../../tilesets/desolate_landscape.json';
import combatJson from '../../tilesets/combat.json';
import castleJson from '../../tilesets/castle.json';
import overworldJson from '../../tilesets/overworld.json';

export interface TileDefinition {
  id: string;
  label: string;
  gb_constant: string;
  gb_tile_index?: number;
  walkable: boolean;
  color: string;
  ascii?: string;
  image_url: string;
  category?: 'enemy' | 'npc' | 'terrain' | 'ui' | 'object';
}

export interface TilesetDefinition {
  id: string;
  label: string;
  gb_tileset_kind: string;
  tiles: TileDefinition[];
}

function parseTilesetJson(data: any): TilesetDefinition {
  return {
    id: data.id,
    label: data.label || data.id,
    gb_tileset_kind: data.gb_tileset_kind || 'WORLD_TILESET_EXTERIOR',
    tiles: (data.tiles || []).map((t: any) => ({
      id: t.id,
      label: t.label || t.id,
      gb_constant: t.gb_constant || `TILE_${t.id.toUpperCase()}`,
      gb_tile_index: t.gb_tile_index,
      walkable: !!t.walkable,
      color: t.color || (t.walkable ? '#88c070' : '#205838'),
      ascii: t.ascii || (t.walkable ? '.' : '#'),
      image_url: t.image_url || `/tiles/${data.id}/${t.id}.png`,
      category: t.category || (t.walkable ? 'terrain' : 'terrain'),
    })),
  };
}

export const BUILTIN_TILESETS: Record<string, TilesetDefinition> = {
  forest: parseTilesetJson(forestJson),
  exterior: parseTilesetJson(exteriorJson),
  interior: parseTilesetJson(interiorJson),
  dungeon: parseTilesetJson(dungeonJson),
  houses_walls: parseTilesetJson(housesWallsJson),
  houses_roofs: parseTilesetJson(housesRoofsJson),
  houses_floors: parseTilesetJson(housesFloorsJson),
  houses_doors: parseTilesetJson(housesDoorsJson),
  houses_windows: parseTilesetJson(housesWindowsJson),
  nature_ground: parseTilesetJson(natureGroundJson),
  nature_vegetation: parseTilesetJson(natureVegJson),
  objects_furniture: parseTilesetJson(objectsFurnitureJson),
  structures_fences: parseTilesetJson(structFencesJson),
  structures_props: parseTilesetJson(structPropsJson),
  desolate_landscape: parseTilesetJson(desolateJson),
  combat: parseTilesetJson(combatJson),
  castle: parseTilesetJson(castleJson),
  overworld: parseTilesetJson(overworldJson),
};

export const TILESET_FOREST = BUILTIN_TILESETS.forest;
export const TILESET_EXTERIOR = BUILTIN_TILESETS.exterior;
export const TILESET_INTERIOR = BUILTIN_TILESETS.interior;
export const TILESET_DUNGEON = BUILTIN_TILESETS.dungeon;
export const TILESET_HOUSES_WALLS = BUILTIN_TILESETS.houses_walls;
export const TILESET_HOUSES_ROOFS = BUILTIN_TILESETS.houses_roofs;
export const TILESET_HOUSES_FLOORS = BUILTIN_TILESETS.houses_floors;
export const TILESET_HOUSES_DOORS = BUILTIN_TILESETS.houses_doors;
export const TILESET_HOUSES_WINDOWS = BUILTIN_TILESETS.houses_windows;
export const TILESET_NATURE_GROUND = BUILTIN_TILESETS.nature_ground;
export const TILESET_NATURE_VEG = BUILTIN_TILESETS.nature_vegetation;
export const TILESET_OBJECTS_FURNITURE = BUILTIN_TILESETS.objects_furniture;
export const TILESET_STRUCT_FENCES = BUILTIN_TILESETS.structures_fences;
export const TILESET_STRUCT_PROPS = BUILTIN_TILESETS.structures_props;
export const TILESET_DESOLATE = BUILTIN_TILESETS.desolate_landscape;
export const TILESET_CASTLE = BUILTIN_TILESETS.castle;
export const TILESET_COMBAT = BUILTIN_TILESETS.combat;

export function getTileset(id: string): TilesetDefinition {
  return BUILTIN_TILESETS[id] || BUILTIN_TILESETS.exterior;
}

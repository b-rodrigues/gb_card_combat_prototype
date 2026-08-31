export interface TileDefinition {
  id: string;
  label: string;
  gb_constant: string;
  gb_tile_index?: number;
  walkable: boolean;
  color: string;
  ascii?: string;
  image_url: string;
}

export interface TilesetDefinition {
  id: string;
  label: string;
  gb_tileset_kind: string;
  tiles: TileDefinition[];
}

export const TILESET_FOREST: TilesetDefinition = {
  id: 'forest',
  label: 'Forest',
  gb_tileset_kind: 'WORLD_TILESET_FOREST',
  tiles: [
    { id: 'floor', label: 'Grass Floor', gb_constant: 'TILE_FLOOR', gb_tile_index: 0, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/forest/floor.png' },
    { id: 'tree', label: 'Dense Tree', gb_constant: 'TILE_WALL', gb_tile_index: 1, walkable: false, color: '#205838', ascii: '#', image_url: '/tiles/forest/tree.png' },
    { id: 'gate', label: 'Exit Gate', gb_constant: 'TILE_EXIT', gb_tile_index: 2, walkable: true, color: '#e0f8d0', ascii: '>', image_url: '/tiles/forest/gate.png' },
    { id: 'stump_tl', label: 'Stump Top-Left', gb_constant: 'TILE_STUMP_TL', gb_tile_index: 3, walkable: false, color: '#704820', ascii: 'p', image_url: '/tiles/forest/stump_tl.png' },
    { id: 'stump_tr', label: 'Stump Top-Right', gb_constant: 'TILE_STUMP_TR', gb_tile_index: 4, walkable: false, color: '#704820', ascii: 'q', image_url: '/tiles/forest/stump_tr.png' },
    { id: 'stump_bl', label: 'Stump Bot-Left', gb_constant: 'TILE_STUMP_BL', gb_tile_index: 5, walkable: false, color: '#704820', ascii: 'b', image_url: '/tiles/forest/stump_bl.png' },
    { id: 'stump_br', label: 'Stump Bot-Right', gb_constant: 'TILE_STUMP_BR', gb_tile_index: 6, walkable: false, color: '#704820', ascii: 'd', image_url: '/tiles/forest/stump_br.png' },
    { id: 'mini_stump', label: 'Mini Stump', gb_constant: 'TILE_WALL', gb_tile_index: 7, walkable: false, color: '#5a3818', ascii: 'o', image_url: '/tiles/forest/stump_mini.png' },
  ]
};

export const TILESET_EXTERIOR: TilesetDefinition = {
  id: 'exterior',
  label: 'Exterior / Plains',
  gb_tileset_kind: 'WORLD_TILESET_EXTERIOR',
  tiles: [
    { id: 'floor', label: 'Grass / Path', gb_constant: 'TILE_FLOOR', gb_tile_index: 0, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/exterior/grass.png' },
    { id: 'wall', label: 'Cliff / Wall', gb_constant: 'TILE_WALL', gb_tile_index: 1, walkable: false, color: '#346856', ascii: '#', image_url: '/tiles/exterior/wall.png' },
    { id: 'gate', label: 'Exit Gate', gb_constant: 'TILE_EXIT', gb_tile_index: 2, walkable: true, color: '#e0f8d0', ascii: '>', image_url: '/tiles/exterior/exit_gate.png' },
    { id: 'building', label: 'Building Wall', gb_constant: 'TILE_BUILDING', gb_tile_index: 3, walkable: false, color: '#081820', ascii: 'B', image_url: '/tiles/exterior/building_wall.png' },
  ]
};

export const TILESET_INTERIOR: TilesetDefinition = {
  id: 'interior',
  label: 'Interior / Castle',
  gb_tileset_kind: 'WORLD_TILESET_INTERIOR',
  tiles: [
    { id: 'floor', label: 'Flagstone Floor', gb_constant: 'TILE_FLOOR', gb_tile_index: 0, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/interior/floor.png' },
    { id: 'wall', label: 'Castle Wall', gb_constant: 'TILE_WALL', gb_tile_index: 1, walkable: false, color: '#346856', ascii: '#', image_url: '/tiles/interior/wall.png' },
    { id: 'door', label: 'Door / Exit', gb_constant: 'TILE_EXIT', gb_tile_index: 2, walkable: true, color: '#e0f8d0', ascii: '>', image_url: '/tiles/interior/door.png' },
    { id: 'building', label: 'Pillar / Prop', gb_constant: 'TILE_BUILDING', gb_tile_index: 3, walkable: false, color: '#081820', ascii: 'B', image_url: '/tiles/interior/solid_prop.png' },
  ]
};

export const BUILTIN_TILESETS: Record<string, TilesetDefinition> = {
  forest: TILESET_FOREST,
  exterior: TILESET_EXTERIOR,
  interior: TILESET_INTERIOR,
};

export function getTileset(id: string): TilesetDefinition {
  return BUILTIN_TILESETS[id] || TILESET_EXTERIOR;
}

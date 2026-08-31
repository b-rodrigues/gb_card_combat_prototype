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

export const TILESET_DUNGEON: TilesetDefinition = {
  id: 'dungeon',
  label: 'Dungeon',
  gb_tileset_kind: 'WORLD_TILESET_DUNGEON',
  tiles: [
    { id: 'floor_basic', label: 'Dungeon Floor', gb_constant: 'TILE_DUNGEON_FLOOR', gb_tile_index: 0, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/dungeon/dungeon_floor_00.png' },
    { id: 'wall_vertical', label: 'Dungeon Wall Vertical', gb_constant: 'TILE_DUNGEON_WALL', gb_tile_index: 1, walkable: false, color: '#205838', ascii: '#', image_url: '/tiles/dungeon/dungeon_wall_00.png' },
    { id: 'wall_horizontal', label: 'Dungeon Wall Horizontal', gb_constant: 'TILE_DUNGEON_WALL', gb_tile_index: 2, walkable: false, color: '#205838', ascii: '#', image_url: '/tiles/dungeon/dungeon_wall_01.png' },
    { id: 'wall_corner_tl', label: 'Dungeon Wall Corner TL', gb_constant: 'TILE_DUNGEON_WALL', gb_tile_index: 3, walkable: false, color: '#205838', ascii: 'l', image_url: '/tiles/dungeon/dungeon_wall_02.png' },
    { id: 'wall_corner_tr', label: 'Dungeon Wall Corner TR', gb_constant: 'TILE_DUNGEON_WALL', gb_tile_index: 4, walkable: false, color: '#205838', ascii: 'j', image_url: '/tiles/dungeon/dungeon_wall_03.png' },
    { id: 'wall_corner_bl', label: 'Dungeon Wall Corner BL', gb_constant: 'TILE_DUNGEON_WALL', gb_tile_index: 5, walkable: false, color: '#205838', ascii: 'b', image_url: '/tiles/dungeon/dungeon_wall_04.png' },
    { id: 'wall_corner_br', label: 'Dungeon Wall Corner BR', gb_constant: 'TILE_DUNGEON_WALL', gb_tile_index: 6, walkable: false, color: '#205838', ascii: 'd', image_url: '/tiles/dungeon/dungeon_wall_05.png' },
    { id: 'crack_fine', label: 'Dungeon Crack Fine', gb_constant: 'TILE_DUNGEON_CRACK', gb_tile_index: 7, walkable: false, color: '#704820', ascii: '.', image_url: '/tiles/dungeon/dungeon_decor_00.png' },
    { id: 'crack_coarse', label: 'Dungeon Crack Coarse', gb_constant: 'TILE_DUNGEON_CRACK', gb_tile_index: 8, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/dungeon/dungeon_decor_01.png' },
    { id: 'pillar_ns', label: 'Dungeon Pillar N-S', gb_constant: 'TILE_DUNGEON_PILLAR', gb_tile_index: 9, walkable: false, color: '#704820', ascii: 'I', image_url: '/tiles/dungeon/dungeon_pillar_ns.png' },
    { id: 'pillar_ew', label: 'Dungeon Pillar E-W', gb_constant: 'TILE_DUNGEON_PILLAR', gb_tile_index: 10, walkable: false, color: '#704820', ascii: 'T', image_url: '/tiles/dungeon/dungeon_pillar_ew.png' },
    { id: 'arch_north', label: 'Dungeon Arch North', gb_constant: 'TILE_DUNGEON_ARCH', gb_tile_index: 11, walkable: true, color: '#e0f8d0', ascii: '>', image_url: '/tiles/dungeon/dungeon_arch_north.png' },
    { id: 'arch_south', label: 'Dungeon Arch South', gb_constant: 'TILE_DUNGEON_ARCH', gb_tile_index: 12, walkable: true, color: '#e0f8d0', ascii: '<', image_url: '/tiles/dungeon/dungeon_arch_south.png' },
    { id: 'arch_east', label: 'Dungeon Arch East', gb_constant: 'TILE_DUNGEON_ARCH', gb_tile_index: 13, walkable: true, color: '#e0f8d0', ascii: '^', image_url: '/tiles/dungeon/dungeon_arch_east.png' },
    { id: 'arch_west', label: 'Dungeon Arch West', gb_constant: 'TILE_DUNGEON_ARCH', gb_tile_index: 14, walkable: true, color: '#e0f8d0', ascii: 'v', image_url: '/tiles/dungeon/dungeon_arch_west.png' },
    { id: 'brick_tl', label: 'Dungeon Brick TL', gb_constant: 'TILE_DUNGEON_WALL', gb_tile_index: 15, walkable: false, color: '#704820', ascii: 'p', image_url: '/tiles/dungeon/dungeon_brick_tl.png' },
    { id: 'brick_tr', label: 'Dungeon Brick TR', gb_constant: 'TILE_DUNGEON_WALL', gb_tile_index: 16, walkable: false, color: '#704820', ascii: 'q', image_url: '/tiles/dungeon/dungeon_brick_tr.png' },
    { id: 'brick_bl', label: 'Dungeon Brick BL', gb_constant: 'TILE_DUNGEON_WALL', gb_tile_index: 17, walkable: false, color: '#704820', ascii: 'b', image_url: '/tiles/dungeon/dungeon_brick_bl.png' },
    { id: 'brick_br', label: 'Dungeon Brick BR', gb_constant: 'TILE_DUNGEON_WALL', gb_tile_index: 18, walkable: false, color: '#704820', ascii: 'd', image_url: '/tiles/dungeon/dungeon_brick_br.png' },
    { id: 'solid_wall', label: 'Dungeon Solid Wall', gb_constant: 'TILE_DUNGEON_SOLID', gb_tile_index: 19, walkable: false, color: '#000000', ascii: '#', image_url: '/tiles/dungeon/dungeon_solid_wall.png' },
    { id: 'floor_stone', label: 'Dungeon Stone Floor', gb_constant: 'TILE_DUNGEON_FLOOR', gb_tile_index: 20, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/dungeon/dungeon_floor_stone.png' },
    { id: 'floor_dirt', label: 'Dungeon Dirt Floor', gb_constant: 'TILE_DUNGEON_FLOOR', gb_tile_index: 21, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/dungeon/dungeon_floor_dirt.png' },
    { id: 'wall_grate', label: 'Dungeon Grate Wall', gb_constant: 'TILE_DUNGEON_WALL', gb_tile_index: 22, walkable: true, color: '#e0f8d0', ascii: '.', image_url: '/tiles/dungeon/dungeon_wall_grate.png' },
    { id: 'door_vertical', label: 'Dungeon Door Vertical', gb_constant: 'TILE_DUNGEON_WALL', gb_tile_index: 23, walkable: false, color: '#205838', ascii: '|', image_url: '/tiles/dungeon/dungeon_door_vertical.png' },
    { id: 'door_horizontal', label: 'Dungeon Door Horizontal', gb_constant: 'TILE_DUNGEON_WALL', gb_tile_index: 24, walkable: false, color: '#205838', ascii: '-', image_url: '/tiles/dungeon/dungeon_door_horizontal.png' },
    { id: 'window_small', label: 'Dungeon Small Window', gb_constant: 'TILE_DUNGEON_WALL', gb_tile_index: 25, walkable: true, color: '#e0f8d0', ascii: '.', image_url: '/tiles/dungeon/dungeon_window_small.png' },
    { id: 'window_large', label: 'Dungeon Large Window', gb_constant: 'TILE_DUNGEON_WALL', gb_tile_index: 26, walkable: true, color: '#e0f8d0', ascii: '.', image_url: '/tiles/dungeon/dungeon_window_large.png' },
    { id: 'floor_mossy', label: 'Dungeon Mossy Floor', gb_constant: 'TILE_DUNGEON_FLOOR', gb_tile_index: 27, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/dungeon/dungeon_floor_mossy.png' }
  ]
};
  forest: TILESET_FOREST,
  exterior: TILESET_EXTERIOR,
  interior: TILESET_INTERIOR,
  dungeon: TILESET_DUNGEON,
};

export function getTileset(id: string): TilesetDefinition {
  return BUILTIN_TILESETS[id] || TILESET_EXTERIOR;
}

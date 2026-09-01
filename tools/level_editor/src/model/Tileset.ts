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
    { id: 'floor', label: 'Floor / Flagstone', gb_constant: 'TILE_FLOOR', gb_tile_index: 0, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/interior/floor.png' },
    { id: 'wall', label: 'Wall / Dungeon Wall', gb_constant: 'TILE_WALL', gb_tile_index: 1, walkable: false, color: '#346856', ascii: '#', image_url: '/tiles/interior/wall.png' },
    { id: 'door', label: 'Door / Exit', gb_constant: 'TILE_EXIT', gb_tile_index: 2, walkable: true, color: '#e0f8d0', ascii: '>', image_url: '/tiles/interior/door.png' },
    { id: 'building', label: 'Prop / Pillar', gb_constant: 'TILE_BUILDING', gb_tile_index: 3, walkable: false, color: '#081820', ascii: 'B', image_url: '/tiles/interior/pillar.png' },
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

export const TILESET_HOUSES_WALLS: TilesetDefinition = {
  id: 'houses_walls',
  label: 'Houses - Walls',
  gb_tileset_kind: 'WORLD_TILESET_HOUSES_WALLS',
  tiles: [
    { id: 'wall_00', label: 'House Wall 00', gb_constant: 'TILE_HOUSES_WALL_00', gb_tile_index: 0, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_00.png' },
    { id: 'wall_01', label: 'House Wall 01', gb_constant: 'TILE_HOUSES_WALL_01', gb_tile_index: 1, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_01.png' },
    { id: 'wall_02', label: 'House Wall 02', gb_constant: 'TILE_HOUSES_WALL_02', gb_tile_index: 2, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_02.png' },
    { id: 'wall_03', label: 'House Wall 03', gb_constant: 'TILE_HOUSES_WALL_03', gb_tile_index: 3, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_03.png' },
    { id: 'wall_04', label: 'House Wall 04', gb_constant: 'TILE_HOUSES_WALL_04', gb_tile_index: 4, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_04.png' },
    { id: 'wall_05', label: 'House Wall 05', gb_constant: 'TILE_HOUSES_WALL_05', gb_tile_index: 5, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_05.png' },
    { id: 'wall_06', label: 'House Wall 06', gb_constant: 'TILE_HOUSES_WALL_06', gb_tile_index: 6, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_06.png' },
    { id: 'wall_07', label: 'House Wall 07', gb_constant: 'TILE_HOUSES_WALL_07', gb_tile_index: 7, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_07.png' },
    { id: 'wall_08', label: 'House Wall 08', gb_constant: 'TILE_HOUSES_WALL_08', gb_tile_index: 8, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_08.png' },
    { id: 'wall_09', label: 'House Wall 09', gb_constant: 'TILE_HOUSES_WALL_09', gb_tile_index: 9, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_09.png' },
    { id: 'wall_10', label: 'House Wall 10', gb_constant: 'TILE_HOUSES_WALL_10', gb_tile_index: 10, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_10.png' },
    { id: 'wall_11', label: 'House Wall 11', gb_constant: 'TILE_HOUSES_WALL_11', gb_tile_index: 11, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_11.png' },
    { id: 'wall_12', label: 'House Wall 12', gb_constant: 'TILE_HOUSES_WALL_12', gb_tile_index: 12, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_12.png' },
    { id: 'wall_13', label: 'House Wall 13', gb_constant: 'TILE_HOUSES_WALL_13', gb_tile_index: 13, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_13.png' },
    { id: 'wall_14', label: 'House Wall 14', gb_constant: 'TILE_HOUSES_WALL_14', gb_tile_index: 14, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_14.png' },
    { id: 'wall_15', label: 'House Wall 15', gb_constant: 'TILE_HOUSES_WALL_15', gb_tile_index: 15, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_15.png' },
    { id: 'wall_16', label: 'House Wall 16', gb_constant: 'TILE_HOUSES_WALL_16', gb_tile_index: 16, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_16.png' },
    { id: 'wall_17', label: 'House Wall 17', gb_constant: 'TILE_HOUSES_WALL_17', gb_tile_index: 17, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_17.png' },
    { id: 'wall_18', label: 'House Wall 18', gb_constant: 'TILE_HOUSES_WALL_18', gb_tile_index: 18, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_18.png' },
    { id: 'wall_19', label: 'House Wall 19', gb_constant: 'TILE_HOUSES_WALL_19', gb_tile_index: 19, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_19.png' },
    { id: 'wall_20', label: 'House Wall 20', gb_constant: 'TILE_HOUSES_WALL_20', gb_tile_index: 20, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_20.png' },
    { id: 'wall_21', label: 'House Wall 21', gb_constant: 'TILE_HOUSES_WALL_21', gb_tile_index: 21, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_21.png' },
    { id: 'wall_22', label: 'House Wall 22', gb_constant: 'TILE_HOUSES_WALL_22', gb_tile_index: 22, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_22.png' },
    { id: 'wall_23', label: 'House Wall 23', gb_constant: 'TILE_HOUSES_WALL_23', gb_tile_index: 23, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_23.png' },
    { id: 'wall_24', label: 'House Wall 24', gb_constant: 'TILE_HOUSES_WALL_24', gb_tile_index: 24, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_24.png' },
    { id: 'wall_25', label: 'House Wall 25', gb_constant: 'TILE_HOUSES_WALL_25', gb_tile_index: 25, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_25.png' },
    { id: 'wall_26', label: 'House Wall 26', gb_constant: 'TILE_HOUSES_WALL_26', gb_tile_index: 26, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_26.png' },
    { id: 'wall_27', label: 'House Wall 27', gb_constant: 'TILE_HOUSES_WALL_27', gb_tile_index: 27, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/houses_walls/houses_wall_27.png' },
  ]
};

export const TILESET_HOUSES_ROOFS: TilesetDefinition = {
  id: 'houses_roofs',
  label: 'Houses - Roofs',
  gb_tileset_kind: 'WORLD_TILESET_HOUSES_ROOFS',
  tiles: [
    { id: 'roof_00', label: 'House Roof 00', gb_constant: 'TILE_HOUSES_ROOF_00', gb_tile_index: 0, walkable: false, color: '#704820', ascii: '^', image_url: '/tiles/houses_roofs/houses_roof_00.png' },
    { id: 'roof_01', label: 'House Roof 01', gb_constant: 'TILE_HOUSES_ROOF_01', gb_tile_index: 1, walkable: false, color: '#704820', ascii: '^', image_url: '/tiles/houses_roofs/houses_roof_01.png' },
    { id: 'roof_02', label: 'House Roof 02', gb_constant: 'TILE_HOUSES_ROOF_02', gb_tile_index: 2, walkable: false, color: '#704820', ascii: '^', image_url: '/tiles/houses_roofs/houses_roof_02.png' },
    { id: 'roof_03', label: 'House Roof 03', gb_constant: 'TILE_HOUSES_ROOF_03', gb_tile_index: 3, walkable: false, color: '#704820', ascii: '^', image_url: '/tiles/houses_roofs/houses_roof_03.png' },
    { id: 'roof_04', label: 'House Roof 04', gb_constant: 'TILE_HOUSES_ROOF_04', gb_tile_index: 4, walkable: false, color: '#704820', ascii: '^', image_url: '/tiles/houses_roofs/houses_roof_04.png' },
    { id: 'roof_05', label: 'House Roof 05', gb_constant: 'TILE_HOUSES_ROOF_05', gb_tile_index: 5, walkable: false, color: '#704820', ascii: '^', image_url: '/tiles/houses_roofs/houses_roof_05.png' },
    { id: 'roof_06', label: 'House Roof 06', gb_constant: 'TILE_HOUSES_ROOF_06', gb_tile_index: 6, walkable: false, color: '#704820', ascii: '^', image_url: '/tiles/houses_roofs/houses_roof_06.png' },
    { id: 'roof_07', label: 'House Roof 07', gb_constant: 'TILE_HOUSES_ROOF_07', gb_tile_index: 7, walkable: false, color: '#704820', ascii: '^', image_url: '/tiles/houses_roofs/houses_roof_07.png' },
    { id: 'roof_08', label: 'House Roof 08', gb_constant: 'TILE_HOUSES_ROOF_08', gb_tile_index: 8, walkable: false, color: '#704820', ascii: '^', image_url: '/tiles/houses_roofs/houses_roof_08.png' },
    { id: 'roof_09', label: 'House Roof 09', gb_constant: 'TILE_HOUSES_ROOF_09', gb_tile_index: 9, walkable: false, color: '#704820', ascii: '^', image_url: '/tiles/houses_roofs/houses_roof_09.png' },
    { id: 'roof_10', label: 'House Roof 10', gb_constant: 'TILE_HOUSES_ROOF_10', gb_tile_index: 10, walkable: false, color: '#704820', ascii: '^', image_url: '/tiles/houses_roofs/houses_roof_10.png' },
    { id: 'roof_11', label: 'House Roof 11', gb_constant: 'TILE_HOUSES_ROOF_11', gb_tile_index: 11, walkable: false, color: '#704820', ascii: '^', image_url: '/tiles/houses_roofs/houses_roof_11.png' },
    { id: 'roof_12', label: 'House Roof 12', gb_constant: 'TILE_HOUSES_ROOF_12', gb_tile_index: 12, walkable: false, color: '#704820', ascii: '^', image_url: '/tiles/houses_roofs/houses_roof_12.png' },
    { id: 'roof_13', label: 'House Roof 13', gb_constant: 'TILE_HOUSES_ROOF_13', gb_tile_index: 13, walkable: false, color: '#704820', ascii: '^', image_url: '/tiles/houses_roofs/houses_roof_13.png' },
  ]
};

export const TILESET_HOUSES_FLOORS: TilesetDefinition = {
  id: 'houses_floors',
  label: 'Houses - Floors',
  gb_tileset_kind: 'WORLD_TILESET_HOUSES_FLOORS',
  tiles: [
    { id: 'floor_00', label: 'House Floor 00', gb_constant: 'TILE_HOUSES_FLOOR_00', gb_tile_index: 0, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/houses_floors/houses_floor_00.png' },
    { id: 'floor_01', label: 'House Floor 01', gb_constant: 'TILE_HOUSES_FLOOR_01', gb_tile_index: 1, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/houses_floors/houses_floor_01.png' },
    { id: 'floor_02', label: 'House Floor 02', gb_constant: 'TILE_HOUSES_FLOOR_02', gb_tile_index: 2, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/houses_floors/houses_floor_02.png' },
    { id: 'floor_03', label: 'House Floor 03', gb_constant: 'TILE_HOUSES_FLOOR_03', gb_tile_index: 3, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/houses_floors/houses_floor_03.png' },
    { id: 'floor_04', label: 'House Floor 04', gb_constant: 'TILE_HOUSES_FLOOR_04', gb_tile_index: 4, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/houses_floors/houses_floor_04.png' },
    { id: 'floor_05', label: 'House Floor 05', gb_constant: 'TILE_HOUSES_FLOOR_05', gb_tile_index: 5, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/houses_floors/houses_floor_05.png' },
    { id: 'floor_06', label: 'House Floor 06', gb_constant: 'TILE_HOUSES_FLOOR_06', gb_tile_index: 6, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/houses_floors/houses_floor_06.png' },
    { id: 'floor_07', label: 'House Floor 07', gb_constant: 'TILE_HOUSES_FLOOR_07', gb_tile_index: 7, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/houses_floors/houses_floor_07.png' },
    { id: 'floor_08', label: 'House Floor 08', gb_constant: 'TILE_HOUSES_FLOOR_08', gb_tile_index: 8, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/houses_floors/houses_floor_08.png' },
    { id: 'floor_09', label: 'House Floor 09', gb_constant: 'TILE_HOUSES_FLOOR_09', gb_tile_index: 9, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/houses_floors/houses_floor_09.png' },
    { id: 'floor_10', label: 'House Floor 10', gb_constant: 'TILE_HOUSES_FLOOR_10', gb_tile_index: 10, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/houses_floors/houses_floor_10.png' },
    { id: 'floor_11', label: 'House Floor 11', gb_constant: 'TILE_HOUSES_FLOOR_11', gb_tile_index: 11, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/houses_floors/houses_floor_11.png' },
    { id: 'floor_12', label: 'House Floor 12', gb_constant: 'TILE_HOUSES_FLOOR_12', gb_tile_index: 12, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/houses_floors/houses_floor_12.png' },
    { id: 'floor_13', label: 'House Floor 13', gb_constant: 'TILE_HOUSES_FLOOR_13', gb_tile_index: 13, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/houses_floors/houses_floor_13.png' },
  ]
};

export const TILESET_HOUSES_DOORS: TilesetDefinition = {
  id: 'houses_doors',
  label: 'Houses - Doors',
  gb_tileset_kind: 'WORLD_TILESET_HOUSES_DOORS',
  tiles: [
    { id: 'door_00', label: 'House Door 00', gb_constant: 'TILE_HOUSES_DOOR_00', gb_tile_index: 0, walkable: true, color: '#e0f8d0', ascii: '>', image_url: '/tiles/houses_doors/houses_door_00.png' },
    { id: 'door_01', label: 'House Door 01', gb_constant: 'TILE_HOUSES_DOOR_01', gb_tile_index: 1, walkable: true, color: '#e0f8d0', ascii: '>', image_url: '/tiles/houses_doors/houses_door_01.png' },
    { id: 'door_02', label: 'House Door 02', gb_constant: 'TILE_HOUSES_DOOR_02', gb_tile_index: 2, walkable: true, color: '#e0f8d0', ascii: '>', image_url: '/tiles/houses_doors/houses_door_02.png' },
    { id: 'door_03', label: 'House Door 03', gb_constant: 'TILE_HOUSES_DOOR_03', gb_tile_index: 3, walkable: true, color: '#e0f8d0', ascii: '>', image_url: '/tiles/houses_doors/houses_door_03.png' },
    { id: 'door_04', label: 'House Door 04', gb_constant: 'TILE_HOUSES_DOOR_04', gb_tile_index: 4, walkable: true, color: '#e0f8d0', ascii: '>', image_url: '/tiles/houses_doors/houses_door_04.png' },
    { id: 'door_05', label: 'House Door 05', gb_constant: 'TILE_HOUSES_DOOR_05', gb_tile_index: 5, walkable: true, color: '#e0f8d0', ascii: '>', image_url: '/tiles/houses_doors/houses_door_05.png' },
  ]
};

export const TILESET_HOUSES_WINDOWS: TilesetDefinition = {
  id: 'houses_windows',
  label: 'Houses - Windows',
  gb_tileset_kind: 'WORLD_TILESET_HOUSES_WINDOWS',
  tiles: [
    { id: 'window_00', label: 'House Window 00', gb_constant: 'TILE_HOUSES_WINDOW_00', gb_tile_index: 0, walkable: true, color: '#e0f8d0', ascii: '.', image_url: '/tiles/houses_windows/houses_window_00.png' },
    { id: 'window_01', label: 'House Window 01', gb_constant: 'TILE_HOUSES_WINDOW_01', gb_tile_index: 1, walkable: true, color: '#e0f8d0', ascii: '.', image_url: '/tiles/houses_windows/houses_window_01.png' },
    { id: 'window_02', label: 'House Window 02', gb_constant: 'TILE_HOUSES_WINDOW_02', gb_tile_index: 2, walkable: true, color: '#e0f8d0', ascii: '.', image_url: '/tiles/houses_windows/houses_window_02.png' },
    { id: 'window_03', label: 'House Window 03', gb_constant: 'TILE_HOUSES_WINDOW_03', gb_tile_index: 3, walkable: true, color: '#e0f8d0', ascii: '.', image_url: '/tiles/houses_windows/houses_window_03.png' },
    { id: 'window_04', label: 'House Window 04', gb_constant: 'TILE_HOUSES_WINDOW_04', gb_tile_index: 4, walkable: true, color: '#e0f8d0', ascii: '.', image_url: '/tiles/houses_windows/houses_window_04.png' },
    { id: 'window_05', label: 'House Window 05', gb_constant: 'TILE_HOUSES_WINDOW_05', gb_tile_index: 5, walkable: true, color: '#e0f8d0', ascii: '.', image_url: '/tiles/houses_windows/houses_window_05.png' },
  ]
};

export const TILESET_NATURE_GROUND: TilesetDefinition = {
  id: 'nature_ground',
  label: 'Nature - Ground',
  gb_tileset_kind: 'WORLD_TILESET_NATURE_GROUND',
  tiles: [
    { id: 'ground_00', label: 'Nature Ground 00', gb_constant: 'TILE_NATURE_GROUND_00', gb_tile_index: 0, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/nature_ground/nature_ground_00.png' },
    { id: 'ground_01', label: 'Nature Ground 01', gb_constant: 'TILE_NATURE_GROUND_01', gb_tile_index: 1, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/nature_ground/nature_ground_01.png' },
    { id: 'ground_02', label: 'Nature Ground 02', gb_constant: 'TILE_NATURE_GROUND_02', gb_tile_index: 2, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/nature_ground/nature_ground_02.png' },
    { id: 'ground_03', label: 'Nature Ground 03', gb_constant: 'TILE_NATURE_GROUND_03', gb_tile_index: 3, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/nature_ground/nature_ground_03.png' },
    { id: 'ground_04', label: 'Nature Ground 04', gb_constant: 'TILE_NATURE_GROUND_04', gb_tile_index: 4, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/nature_ground/nature_ground_04.png' },
    { id: 'ground_05', label: 'Nature Ground 05', gb_constant: 'TILE_NATURE_GROUND_05', gb_tile_index: 5, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/nature_ground/nature_ground_05.png' },
    { id: 'ground_06', label: 'Nature Ground 06', gb_constant: 'TILE_NATURE_GROUND_06', gb_tile_index: 6, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/nature_ground/nature_ground_06.png' },
    { id: 'ground_07', label: 'Nature Ground 07', gb_constant: 'TILE_NATURE_GROUND_07', gb_tile_index: 7, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/nature_ground/nature_ground_07.png' },
    { id: 'ground_08', label: 'Nature Ground 08', gb_constant: 'TILE_NATURE_GROUND_08', gb_tile_index: 8, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/nature_ground/nature_ground_08.png' },
    { id: 'ground_09', label: 'Nature Ground 09', gb_constant: 'TILE_NATURE_GROUND_09', gb_tile_index: 9, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/nature_ground/nature_ground_09.png' },
    { id: 'ground_10', label: 'Nature Ground 10', gb_constant: 'TILE_NATURE_GROUND_10', gb_tile_index: 10, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/nature_ground/nature_ground_10.png' },
    { id: 'ground_11', label: 'Nature Ground 11', gb_constant: 'TILE_NATURE_GROUND_11', gb_tile_index: 11, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/nature_ground/nature_ground_11.png' },
    { id: 'ground_12', label: 'Nature Ground 12', gb_constant: 'TILE_NATURE_GROUND_12', gb_tile_index: 12, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/nature_ground/nature_ground_12.png' },
    { id: 'ground_13', label: 'Nature Ground 13', gb_constant: 'TILE_NATURE_GROUND_13', gb_tile_index: 13, walkable: true, color: '#88c070', ascii: '.', image_url: '/tiles/nature_ground/nature_ground_13.png' },
  ]
};

export const TILESET_NATURE_VEG: TilesetDefinition = {
  id: 'nature_vegetation',
  label: 'Nature - Vegetation',
  gb_tileset_kind: 'WORLD_TILESET_NATURE_VEGETATION',
  tiles: [
    { id: 'veg_00', label: 'Nature Vegetation 00', gb_constant: 'TILE_NATURE_VEG_00', gb_tile_index: 0, walkable: false, color: '#205838', ascii: '#', image_url: '/tiles/nature_vegetation/nature_veg_00.png' },
    { id: 'veg_01', label: 'Nature Vegetation 01', gb_constant: 'TILE_NATURE_VEG_01', gb_tile_index: 1, walkable: false, color: '#205838', ascii: '#', image_url: '/tiles/nature_vegetation/nature_veg_01.png' },
    { id: 'veg_02', label: 'Nature Vegetation 02', gb_constant: 'TILE_NATURE_VEG_02', gb_tile_index: 2, walkable: false, color: '#205838', ascii: '#', image_url: '/tiles/nature_vegetation/nature_veg_02.png' },
    { id: 'veg_03', label: 'Nature Vegetation 03', gb_constant: 'TILE_NATURE_VEG_03', gb_tile_index: 3, walkable: false, color: '#205838', ascii: '#', image_url: '/tiles/nature_vegetation/nature_veg_03.png' },
    { id: 'veg_04', label: 'Nature Vegetation 04', gb_constant: 'TILE_NATURE_VEG_04', gb_tile_index: 4, walkable: false, color: '#205838', ascii: '#', image_url: '/tiles/nature_vegetation/nature_veg_04.png' },
    { id: 'veg_05', label: 'Nature Vegetation 05', gb_constant: 'TILE_NATURE_VEG_05', gb_tile_index: 5, walkable: false, color: '#205838', ascii: '#', image_url: '/tiles/nature_vegetation/nature_veg_05.png' },
    { id: 'veg_06', label: 'Nature Vegetation 06', gb_constant: 'TILE_NATURE_VEG_06', gb_tile_index: 6, walkable: false, color: '#205838', ascii: '#', image_url: '/tiles/nature_vegetation/nature_veg_06.png' },
    { id: 'veg_07', label: 'Nature Vegetation 07', gb_constant: 'TILE_NATURE_VEG_07', gb_tile_index: 7, walkable: false, color: '#205838', ascii: '#', image_url: '/tiles/nature_vegetation/nature_veg_07.png' },
    { id: 'veg_08', label: 'Nature Vegetation 08', gb_constant: 'TILE_NATURE_VEG_08', gb_tile_index: 8, walkable: false, color: '#205838', ascii: '#', image_url: '/tiles/nature_vegetation/nature_veg_08.png' },
    { id: 'veg_09', label: 'Nature Vegetation 09', gb_constant: 'TILE_NATURE_VEG_09', gb_tile_index: 9, walkable: false, color: '#205838', ascii: '#', image_url: '/tiles/nature_vegetation/nature_veg_09.png' },
    { id: 'veg_10', label: 'Nature Vegetation 10', gb_constant: 'TILE_NATURE_VEG_10', gb_tile_index: 10, walkable: false, color: '#205838', ascii: '#', image_url: '/tiles/nature_vegetation/nature_veg_10.png' },
    { id: 'veg_11', label: 'Nature Vegetation 11', gb_constant: 'TILE_NATURE_VEG_11', gb_tile_index: 11, walkable: false, color: '#205838', ascii: '#', image_url: '/tiles/nature_vegetation/nature_veg_11.png' },
    { id: 'veg_12', label: 'Nature Vegetation 12', gb_constant: 'TILE_NATURE_VEG_12', gb_tile_index: 12, walkable: false, color: '#205838', ascii: '#', image_url: '/tiles/nature_vegetation/nature_veg_12.png' },
    { id: 'veg_13', label: 'Nature Vegetation 13', gb_constant: 'TILE_NATURE_VEG_13', gb_tile_index: 13, walkable: false, color: '#205838', ascii: '#', image_url: '/tiles/nature_vegetation/nature_veg_13.png' },
  ]
};

export const TILESET_OBJECTS_FURNITURE: TilesetDefinition = {
  id: 'objects_furniture',
  label: 'Objects - Furniture',
  gb_tileset_kind: 'WORLD_TILESET_OBJECTS_FURNITURE',
  tiles: [
    { id: 'furn_00', label: 'Object Furniture 00', gb_constant: 'TILE_OBJECTS_FURN_00', gb_tile_index: 0, walkable: false, color: '#704820', ascii: 'B', image_url: '/tiles/objects_furniture/objects_furn_00.png' },
    { id: 'furn_01', label: 'Object Furniture 01', gb_constant: 'TILE_OBJECTS_FURN_01', gb_tile_index: 1, walkable: false, color: '#704820', ascii: 'B', image_url: '/tiles/objects_furniture/objects_furn_01.png' },
    { id: 'furn_02', label: 'Object Furniture 02', gb_constant: 'TILE_OBJECTS_FURN_02', gb_tile_index: 2, walkable: false, color: '#704820', ascii: 'B', image_url: '/tiles/objects_furniture/objects_furn_02.png' },
    { id: 'furn_03', label: 'Object Furniture 03', gb_constant: 'TILE_OBJECTS_FURN_03', gb_tile_index: 3, walkable: false, color: '#704820', ascii: 'B', image_url: '/tiles/objects_furniture/objects_furn_03.png' },
    { id: 'furn_04', label: 'Object Furniture 04', gb_constant: 'TILE_OBJECTS_FURN_04', gb_tile_index: 4, walkable: false, color: '#704820', ascii: 'B', image_url: '/tiles/objects_furniture/objects_furn_04.png' },
    { id: 'furn_05', label: 'Object Furniture 05', gb_constant: 'TILE_OBJECTS_FURN_05', gb_tile_index: 5, walkable: false, color: '#704820', ascii: 'B', image_url: '/tiles/objects_furniture/objects_furn_05.png' },
    { id: 'furn_06', label: 'Object Furniture 06', gb_constant: 'TILE_OBJECTS_FURN_06', gb_tile_index: 6, walkable: false, color: '#704820', ascii: 'B', image_url: '/tiles/objects_furniture/objects_furn_06.png' },
    { id: 'furn_07', label: 'Object Furniture 07', gb_constant: 'TILE_OBJECTS_FURN_07', gb_tile_index: 7, walkable: false, color: '#704820', ascii: 'B', image_url: '/tiles/objects_furniture/objects_furn_07.png' },
    { id: 'furn_08', label: 'Object Furniture 08', gb_constant: 'TILE_OBJECTS_FURN_08', gb_tile_index: 8, walkable: false, color: '#704820', ascii: 'B', image_url: '/tiles/objects_furniture/objects_furn_08.png' },
    { id: 'furn_09', label: 'Object Furniture 09', gb_constant: 'TILE_OBJECTS_FURN_09', gb_tile_index: 9, walkable: false, color: '#704820', ascii: 'B', image_url: '/tiles/objects_furniture/objects_furn_09.png' },
    { id: 'furn_10', label: 'Object Furniture 10', gb_constant: 'TILE_OBJECTS_FURN_10', gb_tile_index: 10, walkable: false, color: '#704820', ascii: 'B', image_url: '/tiles/objects_furniture/objects_furn_10.png' },
    { id: 'furn_11', label: 'Object Furniture 11', gb_constant: 'TILE_OBJECTS_FURN_11', gb_tile_index: 11, walkable: false, color: '#704820', ascii: 'B', image_url: '/tiles/objects_furniture/objects_furn_11.png' },
    { id: 'furn_12', label: 'Object Furniture 12', gb_constant: 'TILE_OBJECTS_FURN_12', gb_tile_index: 12, walkable: false, color: '#704820', ascii: 'B', image_url: '/tiles/objects_furniture/objects_furn_12.png' },
    { id: 'furn_13', label: 'Object Furniture 13', gb_constant: 'TILE_OBJECTS_FURN_13', gb_tile_index: 13, walkable: false, color: '#704820', ascii: 'B', image_url: '/tiles/objects_furniture/objects_furn_13.png' },
  ]
};

export const TILESET_STRUCT_FENCES: TilesetDefinition = {
  id: 'structures_fences',
  label: 'Structures - Fences',
  gb_tileset_kind: 'WORLD_TILESET_STRUCTURES_FENCES',
  tiles: [
    { id: 'fence_00', label: 'Structure Fence 00', gb_constant: 'TILE_STRUCT_FENCE_00', gb_tile_index: 0, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/structures_fences/struct_fence_00.png' },
    { id: 'fence_01', label: 'Structure Fence 01', gb_constant: 'TILE_STRUCT_FENCE_01', gb_tile_index: 1, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/structures_fences/struct_fence_01.png' },
    { id: 'fence_02', label: 'Structure Fence 02', gb_constant: 'TILE_STRUCT_FENCE_02', gb_tile_index: 2, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/structures_fences/struct_fence_02.png' },
    { id: 'fence_03', label: 'Structure Fence 03', gb_constant: 'TILE_STRUCT_FENCE_03', gb_tile_index: 3, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/structures_fences/struct_fence_03.png' },
    { id: 'fence_04', label: 'Structure Fence 04', gb_constant: 'TILE_STRUCT_FENCE_04', gb_tile_index: 4, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/structures_fences/struct_fence_04.png' },
    { id: 'fence_05', label: 'Structure Fence 05', gb_constant: 'TILE_STRUCT_FENCE_05', gb_tile_index: 5, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/structures_fences/struct_fence_05.png' },
    { id: 'fence_06', label: 'Structure Fence 06', gb_constant: 'TILE_STRUCT_FENCE_06', gb_tile_index: 6, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/structures_fences/struct_fence_06.png' },
    { id: 'fence_07', label: 'Structure Fence 07', gb_constant: 'TILE_STRUCT_FENCE_07', gb_tile_index: 7, walkable: false, color: '#704820', ascii: '#', image_url: '/tiles/structures_fences/struct_fence_07.png' },
  ]
};

export const TILESET_STRUCT_PROPS: TilesetDefinition = {
  id: 'structures_props',
  label: 'Structures - Props',
  gb_tileset_kind: 'WORLD_TILESET_STRUCTURES_PROPS',
  tiles: [
    { id: 'prop_00', label: 'Structure Prop 00', gb_constant: 'TILE_STRUCT_PROP_00', gb_tile_index: 0, walkable: false, color: '#000000', ascii: 'P', image_url: '/tiles/structures_props/struct_prop_00.png' },
    { id: 'prop_01', label: 'Structure Prop 01', gb_constant: 'TILE_STRUCT_PROP_01', gb_tile_index: 1, walkable: false, color: '#000000', ascii: 'P', image_url: '/tiles/structures_props/struct_prop_01.png' },
    { id: 'prop_02', label: 'Structure Prop 02', gb_constant: 'TILE_STRUCT_PROP_02', gb_tile_index: 2, walkable: false, color: '#000000', ascii: 'P', image_url: '/tiles/structures_props/struct_prop_02.png' },
    { id: 'prop_03', label: 'Structure Prop 03', gb_constant: 'TILE_STRUCT_PROP_03', gb_tile_index: 3, walkable: false, color: '#000000', ascii: 'P', image_url: '/tiles/structures_props/struct_prop_03.png' },
    { id: 'prop_04', label: 'Structure Prop 04', gb_constant: 'TILE_STRUCT_PROP_04', gb_tile_index: 4, walkable: false, color: '#000000', ascii: 'P', image_url: '/tiles/structures_props/struct_prop_04.png' },
    { id: 'prop_05', label: 'Structure Prop 05', gb_constant: 'TILE_STRUCT_PROP_05', gb_tile_index: 5, walkable: false, color: '#000000', ascii: 'P', image_url: '/tiles/structures_props/struct_prop_05.png' },
    { id: 'prop_06', label: 'Structure Prop 06', gb_constant: 'TILE_STRUCT_PROP_06', gb_tile_index: 6, walkable: false, color: '#000000', ascii: 'P', image_url: '/tiles/structures_props/struct_prop_06.png' },
    { id: 'prop_07', label: 'Structure Prop 07', gb_constant: 'TILE_STRUCT_PROP_07', gb_tile_index: 7, walkable: false, color: '#000000', ascii: 'P', image_url: '/tiles/structures_props/struct_prop_07.png' },
  ]
};

export const BUILTIN_TILESETS: Record<string, TilesetDefinition> = {
  forest: TILESET_FOREST,
  exterior: TILESET_EXTERIOR,
  interior: TILESET_INTERIOR,
  dungeon: TILESET_DUNGEON,
  houses_walls: TILESET_HOUSES_WALLS,
  houses_roofs: TILESET_HOUSES_ROOFS,
  houses_floors: TILESET_HOUSES_FLOORS,
  houses_doors: TILESET_HOUSES_DOORS,
  houses_windows: TILESET_HOUSES_WINDOWS,
  nature_ground: TILESET_NATURE_GROUND,
  nature_vegetation: TILESET_NATURE_VEG,
  objects_furniture: TILESET_OBJECTS_FURNITURE,
  structures_fences: TILESET_STRUCT_FENCES,
  structures_props: TILESET_STRUCT_PROPS,
};

export function getTileset(id: string): TilesetDefinition {
  return BUILTIN_TILESETS[id] || TILESET_EXTERIOR;
}

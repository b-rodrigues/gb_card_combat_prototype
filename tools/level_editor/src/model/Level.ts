import { LevelObject } from './Objects';
import { getTileset } from './Tileset';

export interface LevelExit {
  x: number;
  y: number;
  target_scene: string;
  target_x: number;
  target_y: number;
  direction?: 'NORTH' | 'SOUTH' | 'EAST' | 'WEST' | string;
  tile_char?: string;
}

export interface LevelRegion {
  id: string;
  bounds: {
    x: number;
    y: number;
    width: number;
    height: number;
  };
  description?: string;
  gameplay?: {
    purpose?: string;
    difficulty?: number;
  };
}

export interface PlayerSpawn {
  x: number;
  y: number;
  facing?: 'UP' | 'DOWN' | 'LEFT' | 'RIGHT' | string;
}

export interface MapMetadata {
  width: number;
  height: number;
  tileset: string;
  music?: string;
  map_id?: string;
}

export interface LevelData {
  id: string;
  name: string;
  map: MapMetadata;
  layers: {
    terrain: string[][] | Array<{ x: number; y: number; width: number; height: number; tile: string }>;
    blocks?: Array<{ x: number; y: number; width: number; height: number; tile: string }>;
  };
  collision?: {
    overrides?: Array<{ x: number; y: number; walkable: boolean }>;
  };
  player: {
    spawn: PlayerSpawn;
  };
  exits: LevelExit[];
  objects: LevelObject[];
  regions: LevelRegion[];
}

export interface EditorLevel {
  id: string;
  name: string;
  width: number;
  height: number;
  tileset: string;
  music: string;
  mapId: string;
  grid: string[][]; // tile names, e.g. "tree", "floor"
  spawn: PlayerSpawn;
  exits: LevelExit[];
  objects: LevelObject[];
  regions: LevelRegion[];
}

export function createEmptyEditorLevel(id: string = 'new_scene', tilesetId: string = 'forest', width: number = 20, height: number = 18): EditorLevel {
  const grid: string[][] = [];
  for (let y = 0; y < height; y++) {
    const row: string[] = [];
    for (let x = 0; x < width; x++) {
      row.push('floor');
    }
    grid.push(row);
  }

  return {
    id,
    name: id.replace(/_/g, ' ').replace(/\b\w/g, c => c.toUpperCase()),
    width,
    height,
    tileset: tilesetId,
    music: 'MUSIC_OVERWORLD',
    mapId: `MAP_${id.toUpperCase()}`,
    grid,
    spawn: { x: Math.floor(width / 2), y: Math.floor(height / 2), facing: 'DOWN' },
    exits: [],
    objects: [],
    regions: []
  };
}

export function levelDataToEditor(data: LevelData): EditorLevel {
  const w = data.map.width;
  const h = data.map.height;
  const tilesetId = data.map.tileset || 'forest';
  const tileset = getTileset(tilesetId);
  const defaultFloor = 'floor';

  const grid: string[][] = [];
  for (let y = 0; y < h; y++) {
    const row: string[] = [];
    for (let x = 0; x < w; x++) {
      row.push(defaultFloor);
    }
    grid.push(row);
  }

  const rawTerrain = data.layers?.terrain;
  if (Array.isArray(rawTerrain) && rawTerrain.length > 0) {
    if (Array.isArray(rawTerrain[0])) {
      // 2D grid format
      const rawGrid = rawTerrain as string[][];
      for (let y = 0; y < Math.min(h, rawGrid.length); y++) {
        for (let x = 0; x < Math.min(w, rawGrid[y].length); x++) {
          const rawTile = rawGrid[y][x] || defaultFloor;
          const cleanTile = rawTile.includes('.') ? rawTile.split('.')[1] : rawTile;
          grid[y][x] = cleanTile;
        }
      }
    } else {
      // Block format
      const blocks = rawTerrain as Array<{ x: number; y: number; width: number; height: number; tile: string }>;
      for (const b of blocks) {
        const cleanTile = b.tile.includes('.') ? b.tile.split('.')[1] : b.tile;
        for (let cy = b.y; cy < Math.min(h, b.y + b.height); cy++) {
          for (let cx = b.x; cx < Math.min(w, b.x + b.width); cx++) {
            grid[cy][cx] = cleanTile;
          }
        }
      }
    }
  }

  return {
    id: data.id || 'scene',
    name: data.name || data.id || 'Scene',
    width: w,
    height: h,
    tileset: tilesetId,
    music: data.map.music || 'MUSIC_OVERWORLD',
    mapId: data.map.map_id || `MAP_${(data.id || 'scene').toUpperCase()}`,
    grid,
    spawn: data.player?.spawn || { x: Math.floor(w / 2), y: Math.floor(h / 2), facing: 'DOWN' },
    exits: data.exits || [],
    objects: data.objects || [],
    regions: data.regions || []
  };
}

export function editorToLevelData(lvl: EditorLevel): LevelData {
  return {
    id: lvl.id,
    name: lvl.name,
    map: {
      width: lvl.width,
      height: lvl.height,
      tileset: lvl.tileset,
      music: lvl.music,
      map_id: lvl.mapId
    },
    layers: {
      terrain: lvl.grid.map(row => row.map(t => `${lvl.tileset}.${t}`))
    },
    player: {
      spawn: lvl.spawn
    },
    exits: lvl.exits,
    objects: lvl.objects,
    regions: lvl.regions
  };
}

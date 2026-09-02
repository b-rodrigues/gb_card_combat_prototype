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
  isScreen?: boolean;
  originalScreenData?: any;
}

export function titleScreenToEditor(data: any): EditorLevel {
  const w = data.grid_width || 20;
  const h = data.grid_height || 18;
  const tilesetId = 'interior';
  const grid: string[][] = [];

  for (let y = 0; y < h; y++) {
    const row: string[] = [];
    for (let x = 0; x < w; x++) {
      if (y === 0 || y === h - 1 || x === 0 || x === w - 1) {
        row.push('wall');
      } else {
        row.push('floor');
      }
    }
    grid.push(row);
  }

  const objects: LevelObject[] = [
    {
      id: 'title_logo',
      type: 'npc',
      position: { x: data.logo?.x ?? 2, y: data.logo?.y ?? 2 },
      properties: {
        sprite_width: 2,
        sprite_height: 1,
        display_name: data.logo?.lines?.[0]?.trim() || data.title || 'GIAUSAR',
        dialogue_id: data.logo?.lines ? data.logo.lines.join('\n') : data.title
      }
    },
    {
      id: 'title_prompt',
      type: 'npc',
      position: { x: data.prompt?.x ?? 5, y: data.prompt?.y ?? 16 },
      properties: {
        display_name: data.prompt?.text || 'PRESS START',
        dialogue_id: 'Press START'
      }
    }
  ];

  if (Array.isArray(data.menu?.options)) {
    data.menu.options.forEach((opt: string, idx: number) => {
      const optY = (data.menu.first_row ?? 10) + idx * (data.menu.row_step ?? 2);
      objects.push({
        id: `title_menu_opt_${idx + 1}`,
        type: 'npc',
        position: { x: data.menu.x ?? 3, y: optY },
        properties: {
          display_name: opt,
          dialogue_id: `Option ${idx + 1}: ${opt}`
        }
      });
    });
  }

  const regions: LevelRegion[] = [
    {
      id: 'logo_banner',
      bounds: { x: 1, y: 1, width: 18, height: 6 },
      description: 'Title Screen Logo & Subtitle'
    },
    {
      id: 'menu_zone',
      bounds: { x: 2, y: 9, width: 16, height: 8 },
      description: 'Title Menu Options'
    }
  ];

  return {
    id: 'title',
    name: data.title ? `Title Screen (${data.title})` : 'Title Screen',
    width: w,
    height: h,
    tileset: tilesetId,
    music: 'MUSIC_TITLE',
    mapId: 'SCREEN_TITLE',
    grid,
    spawn: { x: data.menu?.caret_x ?? 3, y: data.menu?.first_row ?? 10, facing: 'RIGHT' },
    exits: [],
    objects,
    regions,
    isScreen: true,
    originalScreenData: data
  };
}

export function battleScreenToEditor(data: any): EditorLevel {
  const w = data.grid_width || 20;
  const h = data.grid_height || 18;
  const tilesetId = 'dungeon';
  const grid: string[][] = [];

  for (let y = 0; y < h; y++) {
    const row: string[] = [];
    for (let x = 0; x < w; x++) {
      if (y === 0 || y === h - 1 || x === 0 || x === w - 1) {
        row.push('wall_top');
      } else {
        row.push('floor');
      }
    }
    grid.push(row);
  }

  const objects: LevelObject[] = [];
  const enemiesList = Array.isArray(data.enemies)
    ? data.enemies
    : Array.isArray(data.enemy_positions)
    ? data.enemy_positions.map((p: any, i: number) => ({ label: `ENEMY_${i+1}`, x: p.x, y: p.y, hp: 10, max_hp: 10 }))
    : [];

  enemiesList.forEach((e: any, idx: number) => {
    objects.push({
      id: `battle_enemy_${idx + 1}`,
      type: 'enemy',
      overworld_sprite: 'desolate_landscape.desolate_kobold_01',
      battle_sprite: (e.label || 'slime').toLowerCase(),
      battle_name: e.label || `ENEMY ${idx + 1}`,
      position: { x: 3 + idx * 5, y: e.y !== undefined ? e.y : (2 + idx) },
      properties: {
        entity_id: `ENTITY_ID_${(e.label || 'SLIME').toUpperCase()}`,
        display_name: `${e.label || 'ENEMY'} (${e.hp || 10}/${e.max_hp || 10} HP)`,
        battle: 'BATTLE_DEFAULT'
      }
    });
  });

  const regions: LevelRegion[] = [
    {
      id: 'enemy_hud',
      bounds: { x: 1, y: 1, width: 18, height: 4 },
      description: 'Enemy Roster & Target Status'
    },
    {
      id: 'deck_hud',
      bounds: { x: 1, y: 6, width: 18, height: 5 },
      description: 'Hand Deck: 5 Cards (FIRE_SLASH, WATER_GUARD, HEAL, SHIELD, ENERGY)'
    },
    {
      id: 'combo_hud',
      bounds: { x: 1, y: 12, width: 18, height: 2 },
      description: 'Poker Hand Combos (Pair, Flush, Straight)'
    },
    {
      id: 'timer_hud',
      bounds: { x: 1, y: 15, width: 18, height: 2 },
      description: 'AP Turn Timer Bar'
    }
  ];

  return {
    id: data.id || 'battle',
    name: data.title || data.label || 'Battle Screen',
    width: w,
    height: h,
    tileset: tilesetId,
    music: 'MUSIC_BATTLE',
    mapId: 'SCREEN_BATTLE',
    grid,
    spawn: { x: data.hud_layout?.caret_x ?? 2, y: data.hud_layout?.timer_row ?? 15, facing: 'RIGHT' },
    exits: [],
    objects,
    regions,
    isScreen: true,
    originalScreenData: data
  };
}

export function createEmptyEditorLevel(id: string = 'new_scene', tilesetId: string = 'forest', width: number = 20, height: number = 18): EditorLevel {
  const grid: string[][] = [];
  const tileset = getTileset(tilesetId);
  const defaultFloor = tileset.tiles.find(t => t.walkable)?.id || tileset.tiles[0]?.id || 'floor';

  for (let y = 0; y < height; y++) {
    const row: string[] = [];
    for (let x = 0; x < width; x++) {
      row.push(defaultFloor);
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

export function levelDataToEditor(data: any): EditorLevel {
  if (data.logo && data.menu) {
    return titleScreenToEditor(data);
  }
  if ((data.enemies && data.hud_layout) || (data.enemy_positions && data.hud_layout)) {
    return battleScreenToEditor(data);
  }

  const w = data.map?.width || 20;
  const h = data.map?.height || 18;
  const tilesetId = data.map?.tileset || 'forest';
  const tileset = getTileset(tilesetId);
  const defaultFloor = tileset.tiles.find(t => t.walkable)?.id || tileset.tiles[0]?.id || 'floor';

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

export function editorToLevelData(lvl: EditorLevel): any {
  if (lvl.isScreen && lvl.originalScreenData) {
    const screenData = JSON.parse(JSON.stringify(lvl.originalScreenData));
    if (lvl.id === 'title') {
      const match = lvl.name.match(/^Title Screen \((.*)\)$/);
      if (match) screenData.title = match[1];
    }
    return screenData;
  }

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

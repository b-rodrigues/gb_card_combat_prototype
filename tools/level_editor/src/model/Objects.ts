export type ObjectType =
  | 'player_spawn'
  | 'npc'
  | 'enemy'
  | 'chest'
  | 'door'
  | 'warp'
  | 'trigger'
  | 'item'
  | 'signpost';

export interface LevelObject {
  id: string;
  type: ObjectType;
  position: {
    x: number;
    y: number;
  };
  properties?: Record<string, any>;
  // Sprite/tile configuration for cross-context reuse (overworld + battle)
  overworld_sprite?: string;  // tileset.tile_name for overworld rendering
  animation_frames?: string[]; // Array of tileset.tile_name frames for animation
  animation_speed?: number;   // Animation ticks per frame (default 16 or 250ms)
  battle_sprite?: string;     // tileset.tile_name for battle rendering
  battle_name?: string;       // name shown in battle UI (overrides display_name)
  sprite_width?: number;      // Width in tiles (e.g. 9 for a 9x9 boss meta-tile)
  sprite_height?: number;     // Height in tiles (e.g. 9 for a 9x9 boss meta-tile)
  is_boss?: boolean;          // Flag indicating this is a boss
  meta_tiles?: string[][];    // 2D grid of tile identifiers (e.g. 9x9 tiles)
}

export interface ObjectTemplate {
  type: ObjectType;
  label: string;
  defaultId: string;
  color: string;
  icon: string;
  defaultProps: Record<string, any>;
}

export const OBJECT_TEMPLATES: ObjectTemplate[] = [
  {
    type: 'npc',
    label: 'NPC / Villager',
    defaultId: 'npc_villager',
    color: '#3498db',
    icon: '👤',
    defaultProps: {
      display_name: 'VILLAGER',
      dialogue: 'DIALOGUE_ID_MAYOR_GREETING',
      overworld_sprite: 'interior.villager',
      battle_sprite: '',
      battle_name: 'VILLAGER'
    }
  },
  {
    type: 'enemy',
    label: 'Enemy / Monster',
    defaultId: 'enemy_slime',
    color: '#e74c3c',
    icon: '👾',
    defaultProps: {
      display_name: 'SLIME',
      battle: 'BATTLE_SLIME',
      ai: 'AI_PATROL_CROSS',
      overworld_sprite: 'combat.slime_bottom_mid',
      battle_sprite: 'combat.slime_bottom_mid',
      battle_name: 'SLIME',
      animation_frames: ['combat.slime_bottom_mid', 'combat.slime_anim_mid']
    }
  },
  {
    type: 'enemy',
    label: 'Bat Enemy (Animated)',
    defaultId: 'enemy_bat',
    color: '#9b59b6',
    icon: '🦇',
    defaultProps: {
      display_name: 'BAT',
      battle: 'BATTLE_BAT',
      ai: 'AI_PATROL_CIRCLE',
      overworld_sprite: 'combat.bat_0_body',
      battle_sprite: 'combat.bat_0_body',
      battle_name: 'CAVE BAT',
      animation_frames: ['combat.bat_0_body', 'combat.bat_1_body']
    }
  },
  {
    type: 'enemy',
    label: 'Boss Enemy (9x9 Meta-Tile)',
    defaultId: 'enemy_boss',
    color: '#8e44ad',
    icon: '👑',
    defaultProps: {
      display_name: 'LORD GIAUSAR',
      battle: 'BATTLE_BOSS',
      ai: 'AI_NONE',
      is_boss: true,
      sprite_width: 9,
      sprite_height: 9,
      overworld_sprite: 'combat.boss_head_mid',
      battle_sprite: 'combat.boss_head_mid',
      battle_name: 'LORD GIAUSAR'
    }
  },
  {
    type: 'item',
    label: 'Item Pickup',
    defaultId: 'item_pickup',
    color: '#f1c40f',
    icon: '✨',
    defaultProps: {
      display_name: 'AMULET',
      dialogue: 'DIALOGUE_ID_AMULET_NOTHING'
    }
  },
  {
    type: 'signpost',
    label: 'Signpost',
    defaultId: 'signpost_hint',
    color: '#e67e22',
    icon: '🪧',
    defaultProps: {
      display_name: 'SIGNPOST',
      dialogue: 'DIALOGUE_ID_SIGNPOST'
    }
  },
  {
    type: 'chest',
    label: 'Treasure Chest',
    defaultId: 'chest_gold',
    color: '#9b59b6',
    icon: '📦',
    defaultProps: {
      item_id: 'ITEM_HERB',
      gold: 10
    }
  },
  {
    type: 'trigger',
    label: 'Event Trigger',
    defaultId: 'trigger_event',
    color: '#1abc9c',
    icon: '⚡',
    defaultProps: {
      event_id: 'EVENT_TOWN_ARRIVAL'
    }
  }
];

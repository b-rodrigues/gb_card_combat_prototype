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
      dialogue: 'DIALOGUE_ID_MAYOR_GREETING'
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
      ai: 'AI_PATROL_CROSS'
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

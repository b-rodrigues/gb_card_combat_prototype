/** Combat-art + enemy-type disk I/O via the vite dev API.
 *  Mirrors io/serverLevels.ts: all functions throw on transport failure so
 *  callers can fall back to bundled behavior. */

/** One combat-art set (screens/combat_art/*.json).  Cells are curated tile
 *  names (tools/level_editor/public/tiles/combat/<name>.png, see
 *  tools/compose_battle_sprites.py LAYOUT) or null (blank cell).
 *  frame1 omitted repeats frame0. */
export interface CombatArtSet {
  $schema?: string;
  id: string;
  label: string;
  order: number;
  width: number;
  height: number;
  palette: number;
  frame0: Array<string | null>;
  frame1?: Array<string | null>;
}

export interface CombatArtListItem {
  id: string;
  label: string;
  order: number;
  width: number;
  height: number;
}

export interface EnemyTypeListItem {
  id: string;
  label: string;
  category: string;
  art: string | null;
  ow: boolean;
}

/** Tiles present in the composed battle sheet (assets/battle_sprites.png).
 *  MUST stay in sync with tools/compose_battle_sprites.py LAYOUT: only
 *  these names resolve to sheet cells for the ROM.  Extra PNGs in
 *  public/tiles/combat/ need a LAYOUT entry + compose + make gfx first. */
export const SHEET_TILE_NAMES: string[] = [
  'slime_top_left', 'slime_top_mid', 'slime_top_right',
  'slime_bottom_left', 'slime_bottom_mid', 'slime_bottom_right',
  'slime_anim_left', 'slime_anim_mid', 'slime_anim_right',
  'bat_0_left', 'bat_0_body', 'bat_0_right',
  'bat_1_left', 'bat_1_body', 'bat_1_right',
  'boss_horns_left', 'boss_horns_mid', 'boss_horns_right',
  'boss_head_left', 'boss_head_mid', 'boss_head_right',
  'boss_torso_left', 'boss_torso_mid', 'boss_torso_right',
  'mimic_body',
];

/** Every curated combat tile (tools/level_editor/public/tiles/combat/*.png)
 *  that the Combat Art Studio brush exposes.  A superset of
 *  SHEET_TILE_NAMES: tiles missing from the composed sheet (cards, icons,
 *  HUD, digits, statuses) are paintable but are flagged "not compiled to
 *  ROM" until a LAYOUT entry + compose + make gfx is added.  Kept in sync
 *  with the directory listing; the source tileset (assets/combat-tile.png,
 *  16x4 = 64 tiles) is described in assets/combat-tileset-description.csv. */
export const COMBAT_BRUSH_NAMES: string[] = [
  ...SHEET_TILE_NAMES,
  'arrow_up', 'battery_ap', 'card_bottom_left', 'card_bottom_right',
  'card_mid_center', 'card_mid_left', 'card_mid_right', 'card_slot_blank',
  'card_top_left', 'card_top_mid', 'card_top_right', 'combat_blank',
  'deck_cards', 'digit_1', 'digit_2', 'digit_3', 'digit_4',
  'heart_hp', 'hero', 'icon_bow', 'icon_shield', 'icon_sword',
  'status_fire', 'status_poison',
  'tile_0_0', 'tile_0_1', 'tile_0_2', 'tile_0_3', 'tile_0_4', 'tile_0_5',
  'tile_0_6', 'tile_0_7', 'tile_0_8', 'tile_0_9', 'tile_0_10', 'tile_0_11',
  'tile_0_12', 'tile_0_13', 'tile_0_14', 'tile_0_15',
  'tile_1_0', 'tile_1_1', 'tile_1_2', 'tile_1_3', 'tile_1_4', 'tile_1_5',
  'tile_1_6', 'tile_1_7', 'tile_1_8', 'tile_1_9', 'tile_1_10', 'tile_1_11',
  'tile_1_12', 'tile_1_13', 'tile_1_14', 'tile_1_15',
  'tile_2_0', 'tile_2_1', 'tile_2_2', 'tile_2_3', 'tile_2_4', 'tile_2_5',
  'tile_2_6', 'tile_2_7', 'tile_2_8', 'tile_2_9', 'tile_2_10', 'tile_2_11',
  'tile_2_12', 'tile_2_13', 'tile_2_14', 'tile_2_15',
];

export const COMBAT_TILE_URL = (name: string) => `/tiles/combat/${name}.png`;

export const MAX_ART_W = 6;
export const MAX_ART_H = 4;

export function frameCells(set: CombatArtSet, frame: 0 | 1): Array<string | null> {
  if (frame === 1 && set.frame1) return set.frame1;
  return set.frame0;
}

export async function fetchCombatArtList(): Promise<CombatArtListItem[]> {
  const res = await fetch('/api/combat-art');
  if (!res.ok) throw new Error(`combat-art list returned ${res.status}`);
  const body = await res.json();
  if (!body.success) throw new Error(body.error || 'combat-art list failed');
  return (body.items || []).sort((a: CombatArtListItem, b: CombatArtListItem) => a.order - b.order);
}

export async function fetchCombatArtSet(id: string): Promise<CombatArtSet> {
  const res = await fetch(`/api/combat-art-set?id=${encodeURIComponent(id)}`);
  if (!res.ok) throw new Error(`combat-art ${id} returned ${res.status}`);
  const body = await res.json();
  if (!body.success) throw new Error(body.error || `combat-art ${id} failed`);
  return body.data as CombatArtSet;
}

export async function saveCombatArtSet(set: CombatArtSet): Promise<void> {
  const res = await fetch('/api/save-combat-art', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ id: set.id, data: set }),
  });
  if (!res.ok) throw new Error(`save combat-art returned ${res.status}`);
  const body = await res.json();
  if (!body.success) throw new Error(body.error || 'save combat-art failed');
}

export async function fetchEnemyTypeList(): Promise<EnemyTypeListItem[]> {
  const res = await fetch('/api/enemy-types');
  if (!res.ok) throw new Error(`enemy-types list returned ${res.status}`);
  const body = await res.json();
  if (!body.success) throw new Error(body.error || 'enemy-types list failed');
  return body.items || [];
}

export async function fetchEnemyType(id: string): Promise<any> {
  const res = await fetch(`/api/enemy-type?id=${encodeURIComponent(id)}`);
  if (!res.ok) throw new Error(`enemy-type ${id} returned ${res.status}`);
  const body = await res.json();
  if (!body.success) throw new Error(body.error || `enemy-type ${id} failed`);
  return body.data;
}

export async function saveEnemyType(id: string, data: any): Promise<void> {
  const res = await fetch('/api/save-enemy-type', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ id, data }),
  });
  if (!res.ok) throw new Error(`save enemy-type returned ${res.status}`);
  const body = await res.json();
  if (!body.success) throw new Error(body.error || 'save enemy-type failed');
}

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

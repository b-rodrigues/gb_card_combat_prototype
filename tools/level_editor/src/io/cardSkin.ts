/** Battle hand-card skin disk I/O via the vite dev API (singleton record,
 *  screens/cards_skin.json).  Mirrors io/combatArt.ts: all functions throw
 *  on transport failure so callers can show an error. */

/** One battle-card-type visual (BattleCardType order: sword, shield, bow,
 *  heal, dagger).  icon/color are the JSON names battle_compile.py
 *  resolves (ICON_TILES / SKIN_COLORS). */
export interface CardTypeSkin {
  icon: string;
  color: string;
}

/** The battle hand-card skin (screens/cards_skin.json): box geometry, per
 *  type weapon icon + CGB palette, per element-status icon + palette. */
export interface CardSkin {
  $schema?: string;
  id: string;
  label: string;
  box: { w: number; h: number };
  types: Record<'sword' | 'shield' | 'bow' | 'heal' | 'dagger', CardTypeSkin>;
  elements: Record<'fire' | 'ice' | 'poison', CardTypeSkin>;
}

/** Fixed VRAM icon catalog — names are the slugified combat-tileset
 *  description entries (assets/combat-tileset-description.csv -> the
 *  public/tiles/combat slugs); dagger/ring/amulet have no CSV entry
 *  (atlas-only icons) and keep plain names.  Values resolve through
 *  battle_compile.py ICON_TILES to the fixed VRAM tiles ui_init loads
 *  (ui.h UI_TILE_CARD_* 104-112).  The atlas-only coin is not offered. */
export const CARD_ICON_NAMES: string[] = [
  'combat_sword_icon', 'combat_shield_icon', 'combat_bow_icon',
  'dagger', 'ring', 'amulet',
  'combat_fire_status', 'combat_ice_status', 'combat_poison_status',
];

/** CGB BG palette names (UI_COLOR_* 0-7). */
export const CARD_COLOR_NAMES: string[] = [
  'none', 'fire', 'iron', 'field', 'poison', 'wood', 'gold', 'dim',
];

/** Editor preview colors per UI_COLOR name (approximates the ROM CRAM). */
export const CARD_COLOR_HEX: Record<string, string> = {
  none: '#ffffff',
  fire: '#c34e1b',
  iron: '#5a6e8c',
  field: '#7bb660',
  poison: '#7136c1',
  wood: '#b08a5a',
  gold: '#e0a939',
  dim: '#9a9a9a',
};

/** Preview tile PNGs (public/tiles/combat), keyed by the slugified icon
 *  names.  Ring/dagger/amulet are atlas-only (no CSV slug), so they
 *  preview as text chips. */
export const CARD_ICON_URL: Record<string, string | null> = {
  combat_sword_icon: '/tiles/combat/combat_sword_icon.png',
  combat_shield_icon: '/tiles/combat/combat_shield_icon.png',
  combat_bow_icon: '/tiles/combat/combat_bow_icon.png',
  combat_fire_status: '/tiles/combat/combat_fire_status.png',
  combat_ice_status: '/tiles/combat/combat_ice_status.png',
  combat_poison_status: '/tiles/combat/combat_poison_status.png',
  combat_hp_icon: '/tiles/combat/combat_hp_icon.png',
  combat_ap_icon: '/tiles/combat/combat_ap_icon.png',
  combat_deck_icon: '/tiles/combat/combat_deck_icon.png',
  dagger: null,
  ring: null,
  amulet: null,
};

/** Card frame tiles (assets/card_frames.png, tools/compose_card_frames.py
 *  LAYOUT order TL TM TR / L C R / BL BM BR). */
export const CARD_FRAME_URLS: string[] = [
  '/tiles/combat/combat_top_left_card_corner.png',
  '/tiles/combat/combat_top_middle_card.png',
  '/tiles/combat/combat_top_right_card_corner.png',
  '/tiles/combat/combat_left_card_side.png',
  '/tiles/combat/combat_center_card.png',
  '/tiles/combat/combat_right_card_side.png',
  '/tiles/combat/combat_bottom_left_card_corner.png',
  '/tiles/combat/combat_bottom_middle_card.png',
  '/tiles/combat/combat_bottom_right_card_corner.png',
];

export async function fetchCardSkin(): Promise<CardSkin> {
  const res = await fetch('/api/card-skin');
  if (!res.ok) throw new Error(`card-skin returned ${res.status}`);
  const body = await res.json();
  if (!body.success) throw new Error(body.error || 'card-skin failed');
  return body.data as CardSkin;
}

export async function saveCardSkin(skin: CardSkin): Promise<void> {
  const res = await fetch('/api/save-card-skin', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ data: skin }),
  });
  if (!res.ok) throw new Error(`save card-skin returned ${res.status}`);
  const body = await res.json();
  if (!body.success) throw new Error(body.error || 'save card-skin failed');
}

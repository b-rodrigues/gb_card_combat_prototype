"""Compose assets/card_frames.png (battle hand-card frame sheet).

Reads the CSV-slug editor PNGs (tools/level_editor/public/tiles/combat/)
and lays out 3 cols x 8 rows of 8x8 cells in VRAM-load order:

  row 0: card top border   (top_left_corner, top_middle, top_right_corner)
  row 1: card middle band  (left_side, center, right_side)
  row 2: card bottom border (bottom_left_corner, bottom_middle, bottom_right_corner)

png2gb converts the sheet to src/gfx/card_frame_tiles.h; the ROM loads all
24 tiles (9 frame tiles at UI_TILE_CARD_FRAME_BASE 118, bar segments,
HUD icons, select arrow, status icons, weapon icons -- VRAM block 1, battle enemy art
starts at 128) and the bank-3 renderer stamps 3x4 card boxes from them.

Deterministic: rerunning reproduces the sheet byte-identically.
"""
from PIL import Image

PUB = 'tools/level_editor/public/tiles/combat'
LAYOUT = [
    ['combat_top_left_card_corner', 'combat_top_middle_card', 'combat_top_right_card_corner'],
    ['combat_left_card_side', 'combat_center_card', 'combat_right_card_side'],
    ['combat_bottom_left_card_corner', 'combat_bottom_middle_card', 'combat_bottom_right_card_corner'],
    # Turn-timer bar segments (HUD skin): filled / empty, then the HUD
    # icons from the combat tileset (combat-tileset-description.csv):
    # hp / ap / deck, the up-arrow select icon (replaces the '^'
    # caret on the battle marker/target rows), and the fire / ice /
    # poison status tiles (element riders; overwrite the atlas art the
    # atlas loop loads at 110/111/112).  VRAM: frames at
    # UI_TILE_CARD_FRAME_BASE (118-126), filled at UI_TIMER_FILLED (117),
    # empty at 127, HUD icons overwrite the atlas data at 113/114/116,
    # arrow at UI_TILE_SELECT_ARROW (96), status at 110/111/112.
    ['combat_timer_bar_filled', 'combat_timer_bar_empty', 'combat_hp_icon'],
    ['combat_ap_icon', 'combat_deck_icon', 'combat_arrow_pointing_up'],
    ['combat_fire_status', 'combat_ice_status', 'combat_poison_status'],
    # Card weapon icons (skin weapon_tile VRAM ids 104-108): sword,
    # shield, bow, dagger, ring.  The 6th cell stays blank (None); the
    # loader maps it to the unused VRAM 97 scratch tile.
    ['combat_sword_icon', 'combat_shield_icon', 'combat_bow_icon'],
    ['combat_dagger_icon', 'combat_ring_icon', None],
    # Limited-use arrow counters (bow): remaining-uses glyphs shown on
    # the card's digit row (uses 4/3/2/1) plus the depleted marker.
    # VRAM 97 was the spare scratch slot; the arrow states take 97-101
    # (free BG fetch slots — the atlas only writes 104-116 and nothing
    # ever maps ids 97-101 into a tilemap).
    ['combat_4_arrows_left', 'combat_3_arrows_left', 'combat_2_arrows_left'],
    ['combat_1_arrow_left', 'combat_zero_icon', None],
]


def main():
    rows = len(LAYOUT)
    sheet = Image.new('RGB', (24, rows * 8), (255, 255, 255))
    for y, row in enumerate(LAYOUT):
        for x, name in enumerate(row):
            if name is None:
                continue
            im = Image.open('%s/%s.png' % (PUB, name)).convert('RGB')
            sheet.paste(im.resize((8, 8), Image.NEAREST), (x * 8, y * 8))
    sheet.save('assets/card_frames.png')
    print('wrote assets/card_frames.png')


if __name__ == '__main__':
    main()

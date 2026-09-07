"""Compose assets/card_frames.png (battle hand-card frame sheet).

Reads the CSV-slug editor PNGs (tools/level_editor/public/tiles/combat/)
and lays out 3 cols x 3 rows of 8x8 cells in VRAM-load order:

  row 0: card top border   (top_left_corner, top_middle, top_right_corner)
  row 1: card middle band  (left_side, center, right_side)
  row 2: card bottom border (bottom_left_corner, bottom_middle, bottom_right_corner)

png2gb converts the sheet to src/gfx/card_frame_tiles.h; the ROM loads the
9 tiles at UI_TILE_CARD_FRAME_BASE (118, VRAM block 1 -- battle enemy art
starts at 128) and the bank-3 renderer stamps 3x4 card boxes from them.

Deterministic: rerunning reproduces the sheet byte-identically.
"""
from PIL import Image

PUB = 'tools/level_editor/public/tiles/combat'
LAYOUT = [
    ['combat_top_left_card_corner', 'combat_top_middle_card', 'combat_top_right_card_corner'],
    ['combat_left_card_side', 'combat_center_card', 'combat_right_card_side'],
    ['combat_bottom_left_card_corner', 'combat_bottom_middle_card', 'combat_bottom_right_card_corner'],
    # Turn-timer bar segments (HUD skin): filled / empty; the blank pad cell
    # keeps the 3-column sheet layout.  VRAM: frames at UI_TILE_CARD_FRAME_BASE
    # (118-126), filled at UI_TILE_TIMER_FILLED (117), empty at 127.
    ['combat_timer_bar_filled', 'combat_timer_bar_empty', None],
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

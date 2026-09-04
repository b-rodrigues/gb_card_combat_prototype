"""Compose assets/battle_sprites.png (canonical 3x2 battle-art source sheet).

Reads the curated 32x32 editor PNGs (4x pixel art), downscales NEAREST to
8x8, and lays out 3 cols x 8 rows of cells:

  row 0: slime_top_*      row 4: bat_1_*
  row 1: slime_bottom_*   row 5: boss_horns_*
  row 2: slime_anim_*     row 6: boss_head_*
  row 3: bat_0_*          row 7: blank

Deterministic: rerunning reproduces the sheet byte-identically.
"""
import sys
from PIL import Image

PUB = 'tools/level_editor/public/tiles/combat'
LAYOUT = [
    ['slime_top_left', 'slime_top_mid', 'slime_top_right'],
    ['slime_bottom_left', 'slime_bottom_mid', 'slime_bottom_right'],
    ['slime_anim_left', 'slime_anim_mid', 'slime_anim_right'],
    ['bat_0_left', 'bat_0_body', 'bat_0_right'],
    ['bat_1_left', 'bat_1_body', 'bat_1_right'],
    ['boss_horns_left', 'boss_horns_mid', 'boss_horns_right'],
    ['boss_head_left', 'boss_head_mid', 'boss_head_right'],
    [None, None, None],
]

sheet = Image.new('RGB', (24, 64), (255, 255, 255))
for y, row in enumerate(LAYOUT):
    for x, name in enumerate(row):
        if name is None:
            continue
        im = Image.open('%s/%s.png' % (PUB, name)).convert('RGB')
        sheet.paste(im.resize((8, 8), Image.NEAREST), (x * 8, y * 8))
sheet.save('assets/battle_sprites.png')
print('wrote assets/battle_sprites.png')

"""Compose assets/hero_sprites.png (canonical shared overworld hero sheet).

Reads the curated 8x8 hero PNGs (transparent background) and lays out
1 row of cells: hero_f0, hero_f1.

Deterministic: rerunning reproduces the sheet byte-identically.
"""
from PIL import Image

PUB = 'tools/level_editor/public/tiles/hero'
LAYOUT = [
    ['hero_f0', 'hero_f1'],
]

# Tile-name -> sheet (x, y): single source of truth for hero cell addressing.
# battle_compile.py imports this (no side effects) to resolve hero.json cells.
TILE_COORDS = {}
for _y, _row in enumerate(LAYOUT):
    for _x, _name in enumerate(_row):
        if _name is not None:
            TILE_COORDS[_name] = (_x, _y)


def main():
    sheet = Image.new('RGBA', (16, 8), (0, 0, 0, 0))
    for y, row in enumerate(LAYOUT):
        for x, name in enumerate(row):
            if name is None:
                continue
            im = Image.open('%s/%s.png' % (PUB, name)).convert('RGBA')
            sheet.paste(im, (x * 8, y * 8))
    sheet.save('assets/hero_sprites.png')
    print('wrote assets/hero_sprites.png')


if __name__ == '__main__':
    main()
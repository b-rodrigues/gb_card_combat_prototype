#!/usr/bin/env python3
"""Extract individual 8x8 tiles from source PNGs for the web editor.

Reads tile coordinates from asset_atlas.py (TILESETS + FOREST_COORDS),
extracts each 8x8 tile, upscales 4x via nearest-neighbor (32x32),
and saves to tools/level_editor/public/tiles/{tileset}/{tile_id}.png

This ensures the web editor shows the EXACT same tiles the Game Boy renders.
"""

import sys
import os
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))
from tools.asset_atlas import TILESETS, FOREST_COORDS

try:
    from PIL import Image
except ImportError:
    sys.exit("extract_tiles.py: requires Pillow (`pip install Pillow`)")

TILE_SIZE = 8
UPSCALE = 4  # 32x32 output
OUTPUT_DIR = Path(__file__).resolve().parent / "public" / "tiles"

TILESET_ID_MAP = {
    "RPG_exterior.png": {
        **{k.lower(): "exterior" for k in TILESETS["RPG_exterior.png"].keys()},
        **{k.lower(): "forest" for k in FOREST_COORDS.keys()},
    },
    "RPG_interior.png": {
        **{k.lower(): "interior" for k in TILESETS["RPG_interior.png"].keys()},
    },
    "DungeonTileset.png": {
        **{k.lower(): "dungeon" for k in TILESETS["DungeonTileset.png"].keys()},
    },
}

COORDS_MAP = {
    "RPG_exterior.png": {
        **{k.lower(): v for k, v in TILESETS["RPG_exterior.png"].items()},
        **{k.lower(): v for k, v in FOREST_COORDS.items()},
    },
    "RPG_interior.png": {
        **{k.lower(): v for k, v in TILESETS["RPG_interior.png"].items()},
    },
    "DungeonTileset.png": {
        **{k.lower(): v for k, v in TILESETS["DungeonTileset.png"].items()},
    },
}


def extract_tile(png_path, col, row):
    """Extract a single 8x8 tile from the PNG at (col, row)."""
    img = Image.open(png_path).convert("RGBA")
    ox, oy = col * TILE_SIZE, row * TILE_SIZE
    tile = img.crop((ox, oy, ox + TILE_SIZE, oy + TILE_SIZE))
    return tile


def upscale_nearest(tile, factor=UPSCALE):
    """Upscale tile using nearest-neighbor (crisp pixel art)."""
    return tile.resize(
        (TILE_SIZE * factor, TILE_SIZE * factor),
        Image.NEAREST
    )


def main():
    assets_dir = Path(__file__).resolve().parent.parent.parent / "assets"

    for png_name in ["RPG_exterior.png", "RPG_interior.png", "DungeonTileset.png"]:
        png_path = assets_dir / png_name
        if not png_path.exists():
            print(f"Missing source: {png_path}", file=sys.stderr)
            sys.exit(1)

        coords = COORDS_MAP[png_name]
        tileset_map = TILESET_ID_MAP[png_name]

        for tile_name, (col, row) in coords.items():
            tileset_id = tileset_map[tile_name.lower()]

            tile = extract_tile(png_path, col, row)
            upscaled = upscale_nearest(tile)

            out_dir = OUTPUT_DIR / tileset_id
            out_dir.mkdir(parents=True, exist_ok=True)

            # Convert semantic name to tile_id (e.g., FOREST_TREE -> tree)
            tile_id = tile_name.lower().replace("forest_", "").replace("exterior_", "").replace("interior_", "")
            out_path = out_dir / f"{tile_id}.png"

            upscaled.save(out_path)
            print(f"  {tileset_id}/{tile_id}.png <- {png_name} ({col},{row})")

    print(f"\nDone! Extracted tiles to {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
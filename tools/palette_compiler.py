#!/usr/bin/env python3
"""palette_compiler.py -- Match tileset tiles to fixed CGB palettes.

Single source of truth for CGB background palette assignments. Consumes
tileset JSON (tools/level_editor/tilesets/*.json) and PNG assets, outputs
JSON manifests consumed by both the web editor (WYSIWYG canvas) and the ROM
compiler (generate_tiles.py -> tile_palette.h).

Unlike the previous k-means approach, this version matches tiles to the
FIXED hand-tuned palettes defined in src/game/tiles_content.c
(cgb_bg_palettes_forest, cgb_bg_palettes_desolate, cgb_bg_palettes_castle).
This ensures the tile_palette.h indices correspond to the actual ROM palettes.

Algorithm:
1. Load PNG + tileset JSON (tiles with vram_block positions)
2. Extract average/characteristic color for each 8x8 tile
3. For each tileset, use the fixed 8-palette definition (from tiles_content.c)
4. Match each tile to the best-fitting fixed palette (by color distance)
5. Output manifest: generated/tiles/<tileset>.json

Manifest format:
{
  "tileset": "forest",
  "anchor_color": "#7bb660",
  "palettes": [
    ["#ffffff", "#aaaaaa", "#555555", "#000000"],  // palette 0: gray
    ["#ffffe0", "#ff8c28", "#dc3214", "#640a00"],  // palette 1: fire
    ...
  ],
  "tile_palettes": [0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 5, 5, 3, 3, ...]
}

Each entry in palettes is [color0, color1, color2, color3] matching the
fixed ROM palette. tile_palettes maps sheet index (0..N-1) to fixed
palette index (0..7).
"""

import sys
import json
import argparse
from pathlib import Path
from typing import List, Tuple, Dict, Any, Optional
from PIL import Image
import math

REPO_ROOT = Path(__file__).resolve().parent.parent
TILESETS_DIR = REPO_ROOT / "tools" / "level_editor" / "tilesets"
ASSETS_DIR = REPO_ROOT / "assets"
GENERATED_DIR = REPO_ROOT / "generated" / "tiles"

TILE_SIZE = 8

# Tilesets to process
TILESETS = ["forest", "castle", "desolate_landscape"]

# PNG mapping
PNG_MAP = {
    "forest": "forest-tile.png",
    "castle": "castle-tile.png",
    "desolate_landscape": "desolate_landscape.png",
}

# Fixed CGB palettes from src/game/tiles_content.c (RGB8 format -> 0-255)
# Each palette: 4 colors, each color is (R, G, B)
FIXED_PALETTES = {
    "forest": [
        # Palette 0: gray
        [(255, 255, 255), (170, 170, 170), (85, 85, 85), (0, 0, 0)],
        # Palette 1: fire
        [(255, 255, 224), (255, 140, 40), (220, 50, 20), (100, 10, 0)],
        # Palette 2: iron/ice
        [(235, 242, 250), (140, 180, 214), (70, 105, 138), (27, 43, 58)],
        # Palette 3: field (greens) - UI_COLOR_FIELD
        [(120, 176, 96), (40, 72, 24), (24, 56, 8), (0, 0, 0)],
        # Palette 4: poison
        [(240, 255, 240), (100, 220, 100), (30, 140, 50), (10, 50, 20)],
        # Palette 5: wood (browns, harmonized Color 0 = grass green) - UI_COLOR_WOOD
        [(120, 176, 96), (196, 138, 72), (138, 82, 34), (61, 32, 10)],
        # Palette 6: gold
        [(255, 252, 224), (255, 215, 0), (200, 140, 8), (90, 58, 0)],
        # Palette 7: dim
        [(200, 200, 200), (150, 150, 150), (90, 90, 90), (40, 40, 40)],
    ],
    "desolate_landscape": [
        # Palette 0: gray
        [(255, 255, 255), (170, 170, 170), (85, 85, 85), (0, 0, 0)],
        # Palette 1: campfire (fire) - harmonized Color 0 = slate rock
        [(147, 141, 161), (237, 194, 20), (215, 80, 20), (80, 10, 0)],
        # Palette 2: iron/ice - harmonized Color 0 = slate rock
        [(147, 141, 161), (140, 180, 214), (70, 105, 138), (27, 43, 58)],
        # Palette 3: flora (purples) - harmonized Color 0 = slate rock
        [(147, 141, 161), (116, 111, 128), (63, 58, 74), (38, 35, 46)],
        # Palette 4: poison - harmonized Color 0 = slate rock
        [(147, 141, 161), (100, 220, 100), (30, 140, 50), (10, 50, 20)],
        # Palette 5: deadwood (browns) - harmonized Color 0 = slate rock
        [(147, 141, 161), (141, 117, 74), (111, 90, 52), (38, 35, 46)],
        # Palette 6: gold - harmonized Color 0 = slate rock
        [(147, 141, 161), (215, 167, 38), (141, 117, 74), (50, 30, 10)],
        # Palette 7: slate rock - harmonized Color 0 = slate rock
        [(147, 141, 161), (131, 123, 150), (63, 58, 74), (38, 35, 46)],
    ],
    "castle": [
        # Palette 0: stone (light gray)
        [(215, 215, 215), (179, 176, 176), (130, 130, 130), (46, 46, 46)],
        # Palette 1: curtain (red)
        [(215, 215, 215), (139, 27, 27), (98, 18, 18), (30, 0, 0)],
        # Palette 2: iron (blues)
        [(215, 215, 215), (140, 160, 180), (70, 90, 110), (30, 40, 50)],
        # Palette 3: moss/green
        [(215, 215, 215), (90, 140, 80), (40, 80, 30), (10, 30, 10)],
        # Palette 4: poison
        [(215, 215, 215), (120, 200, 120), (40, 120, 50), (10, 50, 20)],
        # Palette 5: wood furniture (browns)
        [(215, 215, 215), (158, 142, 113), (111, 90, 52), (40, 25, 10)],
        # Palette 6: gold
        [(215, 215, 215), (215, 167, 38), (162, 146, 113), (60, 40, 10)],
        # Palette 7: dim shadow
        [(215, 215, 215), (130, 130, 130), (86, 86, 86), (35, 35, 35)],
    ],
}

# Scene anchor colors (Color 0 of outdoor palettes)
ANCHOR_COLORS = {
    "forest": "#7bb660",           # grass green (RGB: 120, 176, 96)
    "desolate_landscape": "#938da1",  # slate rock (RGB: 147, 141, 161)
    "castle": "#d7d7d7",           # light stone (RGB: 215, 215, 215)
}

# Palette names for documentation in manifest
PALETTE_NAMES = {
    "forest": [
        "gray", "fire", "iron_ice", "field", "poison", "wood", "gold", "dim"
    ],
    "desolate_landscape": [
        "gray", "campfire", "iron_ice", "flora", "poison", "deadwood", "gold", "slate_rock"
    ],
    "castle": [
        "stone", "curtain", "iron", "moss_green", "poison", "wood_furn", "gold", "dim_shadow"
    ],
}


def rgb_to_hex(rgb: Tuple[int, int, int]) -> str:
    """Convert (R, G, B) to '#RRGGBB'."""
    return f"#{rgb[0]:02x}{rgb[1]:02x}{rgb[2]:02x}"


def color_distance(c1: Tuple[int, int, int], c2: Tuple[int, int, int]) -> float:
    """Euclidean distance in RGB space."""
    return math.sqrt(sum((a - b) ** 2 for a, b in zip(c1, c2)))


def load_tileset_json(tileset_id: str) -> Dict[str, Any]:
    """Load tileset JSON from tools/level_editor/tilesets/."""
    path = TILESETS_DIR / f"{tileset_id}.json"
    if not path.exists():
        raise FileNotFoundError(f"Tileset JSON not found: {path}")
    return json.loads(path.read_text())


def load_png(tileset_id: str) -> Image.Image:
    """Load PNG from assets/."""
    png_name = PNG_MAP[tileset_id]
    path = ASSETS_DIR / png_name
    if not path.exists():
        raise FileNotFoundError(f"PNG not found: {path}")
    img = Image.open(path).convert("RGB")
    return img


def get_tile_unique_colors(img: Image.Image, tx: int, ty: int) -> List[Tuple[int, int, int]]:
    """Get all unique RGB colors in a tile."""
    px = img.load()
    ox, oy = tx * TILE_SIZE, ty * TILE_SIZE
    unique = {px[ox + x, oy + y] for y in range(TILE_SIZE) for x in range(TILE_SIZE)}
    return list(unique)


def extract_tile_colors(img: Image.Image, tiles_x: int, tiles_y: int) -> List[List[Tuple[int, int, int]]]:
    """Extract unique colors for each tile in sheet order."""
    tile_colors = []
    for ty in range(tiles_y):
        for tx in range(tiles_x):
            tile_colors.append(get_tile_unique_colors(img, tx, ty))
    return tile_colors


def get_sheet_order_from_vram_block(tileset_json: Dict[str, Any]) -> List[str]:
    """Extract tile IDs in sheet order from vram_block.tiles[]."""
    vram_block = tileset_json.get("vram_block", {})
    tiles = vram_block.get("tiles", [])
    sorted_tiles = sorted(tiles, key=lambda t: t.get("index", 0))
    return [t["tile"] for t in sorted_tiles if "tile" in t]


def is_brown(rgb: Tuple[int, int, int]) -> bool:
    """Check if a color is brown-ish (wood/bark tones)."""
    r, g, b = rgb
    # Brown: R > G > B, with moderate saturation
    # Typical brown range: R 60-200, G 40-140, B 10-80
    return (r > g > b and
            r > 60 and g > 30 and b < 100 and
            (r - g) > 10 and (g - b) > 10)


def is_green(rgb: Tuple[int, int, int]) -> bool:
    """Check if a color is green-ish (foliage/grass)."""
    r, g, b = rgb
    return g > r and g > b and g > 40


DEFAULT_FLOOR_PALETTES = {
    "forest": 3,              # field (greens) - UI_COLOR_FIELD
    "desolate_landscape": 7,  # slate rock - UI_COLOR_DIM
    "castle": 0,              # stone (light gray) - UI_COLOR_NONE
}


def match_tile_to_palette(
    tile_colors: List[Tuple[int, int, int]],
    palettes: List[List[Tuple[int, int, int]]],
    anchor_rgb: Tuple[int, int, int],
    tileset_id: str = ""
) -> int:
    """
    Match a tile's color set to the best fixed palette.

    For each unique color in the tile, find the closest color in each fixed
    palette (including anchor at index 0). The palette with the lowest total
    distance wins.

    Special case for forest/desolate: tiles with both green (anchor-like)
    and brown colors should prefer the wood palette (index 5) which has
    harmonized Color 0 = anchor + brown foreground colors.
    """
    floor_palette_idx = DEFAULT_FLOOR_PALETTES.get(tileset_id, 0)

    # If tile only contains the scene's anchor backdrop color, assign floor palette directly
    if tile_colors and all(color_distance(c, anchor_rgb) < 5.0 for c in tile_colors):
        return floor_palette_idx

    # Detect if tile has both green and brown colors
    has_green = any(is_green(c) for c in tile_colors)
    has_brown = any(is_brown(c) for c in tile_colors)

    # For forest/desolate, wood palette is index 5
    wood_palette_idx = 5 if tileset_id in ("forest", "desolate_landscape") else -1

    best_palette = 0
    best_total_dist = float('inf')

    for pal_idx, palette in enumerate(palettes):
        total_dist = 0.0
        for tile_color in tile_colors:
            # Find closest color in this palette
            min_dist = min(color_distance(tile_color, pal_color) for pal_color in palette)
            total_dist += min_dist

        # Normalize by number of tile colors
        avg_dist = total_dist / len(tile_colors) if tile_colors else float('inf')

        # Boost wood palette for mixed green/brown tiles (harmonized Color 0 case)
        if has_green and has_brown and pal_idx == wood_palette_idx:
            avg_dist *= 0.5  # Strong preference for wood palette

        # Prefer floor palette when distances tie
        if avg_dist < best_total_dist or (abs(avg_dist - best_total_dist) < 1e-4 and pal_idx == floor_palette_idx):
            best_total_dist = avg_dist
            best_palette = pal_idx

    return best_palette


def generate_manifest(
    tileset_id: str,
    anchor_rgb: Tuple[int, int, int],
    palettes: List[List[Tuple[int, int, int]]],
    tile_palettes: List[int],
    palette_names: List[str]
) -> Dict[str, Any]:
    """Generate the JSON manifest structure."""
    return {
        "tileset": tileset_id,
        "anchor_color": rgb_to_hex(anchor_rgb),
        "palettes": [
            {
                "index": i,
                "name": palette_names[i] if i < len(palette_names) else f"palette_{i}",
                "colors": [rgb_to_hex(c) for c in pal]
            }
            for i, pal in enumerate(palettes)
        ],
        "tile_palettes": tile_palettes,
    }


def write_manifest(tileset_id: str, manifest: Dict[str, Any]) -> Path:
    """Write manifest to generated/tiles/<tileset>.json."""
    GENERATED_DIR.mkdir(parents=True, exist_ok=True)
    out_path = GENERATED_DIR / f"{tileset_id}.json"
    out_path.write_text(json.dumps(manifest, indent=2))
    return out_path


# Per-tileset curated overrides (e.g. animated fire frames)
TILE_PALETTE_OVERRIDES = {
    "desolate_landscape": {
        37: 1,  # Campfire frame 1 (fire)
        38: 1,  # Campfire frame 2 (fire)
    },
}


def process_tileset(tileset_id: str) -> Dict[str, Any]:
    """Process a single tileset: load JSON+PNG, match to fixed palettes, output manifest."""
    print(f"Processing {tileset_id}...")

    # Load inputs
    tileset_json = load_tileset_json(tileset_id)
    img = load_png(tileset_id)

    # Get sheet dimensions from vram_block
    vram_block = tileset_json.get("vram_block", {})
    tiles = vram_block.get("tiles", [])
    if not tiles:
        raise ValueError(f"No vram_block.tiles in {tileset_id}.json")

    max_x = max(t.get("x", 0) for t in tiles)
    max_y = max(t.get("y", 0) for t in tiles)
    tiles_x = max_x + 1
    tiles_y = max_y + 1

    print(f"  Sheet: {tiles_x}x{tiles_y} = {tiles_x * tiles_y} tiles")

    # Extract unique colors per tile
    tile_colors_list = extract_tile_colors(img, tiles_x, tiles_y)
    print(f"  Extracted colors from {len(tile_colors_list)} tiles")

    # Get fixed palettes for this tileset
    palettes = FIXED_PALETTES[tileset_id]
    anchor_hex = ANCHOR_COLORS[tileset_id]
    anchor_rgb = tuple(int(anchor_hex.lstrip('#')[i:i+2], 16) for i in (0, 2, 4))
    print(f"  Anchor color: {anchor_hex} -> RGB{anchor_rgb}")

    overrides = TILE_PALETTE_OVERRIDES.get(tileset_id, {})

    # Match each tile to best fixed palette
    tile_palettes = []
    for i, tile_colors in enumerate(tile_colors_list):
        if i in overrides:
            pal_idx = overrides[i]
        else:
            pal_idx = match_tile_to_palette(tile_colors, palettes, anchor_rgb, tileset_id)
        tile_palettes.append(pal_idx)

    print(f"  Tile palette assignments: {tile_palettes}")

    # Generate manifest
    manifest = generate_manifest(
        tileset_id, anchor_rgb, palettes, tile_palettes, PALETTE_NAMES[tileset_id]
    )

    # Write output
    out_path = write_manifest(tileset_id, manifest)
    print(f"  Wrote manifest: {out_path}")

    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--tilesets", nargs="+", default=TILESETS,
                        help="Tileset IDs to process (default: all)")
    parser.add_argument("--out-dir", type=Path, default=GENERATED_DIR,
                        help="Output directory for manifests")
    args = parser.parse_args()

    for ts_id in args.tilesets:
        if ts_id not in FIXED_PALETTES:
            print(f"Unknown tileset: {ts_id} (no fixed palettes defined)", file=sys.stderr)
            sys.exit(1)
        try:
            process_tileset(ts_id)
        except Exception as e:
            print(f"Error processing {ts_id}: {e}", file=sys.stderr)
            import traceback
            traceback.print_exc()
            sys.exit(1)

    print("Done.")


if __name__ == "__main__":
    main()
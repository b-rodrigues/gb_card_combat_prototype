#!/usr/bin/env python3
"""Import a tileset from a source PNG sheet + CSV description file.

Reads a 128x24 (16 cols x 3 rows) PNG of 8x8 tiles and a matching CSV
with one label per cell.  Produces:
  - individual tile PNGs in --output-dir
  - a tileset JSON at --output-json

Usage (inside nix develop for Pillow):
    python3 tools/level_editor/import_tileset.py \
      --sheet assets/forest-tile.png \
      --csv assets/forest-tileset-description.csv \
      --tileset-id forest \
      --label "Whispering Forest" \
      --gb-tileset-kind WORLD_TILESET_FOREST \
      --output-dir tools/level_editor/public/tiles/forest \
      --output-json tools/level_editor/tilesets/forest.json
"""
import argparse
import csv
import io
import json
import os
import re
import sys
from collections import Counter

from PIL import Image

TILE_SIZE = 8


def slugify(text):
    """Convert description text to a lowercase underscored slug."""
    text = text.strip().lower()
    text = re.sub(r"[^a-z0-9]+", "_", text)
    text = text.strip("_")
    return text


def infer_walkable(description):
    # Only genuine terrain counts as walkable.  Sprite-sheet art that shares
    # the bank (hero/npc/merchant/enemy frames) must never be walkable: the
    # ROM renders those tiles as solid and blocks movement onto them, so a
    # walkable:true flag here would make host collision disagree with the
    # game (see world_is_walkable).
    d = description.lower()
    if "walkable" in d or "floor" in d or "exit" in d:
        return True
    return False


def infer_category(description):
    d = description.lower()
    if "wall" in d:
        return "wall"
    if "tree" in d or "stump" in d or "rock" in d or "treetop" in d or "treetrunk" in d:
        return "nature"
    if "enemy" in d or "kobold" in d or "bats" in d:
        return "enemy"
    if "hero" in d or "merchant" in d:
        return "npc"
    if "fire" in d:
        return "object"
    if "exit" in d:
        return "terrain"
    if "floor" in d:
        return "terrain"
    return "terrain"


def ascii_char(walkable):
    return "." if walkable else "#"


def main():
    parser = argparse.ArgumentParser(description="Import tileset from PNG + CSV")
    parser.add_argument("--sheet", required=True, help="Source PNG (128x24)")
    parser.add_argument("--csv", required=True, help="Description CSV")
    parser.add_argument("--tileset-id", required=True, help="Tileset id (e.g. forest)")
    parser.add_argument("--label", required=True, help="Human label (e.g. Whispering Forest)")
    parser.add_argument("--gb-tileset-kind", required=True, help="GB constant (e.g. WORLD_TILESET_FOREST)")
    parser.add_argument("--output-dir", required=True, help="Directory for tile PNGs")
    parser.add_argument("--output-json", required=True, help="Output tileset JSON path")
    args = parser.parse_args()

    img = Image.open(args.sheet).convert("RGBA")
    w, h = img.size
    cols = w // TILE_SIZE
    rows = h // TILE_SIZE
    print(f"Sheet: {w}x{h} = {cols}x{rows} tiles")

    with open(args.csv, newline="") as f:
        reader = csv.reader(io.StringIO(f.read()))
        csv_rows = list(reader)

    assert len(csv_rows) == rows, f"Expected {rows} CSV rows, got {len(csv_rows)}"
    for i, row in enumerate(csv_rows):
        assert len(row) == cols, f"Row {i}: expected {cols} cols, got {len(row)}"

    os.makedirs(args.output_dir, exist_ok=True)

    # Build tile list: (tile_id, description, col, row)
    raw_tiles = []
    for r in range(rows):
        for c in range(cols):
            desc = csv_rows[r][c].strip()
            slug = slugify(desc)
            raw_tiles.append((slug, desc, c, r))

    # Deduplicate: count how many times each slug appears
    slug_counts = Counter(t[0] for t in raw_tiles)
    slug_seen = Counter()

    tiles = []
    for slug, desc, c, r in raw_tiles:
        if slug_counts[slug] > 1:
            slug_seen[slug] += 1
            tile_id = f"{args.tileset_id}_{slug}_{slug_seen[slug]}"
        else:
            tile_id = f"{args.tileset_id}_{slug}"

        # Extract 8x8 tile from sheet
        left = c * TILE_SIZE
        upper = r * TILE_SIZE
        tile_img = img.crop((left, upper, left + TILE_SIZE, upper + TILE_SIZE))

        # Determine dominant color for the fallback swatch
        pixels = list(tile_img.getdata())
        opaque = [p for p in pixels if p[3] > 0]
        if opaque:
            r_avg = sum(p[0] for p in opaque) // len(opaque)
            g_avg = sum(p[1] for p in opaque) // len(opaque)
            b_avg = sum(p[2] for p in opaque) // len(opaque)
            color = f"#{r_avg:02x}{g_avg:02x}{b_avg:02x}"
        else:
            color = "#000000"

        png_name = f"{tile_id}.png"
        tile_img.save(os.path.join(args.output_dir, png_name))

        walkable = infer_walkable(desc)
        category = infer_category(desc)

        gb_const = f"TILE_{args.tileset_id.upper()}_{slug.upper()}"

        tiles.append({
            "id": tile_id,
            "label": desc.strip(),
            "gb_constant": gb_const,
            "walkable": walkable,
            "color": color,
            "ascii": ascii_char(walkable),
            "image_url": f"/tiles/{args.tileset_id}/{png_name}",
            "category": category,
        })

    tileset = {
        "id": args.tileset_id,
        "label": args.label,
        "gb_tileset_kind": args.gb_tileset_kind,
        "tiles": tiles,
    }

    with open(args.output_json, "w") as f:
        json.dump(tileset, f, indent=2)

    print(f"Wrote {len(tiles)} tiles to {args.output_dir}/")
    print(f"Wrote {args.output_json}")

    # Print summary of ID collisions resolved
    if any(v > 1 for v in slug_counts.values()):
        print("\nDeduplicated slugs:")
        for slug, count in slug_counts.items():
            if count > 1:
                print(f"  '{slug}' x{count}")


if __name__ == "__main__":
    main()

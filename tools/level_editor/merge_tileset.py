#!/usr/bin/env python3
"""Surgically merge an updated sheet + CSV descriptions into a curated
tileset manifest.

A raw import_tileset.py run would destroy hand curation (numeric
TILE_DESOLATE_LANDSCAPE_* constants wired to world.h, shared forest
constants, curated walkable flags). This instead recomputes every cell
and applies changes ONLY at indices whose CSV description differs from
the manifest label, preserving every curated field elsewhere:

- desolate_landscape: gb_constant stays index-derived
  (TILE_DESOLATE_LANDSCAPE_<nn>) so the world.h/VRAM 1:1 mapping holds.
- other tilesets: gb_constant follows generator naming for new cells.
- walkable/ascii/category follow the current infer_* rules, so manifest
  and importer can never silently disagree again.
- tile PNGs are (re-)extracted only for changed cells.

Usage (inside nix develop for Pillow):
    python3 tools/level_editor/merge_tileset.py \
      --sheet assets/forest-tile.png \
      --csv assets/forest-tileset-description.csv \
      --tileset-json tools/level_editor/tilesets/forest.json \
      --png-dir tools/level_editor/public/tiles/forest

Exits nonzero on dimension mismatch; prints the full cell diff.
"""
import argparse
import csv
import io
import json
import os
import sys

from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from import_tileset import slugify, infer_walkable, infer_category, ascii_char

TILE_SIZE = 8


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--sheet", required=True)
    parser.add_argument("--csv", required=True)
    parser.add_argument("--tileset-json", required=True)
    parser.add_argument("--png-dir", required=True)
    args = parser.parse_args()

    img = Image.open(args.sheet).convert("RGBA")
    cols = img.size[0] // TILE_SIZE
    rows = img.size[1] // TILE_SIZE

    with open(args.csv, newline="") as f:
        csv_rows = list(csv.reader(io.StringIO(f.read())))
    assert len(csv_rows) == rows, f"CSV rows {len(csv_rows)} != sheet rows {rows}"
    for i, row in enumerate(csv_rows):
        assert len(row) == cols, f"CSV row {i} cols {len(row)} != {cols}"

    with open(args.tileset_json, encoding="utf-8") as f:
        manifest = json.load(f)
    tileset_id = manifest["id"]
    tiles = manifest["tiles"]
    assert len(tiles) == cols * rows, \
        f"manifest has {len(tiles)} tiles, sheet has {cols * rows}"

    os.makedirs(args.png_dir, exist_ok=True)
    changed = []
    for r in range(rows):
        for c in range(cols):
            idx = r * cols + c
            desc = csv_rows[r][c].strip()
            entry = tiles[idx]
            if entry.get("label", "").strip() == desc:
                continue
            slug = slugify(desc)
            tile_id = f"{tileset_id}_{slug}"
            if tileset_id == "desolate_landscape":
                gb_const = f"TILE_DESOLATE_LANDSCAPE_{idx:02d}"
            else:
                gb_const = f"TILE_{tileset_id.upper()}_{slug.upper()}"
            walkable = infer_walkable(desc)
            tile_img = img.crop((c * TILE_SIZE, r * TILE_SIZE,
                                 c * TILE_SIZE + TILE_SIZE, r * TILE_SIZE + TILE_SIZE))
            opaque = [p for p in tile_img.getdata() if p[3] > 0]
            if opaque:
                color = "#%02x%02x%02x" % (
                    sum(p[0] for p in opaque) // len(opaque),
                    sum(p[1] for p in opaque) // len(opaque),
                    sum(p[2] for p in opaque) // len(opaque))
            else:
                color = "#000000"
            png_name = f"{tile_id}.png"
            tile_img.save(os.path.join(args.png_dir, png_name))
            old_id = entry.get("id")
            entry.update({
                "id": tile_id,
                "label": desc.strip(),
                "gb_constant": gb_const,
                "walkable": walkable,
                "color": color,
                "ascii": ascii_char(walkable),
                "image_url": f"/tiles/{tileset_id}/{png_name}",
                "category": infer_category(desc),
            })
            changed.append((idx, old_id, tile_id, walkable))

    with open(args.tileset_json, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)
        f.write("\n")

    if not changed:
        print("merge: no cell changes; manifest already in sync.")
        return 0
    print(f"merge: {len(changed)} changed cell(s):")
    for idx, old_id, new_id, walkable in changed:
        print(f"  [{idx:2d}] {old_id} -> {new_id} "
              f"({'walkable' if walkable else 'solid'})")
    return 0


if __name__ == "__main__":
    sys.exit(main())

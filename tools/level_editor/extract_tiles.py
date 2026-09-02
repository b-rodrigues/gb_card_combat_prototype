#!/usr/bin/env python3
"""Import and slice ONLY the official tilesets:
- castle-tile.png (72x24 -> 9x3 = 27 tiles)
- combat-tile.png (128x24 -> 16x3 = 48 tiles)
- forest-tile.png (128x24 -> 16x3 = 48 tiles)
- overworld-tile.png (128x24 -> 16x3 = 48 tiles)
- intrepid.png (128x48 -> 16x6 = 96 tiles)

Removes all old testing tilesets from public/tiles/ and tilesets/.
"""

import json
import os
import shutil
from pathlib import Path
from PIL import Image

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
ASSETS_DIR = REPO_ROOT / "assets"
EDITOR_DIR = REPO_ROOT / "tools" / "level_editor"
PUBLIC_TILES_DIR = EDITOR_DIR / "public" / "tiles"
TILESETS_JSON_DIR = EDITOR_DIR / "tilesets"

TILE_SIZE = 8
UPSCALE = 4  # 32x32 nearest-neighbor

TARGET_SHEETS = [
    {
        "id": "castle",
        "label": "Castle & Bastion",
        "file": "castle-tile.png",
        "gb_tileset_kind": "WORLD_TILESET_CASTLE",
        "cols": 9,
        "rows": 3,
        "default_floor": (0, 2),  # (col 0, row 2) is plain gray floor
    },
    {
        "id": "combat",
        "label": "Combat & Battle HUD",
        "file": "combat-tile.png",
        "gb_tileset_kind": "WORLD_TILESET_COMBAT",
        "cols": 16,
        "rows": 3,
        "default_floor": None,
        "category": "ui",
    },
    {
        "id": "forest",
        "label": "Whispering Forest",
        "file": "forest-tile.png",
        "gb_tileset_kind": "WORLD_TILESET_FOREST",
        "cols": 16,
        "rows": 3,
        "default_floor": (0, 2),  # (col 0, row 2) is solid green grass floor
    },
    {
        "id": "overworld",
        "label": "Overworld Realm",
        "file": "overworld-tile.png",
        "gb_tileset_kind": "WORLD_TILESET_OVERWORLD",
        "cols": 16,
        "rows": 3,
        "default_floor": (0, 2),  # (col 0, row 2) is solid green grass floor
    },
    {
        "id": "intrepid",
        "label": "Intrepid Font & Glyphs",
        "file": "intrepid.png",
        "gb_tileset_kind": "WORLD_TILESET_INTREPID",
        "cols": 16,
        "rows": 6,
        "default_floor": None,
        "category": "ui",
    },
]


def rgb_to_hex(rgb):
    return f"#{rgb[0]:02x}{rgb[1]:02x}{rgb[2]:02x}"


def get_dominant_color(img_rgba):
    colors = img_rgba.getcolors(maxcolors=256)
    if not colors:
        return "#88c070"
    # Find most common non-transparent color
    colors.sort(key=lambda x: x[0], reverse=True)
    for count, col in colors:
        if len(col) == 4 and col[3] < 128:
            continue
        return rgb_to_hex(col[:3])
    return "#88c070"


def clean_old_tilesets():
    print("--- Cleaning old testing tilesets ---")
    active_ids = {s["id"] for s in TARGET_SHEETS}

    # 1. Clean public/tiles/
    if PUBLIC_TILES_DIR.exists():
        for d in PUBLIC_TILES_DIR.iterdir():
            if d.is_dir() and d.name not in active_ids:
                print(f"Removing old tile directory: {d.name}/")
                shutil.rmtree(d)

    # 2. Clean tilesets/*.json
    if TILESETS_JSON_DIR.exists():
        for f in TILESETS_JSON_DIR.glob("*.json"):
            if f.stem not in active_ids:
                print(f"Removing old tileset JSON: {f.name}")
                f.unlink()


def process_sheet(sheet):
    sheet_id = sheet["id"]
    png_path = ASSETS_DIR / sheet["file"]
    if not png_path.exists():
        raise FileNotFoundError(f"Missing required sheet: {png_path}")

    img = Image.open(png_path).convert("RGBA")
    out_tile_dir = PUBLIC_TILES_DIR / sheet_id
    out_tile_dir.mkdir(parents=True, exist_ok=True)

    cols, rows = sheet["cols"], sheet["rows"]
    tiles_list = []

    print(f"\nProcessing {sheet_id} from {sheet['file']} ({cols}x{rows} = {cols*rows} tiles)...")

    # If sheet has a default floor coordinate
    default_floor_coord = sheet.get("default_floor")

    # Load CSV description if forest
    csv_rows = None
    if sheet_id == "forest":
        csv_path = ASSETS_DIR / "forest-tileset-description.csv"
        if csv_path.exists():
            import csv
            with open(csv_path, "r", encoding="utf-8") as f:
                csv_rows = [row for row in csv.reader(f) if row]

    # Slicing tiles
    for r in range(rows):
        for c in range(cols):
            ox = c * TILE_SIZE
            oy = r * TILE_SIZE
            tile_8x8 = img.crop((ox, oy, ox + TILE_SIZE, oy + TILE_SIZE))
            tile_upscaled = tile_8x8.resize(
                (TILE_SIZE * UPSCALE, TILE_SIZE * UPSCALE),
                Image.NEAREST
            )

            tile_id = f"tile_{r}_{c}"
            # Also save canonical castle_r_c for castle
            if sheet_id == "castle":
                canonical_name = f"castle_{r}_{c}"
            else:
                canonical_name = tile_id

            out_path = out_tile_dir / f"{canonical_name}.png"
            tile_upscaled.save(out_path)

            # If tile_id differs from canonical_name, save both
            if canonical_name != tile_id:
                tile_upscaled.save(out_tile_dir / f"{tile_id}.png")

            is_floor = (default_floor_coord is not None and c == default_floor_coord[0] and r == default_floor_coord[1])
            is_walkable = is_floor

            dom_color = get_dominant_color(tile_8x8)
            category = sheet.get("category", "terrain")

            label = f"{sheet['label']} ({c},{r})"
            if is_floor:
                label = f"{sheet['label']} Floor"

            # Use CSV descriptions if available (for forest)
            if csv_rows and r < len(csv_rows) and c < len(csv_rows[r]):
                desc = csv_rows[r][c].strip()
                d_low = desc.lower()
                is_walkable = ("plain floor" in d_low or "walkable" in d_low or "exit" in d_low)
                if "corner" in d_low or "wall" in d_low:
                    category = "wall"
                elif "tree" in d_low or "stump" in d_low:
                    category = "nature"
                elif "enemy" in d_low:
                    category = "enemy"
                elif "hero" in d_low:
                    category = "object"
                elif "merchant" in d_low:
                    category = "npc"
                elif "fire" in d_low:
                    category = "object"
                elif "exit" in d_low or "floor" in d_low:
                    category = "terrain"

                cleaned_desc = desc.replace("florr", "floor").replace("enemay", "enemy").replace("three stump", "tree stump")
                label = " ".join(word.capitalize() for word in cleaned_desc.split())
                if is_walkable and "walkable" not in label.lower():
                    label += " (Walkable)"
                elif not is_walkable and category in ("wall", "nature"):
                    label += " (Solid)"

            tile_entry = {
                "id": canonical_name,
                "label": label,
                "gb_constant": f"TILE_{sheet_id.upper()}_{r}_{c}",
                "walkable": is_walkable,
                "color": dom_color,
                "ascii": "." if is_walkable else "#",
                "image_url": f"/tiles/{sheet_id}/{canonical_name}.png",
                "category": category,
            }
            tiles_list.append(tile_entry)

    # Convenience aliases for standard maps:
    # 1. Plain floor alias
    if default_floor_coord is not None:
        fc, fr = default_floor_coord
        floor_crop = img.crop((fc * TILE_SIZE, fr * TILE_SIZE, (fc + 1) * TILE_SIZE, (fr + 1) * TILE_SIZE)).resize(
            (TILE_SIZE * UPSCALE, TILE_SIZE * UPSCALE), Image.NEAREST
        )
        floor_crop.save(out_tile_dir / "floor.png")
        # Prepend 'floor' entry
        tiles_list.insert(0, {
            "id": "floor",
            "label": "Floor (Solid Walkable)",
            "gb_constant": "TILE_FLOOR",
            "walkable": True,
            "color": get_dominant_color(img.crop((fc * TILE_SIZE, fr * TILE_SIZE, (fc + 1) * TILE_SIZE, (fr + 1) * TILE_SIZE))),
            "ascii": ".",
            "image_url": f"/tiles/{sheet_id}/floor.png",
            "category": "terrain",
        })

    # 2. Forest map compatibility aliases (tree, gate, stump_*)
    if sheet_id == "forest":
        # Copy tree (1, 0)
        tree_crop = img.crop((8, 0, 16, 8)).resize((32, 32), Image.NEAREST)
        tree_crop.save(out_tile_dir / "tree.png")
        tiles_list.insert(1, {
            "id": "tree",
            "label": "Forest Tree",
            "gb_constant": "TILE_WALL",
            "walkable": False,
            "color": "#205838",
            "ascii": "#",
            "image_url": "/tiles/forest/tree.png",
            "category": "nature",
        })
        # gate (2, 0)
        gate_crop = img.crop((16, 0, 24, 8)).resize((32, 32), Image.NEAREST)
        gate_crop.save(out_tile_dir / "gate.png")
        tiles_list.insert(2, {
            "id": "gate",
            "label": "Forest Gate",
            "gb_constant": "TILE_EXIT",
            "walkable": True,
            "color": "#e0f8d0",
            "ascii": ">",
            "image_url": "/tiles/forest/gate.png",
            "category": "terrain",
        })
        # Stumps (tl, tr, bl, br)
        stumps = [
            ("stump_tl", (3, 0), "p"),
            ("stump_tr", (4, 0), "q"),
            ("stump_bl", (3, 1), "b"),
            ("stump_br", (4, 1), "d"),
        ]
        for s_id, (sc, sr), ascii_char in stumps:
            stump_crop = img.crop((sc * 8, sr * 8, (sc + 1) * 8, (sr + 1) * 8)).resize((32, 32), Image.NEAREST)
            stump_crop.save(out_tile_dir / f"{s_id}.png")
            tiles_list.append({
                "id": s_id,
                "label": f"Stump {s_id[-2:].upper()}",
                "gb_constant": f"TILE_{s_id.upper()}",
                "walkable": False,
                "color": "#704820",
                "ascii": ascii_char,
                "image_url": f"/tiles/forest/{s_id}.png",
                "category": "nature",
            })

    # 3. Castle compatibility aliases (wall)
    if sheet_id == "castle":
        wall_crop = img.crop((8, 8, 16, 16)).resize((32, 32), Image.NEAREST)
        wall_crop.save(out_tile_dir / "wall.png")
        tiles_list.insert(1, {
            "id": "wall",
            "label": "Castle Wall",
            "gb_constant": "TILE_WALL",
            "walkable": False,
            "color": "#7f8c8d",
            "ascii": "#",
            "image_url": "/tiles/castle/wall.png",
            "category": "wall",
        })

    # Save JSON definition
    tileset_json_data = {
        "id": sheet_id,
        "label": sheet["label"],
        "gb_tileset_kind": sheet["gb_tileset_kind"],
        "tiles": tiles_list,
    }
    json_path = TILESETS_JSON_DIR / f"{sheet_id}.json"
    TILESETS_JSON_DIR.mkdir(parents=True, exist_ok=True)
    with open(json_path, "w", encoding="utf-8") as fp:
        json.dump(tileset_json_data, fp, indent=2)

    print(f"Saved {json_path} ({len(tiles_list)} tiles)")


def main():
    clean_old_tilesets()
    for sheet in TARGET_SHEETS:
        process_sheet(sheet)
    print("\nAll 5 target tilesets successfully processed and imported!")


if __name__ == "__main__":
    main()

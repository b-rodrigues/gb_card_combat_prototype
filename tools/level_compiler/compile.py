#!/usr/bin/env python3
"""
compile.py - Level Compiler for Game Boy RPG Level Editor

Translates JSON level specifications into C scene definitions and terrain blocks
for the GBDK-4 Game Boy RPG engine.

Pipeline:
    JSON Level -> validate -> resolve tiles -> optimize terrain -> emit C (scenes_content.c)

Usage:
    python3 tools/level_compiler/compile.py levels/forest.json
    python3 tools/level_compiler/compile.py levels/*.json -o src/game/scenes_content.c
    python3 tools/level_compiler/compile.py --all -o src/game/scenes_content.c
"""

import sys
import os
import json
import glob
import argparse
from pathlib import Path

# Add level_compiler directory to sys.path for validator import
SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent.parent
sys.path.insert(0, str(SCRIPT_DIR))

from validate import validate_level, load_tilesets

# Canonical scene order matching MapId enum in src/world/world.h
SCENE_ORDER = ["field", "town", "forest", "mountain_pass", "castle"]

MAP_ENUM_MAP = {
    "field": "MAP_FIELD",
    "town": "MAP_TOWN",
    "forest": "MAP_FOREST",
    "mountain_pass": "MAP_MOUNTAIN_PASS",
    "castle": "MAP_CASTLE"
}

SCENE_ENUM_MAP = {
    "field": "SCENE_FIELD",
    "town": "SCENE_TOWN",
    "forest": "SCENE_FOREST",
    "mountain_pass": "SCENE_MOUNTAIN_PASS",
    "castle": "SCENE_CASTLE"
}

TILESET_KIND_MAP = {
    "exterior": "WORLD_TILESET_EXTERIOR",
    "interior": "WORLD_TILESET_INTERIOR",
    "forest": "WORLD_TILESET_FOREST"
}


def load_level(path):
    """Load and parse level JSON."""
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def resolve_tiles(level_data, tileset):
    """Resolve semantic tile IDs to C constants."""
    tile_dict = {t["id"]: t for t in tileset.get("tiles", [])}
    return tile_dict


def derive_collision(level_data, tileset):
    """Derive 2D collision grid (True=walkable, False=blocked)."""
    width = level_data["map"]["width"]
    height = level_data["map"]["height"]
    grid = [[True for _ in range(width)] for _ in range(height)]

    # Perimeter walls
    for x in range(width):
        grid[0][x] = False
        grid[height - 1][x] = False
    for y in range(height):
        grid[y][0] = False
        grid[y][width - 1] = False

    tile_dict = resolve_tiles(level_data, tileset)
    terrain = level_data.get("layers", {}).get("terrain", [])

    if isinstance(terrain, list) and len(terrain) > 0 and isinstance(terrain[0], list):
        for y in range(min(height, len(terrain))):
            for x in range(min(width, len(terrain[y]))):
                t_id = terrain[y][x].split(".")[-1]
                t_info = tile_dict.get(t_id, {})
                if not t_info.get("walkable", True):
                    grid[y][x] = False
    elif isinstance(terrain, list):
        for block in terrain:
            bx = block.get("x", 0)
            by = block.get("y", 0)
            bw = block.get("width", 0)
            bh = block.get("height", 0)
            t_id = block.get("tile", "").split(".")[-1]
            t_info = tile_dict.get(t_id, {})
            if not t_info.get("walkable", True):
                for cy in range(by, min(height, by + bh)):
                    for cx in range(bx, min(width, bx + bw)):
                        grid[cy][cx] = False

    return grid


def optimize_terrain(level_data, tileset):
    """
    Extract minimal list of SceneTerrainBlock rects.
    Returns list of dicts: [{'x': x, 'y': y, 'w': w, 'h': h, 'tile': 'TILE_WALL'}, ...]
    """
    tile_dict = resolve_tiles(level_data, tileset)
    terrain = level_data.get("layers", {}).get("terrain", [])
    blocks_out = []

    if isinstance(terrain, list) and len(terrain) > 0 and isinstance(terrain[0], dict):
        # Already formatted as block rects
        for b in terrain:
            t_id = b.get("tile", "").split(".")[-1]
            t_info = tile_dict.get(t_id, {})
            gb_const = t_info.get("gb_constant", "TILE_WALL")
            # If it's TILE_FLOOR and it's the default background, skip unless needed
            if gb_const == "TILE_FLOOR":
                continue
            blocks_out.append({
                "x": b["x"],
                "y": b["y"],
                "w": b["width"],
                "h": b["height"],
                "tile": gb_const,
                "comment": b.get("comment", "")
            })
        return blocks_out

    if isinstance(terrain, list) and len(terrain) > 0 and isinstance(terrain[0], list):
        # 2D Grid: Optimize using 2D greedy rectangle merging
        width = level_data["map"]["width"]
        height = level_data["map"]["height"]
        visited = [[False for _ in range(width)] for _ in range(height)]

        for y in range(height):
            for x in range(width):
                if visited[y][x]:
                    continue
                t_id = terrain[y][x].split(".")[-1]
                t_info = tile_dict.get(t_id, {})
                gb_const = t_info.get("gb_constant", "TILE_FLOOR")

                # Default background is TILE_FLOOR, and perimeter is TILE_WALL
                if gb_const == "TILE_FLOOR":
                    visited[y][x] = True
                    continue

                # Find max width
                bw = 1
                while x + bw < width and not visited[y][x + bw]:
                    next_tid = terrain[y][x + bw].split(".")[-1]
                    next_const = tile_dict.get(next_tid, {}).get("gb_constant", "TILE_FLOOR")
                    if next_const != gb_const:
                        break
                    bw += 1

                # Find max height
                bh = 1
                while y + bh < height:
                    row_ok = True
                    for cx in range(x, x + bw):
                        if visited[y + bh][cx]:
                            row_ok = False
                            break
                        row_tid = terrain[y + bh][cx].split(".")[-1]
                        row_const = tile_dict.get(row_tid, {}).get("gb_constant", "TILE_FLOOR")
                        if row_const != gb_const:
                            row_ok = False
                            break
                    if not row_ok:
                        break
                    bh += 1

                # Mark visited
                for cy in range(y, y + bh):
                    for cx in range(x, x + bw):
                        visited[cy][cx] = True

                blocks_out.append({
                    "x": x,
                    "y": y,
                    "w": bw,
                    "h": bh,
                    "tile": gb_const
                })

    return blocks_out


def emit_exits(levels_by_id):
    """
    Build g_all_exits array and compute offsets/counts for each scene.
    Returns (exits_c_code, scene_exit_offsets).
    """
    all_exits = []
    scene_exit_info = {}

    # Order by SCENE_ORDER
    ordered_ids = [sid for sid in SCENE_ORDER if sid in levels_by_id]
    for sid in levels_by_id:
        if sid not in ordered_ids:
            ordered_ids.append(sid)

    for sid in ordered_ids:
        lvl = levels_by_id[sid]
        exits = lvl.get("exits", [])
        start_index = len(all_exits)
        count = len(exits)
        scene_exit_info[sid] = (start_index, count)

        for e in exits:
            target_scene_id = e["target_scene"]
            target_enum = SCENE_ENUM_MAP.get(target_scene_id, f"SCENE_{target_scene_id.upper()}")
            tile_char = e.get("tile_char", ">")
            all_exits.append({
                "gate_x": e["x"],
                "gate_y": e["y"],
                "spawn_x": e["target_x"],
                "spawn_y": e["target_y"],
                "target_scene": target_enum,
                "tile_char": tile_char
            })

    lines = []
    lines.append("const SceneExit g_all_exits[] = {")
    exit_lines = []
    for e in all_exits:
        target_str = f"{e['target_scene']},"
        exit_lines.append(f"    {{ {e['gate_x']:2d}, {e['gate_y']:2d}, {e['spawn_x']:2d}, {e['spawn_y']:2d}, {target_str:<20s} '{e['tile_char']}' }}")
    lines.append(",\n".join(exit_lines))
    lines.append("};\n")

    return "\n".join(lines), scene_exit_info


def emit_c_code(levels_by_id, tilesets):
    """Generate complete C file matching src/game/scenes_content.c."""
    ordered_ids = [sid for sid in SCENE_ORDER if sid in levels_by_id]
    for sid in levels_by_id:
        if sid not in ordered_ids:
            ordered_ids.append(sid)

    output = []
    output.append("/* Generated by tools/level_compiler/compile.py -- DO NOT EDIT DIRECTLY */")
    output.append("#pragma bank 5\n")
    output.append('#include "scene.h"\n')

    # 1. Emit Exits Table
    exits_code, exit_offsets = emit_exits(levels_by_id)
    output.append(exits_code)

    # 2. Emit Terrain Blocks for each scene
    scene_terrain_symbols = {}

    for sid in ordered_ids:
        lvl = levels_by_id[sid]
        tileset_id = lvl["map"]["tileset"]
        tileset = tilesets.get(tileset_id, {})
        blocks = optimize_terrain(lvl, tileset)

        if not blocks:
            scene_terrain_symbols[sid] = "0"
            continue

        sym_name = f"s_{sid}_terrain"
        scene_terrain_symbols[sid] = sym_name

        output.append(f"static const SceneTerrainBlock {sym_name}[] = {{")
        for b in blocks:
            output.append(f"    {{ {b['x']}, {b['y']}, {b['w']}, {b['h']}, {b['tile']} }},")
        output.append("    { 0, 0, 0, 0, 0 }")
        output.append("};\n")

    # 3. Emit Scene Definition Table
    output.append("const SceneDefinition g_scenes[] = {")
    scene_defs = []
    for sid in ordered_ids:
        lvl = levels_by_id[sid]
        map_id_enum = MAP_ENUM_MAP.get(sid, f"MAP_{sid.upper()}")
        music_enum = lvl["map"].get("music", "MUSIC_OVERWORLD")
        width = lvl["map"]["width"]
        height = lvl["map"]["height"]
        start_idx, count = exit_offsets[sid]
        exits_ptr = f"&g_all_exits[{start_idx}]" if count > 0 else "0"
        tileset_kind = TILESET_KIND_MAP.get(lvl["map"]["tileset"], "WORLD_TILESET_EXTERIOR")
        terrain_ptr = scene_terrain_symbols.get(sid, "0")

        scene_defs.append(
            f"    {{ {map_id_enum + ',':<20s} {music_enum + ',':<18s} {width:2d}, {height:2d}, {exits_ptr + ',':<20s} {count}, {tileset_kind + ',':<24s} {terrain_ptr} }}"
        )
    output.append(",\n".join(scene_defs))
    output.append("};\n")

    return "\n".join(output)


def main():
    parser = argparse.ArgumentParser(description="Compile Game Boy RPG level JSON files to C.")
    parser.add_argument("levels", nargs="*", help="Level JSON files to compile")
    parser.add_argument("--all", action="store_true", help="Compile all level files in levels/ directory")
    parser.add_argument("-o", "--output", help="Output C file path (default: src/game/scenes_content.c)")
    parser.add_argument("--tilesets-dir", help="Tilesets directory")

    args = parser.parse_args()

    tilesets = load_tilesets(args.tilesets_dir)

    level_files = []
    if args.all or not args.levels:
        levels_dir = REPO_ROOT / "levels"
        level_files = sorted(list(levels_dir.glob("*.json")))
    else:
        for pattern in args.levels:
            matches = glob.glob(pattern)
            if matches:
                level_files.extend([Path(m) for m in matches])
            else:
                level_files.append(Path(pattern))

    if not level_files:
        print("No level files found to compile.", file=sys.stderr)
        sys.exit(1)

    levels_by_id = {}
    has_errors = False

    for lpath in level_files:
        try:
            data = load_level(lpath)
            is_valid, errors, warnings, passed = validate_level(data, tilesets)
            if not is_valid:
                print(f"Validation failed for {lpath}:", file=sys.stderr)
                for err in errors:
                    print(f"  - ERROR: {err}", file=sys.stderr)
                has_errors = True
            levels_by_id[data["id"]] = data
        except Exception as e:
            print(f"Failed to read/validate {lpath}: {e}", file=sys.stderr)
            has_errors = True

    if has_errors:
        print("\nCompilation aborted due to validation errors.", file=sys.stderr)
        sys.exit(1)

    c_code = emit_c_code(levels_by_id, tilesets)

    output_path = args.output
    if not output_path:
        output_path = REPO_ROOT / "src" / "game" / "scenes_content.c"
    else:
        output_path = Path(output_path)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(c_code)

    print(f"Successfully compiled {len(levels_by_id)} level(s) to {output_path}")


if __name__ == "__main__":
    main()

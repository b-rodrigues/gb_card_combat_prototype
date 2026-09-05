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
SCENE_ORDER = ["field", "town", "forest", "mountain_pass", "castle", "south_field"]

MAP_ENUM_MAP = {
    "field": "MAP_FIELD",
    "town": "MAP_TOWN",
    "forest": "MAP_FOREST",
    "mountain_pass": "MAP_MOUNTAIN_PASS",
    "castle": "MAP_CASTLE",
    "south_field": "MAP_SOUTH_FIELD"
}

SCENE_ENUM_MAP = {
    "field": "SCENE_FIELD",
    "town": "SCENE_TOWN",
    "forest": "SCENE_FOREST",
    "mountain_pass": "SCENE_MOUNTAIN_PASS",
    "castle": "SCENE_CASTLE",
    "south_field": "SCENE_SOUTH_FIELD"
}

TILESET_KIND_MAP = {
    "forest": "WORLD_TILESET_FOREST",
    "village": "WORLD_TILESET_VILLAGE",
    "desolate_landscape": "WORLD_TILESET_DESOLATE",
    "desolate": "WORLD_TILESET_DESOLATE",
    "castle": "WORLD_TILESET_CASTLE",
}

# player.spawn facing spellings (schema allows compass aliases) to the
# engine Direction enum.  The spawn is compiled into SceneDefinition so the
# JSON is the single source of truth (see game_new_game).
SPAWN_FACING_MAP = {
    "UP": "DIRECTION_UP",
    "NORTH": "DIRECTION_UP",
    "DOWN": "DIRECTION_DOWN",
    "SOUTH": "DIRECTION_DOWN",
    "LEFT": "DIRECTION_LEFT",
    "WEST": "DIRECTION_LEFT",
    "RIGHT": "DIRECTION_RIGHT",
    "EAST": "DIRECTION_RIGHT",
}


def load_level(path):
    """Load and parse level JSON."""
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def first_plain_tile(tileset_id, tileset):
    """First tile id with 'plain' in the name (manifest order)."""
    for t in tileset.get("tiles", []):
        if "plain" in t.get("id", ""):
            return "%s.%s" % (tileset_id, t["id"])
    return None


def level_default_tile(level_data, tilesets):
    """The level's ground tile for unpainted cells: the explicit
    default_walkable field, else the first plain tile in manifest order."""
    tileset_id = level_data["map"]["tileset"]
    tileset = tilesets.get(tileset_id, {})
    explicit = level_data.get("default_walkable", "")
    if explicit:
        return explicit
    return first_plain_tile(tileset_id, tileset) or ""


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


def map_base_tile_const(gb_const, t_info):
    if gb_const in ("TILE_FLOOR", "TILE_WALL", "TILE_EXIT", "TILE_BUILDING",
                    "TILE_STUMP_TL", "TILE_STUMP_TR", "TILE_STUMP_BL", "TILE_STUMP_BR"):
        return gb_const
    if gb_const and gb_const.startswith("TILE_"):
        return gb_const
    ascii_char = t_info.get("ascii", "#")
    if ascii_char == ".":
        return "TILE_FLOOR"
    elif ascii_char == "#":
        return "TILE_WALL"
    elif ascii_char in (">", "<"):
        return "TILE_EXIT"
    elif ascii_char == "B" or ascii_char == "*":
        return "TILE_BUILDING"
    return "TILE_FLOOR" if t_info.get("walkable", True) else "TILE_WALL"


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
            gb_const = map_base_tile_const(t_info.get("gb_constant", "TILE_WALL"), t_info)
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
                gb_const = map_base_tile_const(t_info.get("gb_constant", "TILE_FLOOR"), t_info)

                # Default background is TILE_FLOOR, and perimeter is TILE_WALL
                if gb_const in ("TILE_FLOOR", "TILE_DESOLATE_FLOOR_PLAIN"):
                    visited[y][x] = True
                    continue

                # Find max width
                bw = 1
                while x + bw < width and not visited[y][x + bw]:
                    next_tid = terrain[y][x + bw].split(".")[-1]
                    next_info = tile_dict.get(next_tid, {})
                    next_const = map_base_tile_const(next_info.get("gb_constant", "TILE_FLOOR"), next_info)
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
                        row_info = tile_dict.get(row_tid, {})
                        row_const = map_base_tile_const(row_info.get("gb_constant", "TILE_FLOOR"), row_info)
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
        if music_enum in ("MUSIC_DESOLATE_LANDSCAPE", "desolate_landscape"):
            music_enum = "MUSIC_DESOLATE"
        elif music_enum in ("MUSIC_FOREST", "forest", "Forest"):
            music_enum = "MUSIC_FOREST"
        width = lvl["map"]["width"]
        height = lvl["map"]["height"]
        start_idx, count = exit_offsets[sid]
        exits_ptr = f"&g_all_exits[{start_idx}]" if count > 0 else "0"
        tileset_kind = TILESET_KIND_MAP.get(lvl["map"]["tileset"], "WORLD_TILESET_FOREST")
        terrain_ptr = scene_terrain_symbols.get(sid, "0")
        spawn = lvl.get("player", {}).get("spawn", {})
        spawn_facing = SPAWN_FACING_MAP.get(
            str(spawn.get("facing", "DOWN")).upper(), "DIRECTION_DOWN")
        default_tile = level_default_tile(lvl, tilesets)
        tile_dict = resolve_tiles(lvl, tilesets.get(lvl["map"]["tileset"], {}))
        default_info = tile_dict.get(default_tile.split(".")[-1], {})
        default_const = map_base_tile_const(
            default_info.get("gb_constant", "TILE_FLOOR"), default_info)

        scene_defs.append(
            f"    {{ {map_id_enum + ',':<20s} {music_enum + ',':<18s} {width:2d}, {height:2d}, {exits_ptr + ',':<20s} {count}, {tileset_kind + ',':<24s} {terrain_ptr + ',':<20s} {spawn.get('x', 0)}, {spawn.get('y', 0)}, {spawn_facing + ',':<16s} {default_const} }}"
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
    parser.add_argument("--actors-output", help="Actor tables output C file path (default: src/game/actors_content.c)")
    parser.add_argument("--check", action="store_true",
                        help="Do not write; exit nonzero if fresh output differs from the file")

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

    actors_code = emit_actors_code(levels_by_id)

    actors_path = args.actors_output
    if not actors_path:
        actors_path = REPO_ROOT / "src" / "game" / "actors_content.c"
    else:
        actors_path = Path(actors_path)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    if args.check:
        for path, fresh in ((output_path, c_code), (actors_path, actors_code)):
            try:
                committed = path.read_text(encoding="utf-8")
            except FileNotFoundError:
                committed = None
            if committed is None or committed.strip() != fresh.strip():
                print(f"DRIFT: fresh compile differs from {path}", file=sys.stderr)
                sys.exit(1)
        print(f"compile --check OK: {output_path} and {actors_path} match fresh output")
        return
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(c_code)
    actors_path.parent.mkdir(parents=True, exist_ok=True)
    with open(actors_path, "w", encoding="utf-8") as f:
        f.write(actors_code)

    print(f"Successfully compiled {len(levels_by_id)} level(s) to {output_path}")


"""Resolve the overworld sprite kind for a level object from its editor
sprite choice (overworld_sprite / animation_frames tile names) or its
entity type.  Matching is by substring on the tileset-qualified tile ids:
the editor names each hostile actor's sprite after its art, e.g.
`desolate_landscape_enemy_kobold_frame_1`, `desolate_landscape_enemy_bats_*`,
and the castle boss `castle_top_left_boss` (a 2x2 sprite)."""
def resolve_sprite_kind(obj):
    frames = obj.get("animation_frames") or []
    names = " ".join(str(f) for f in frames) + " " + str(obj.get("overworld_sprite") or "")
    for token, kind in (("kobold", "SPRITE_KIND_KOBOLD"),
                        ("boss", "SPRITE_KIND_BOSS"),
                        ("bat", "SPRITE_KIND_BAT"),
                        ("chest", "SPRITE_KIND_CHEST")):
        if token in names:
            return kind
    ent = (obj.get("properties") or {}).get("entity_id", "")
    if "ENTITY_ID_SLIME_LORD" in ent:
        return "SPRITE_KIND_BOSS"
    if "ENTITY_ID_SLIME" in ent:
        return "SPRITE_KIND_KOBOLD"
    if "ENTITY_ID_BAT" in ent:
        return "SPRITE_KIND_BAT"
    if ent in ("ENTITY_ID_MAYOR", "ENTITY_ID_GUARD",
               "ENTITY_ID_SHOPKEEPER", "ENTITY_ID_MERCHANT",
               "ENTITY_ID_WIZARD"):
        return "SPRITE_KIND_TILE"
    return "SPRITE_KIND_ASCII"


"""Full-fidelity actor tables: every WorldActorDefinition row is generated
from its level JSON object, so the JSON roundtrips the C rows (see
decompile.py). Table order matches the historic file to keep diffs small."""
ACTOR_TABLE_ORDER = ["town", "field", "forest", "mountain_pass", "castle", "south_field"]


def default_actor_flags(otype):
    if otype == "enemy":
        return ["HOSTILE", "BLOCKING", "INTERACTABLE"]
    return ["BLOCKING", "INTERACTABLE"]


def default_actor_visual(obj):
    props = obj.get("properties", {})
    ent = props.get("entity_id", "")
    if ent == "ENTITY_ID_SLIME":
        return "E"
    if ent == "ENTITY_ID_BAT":
        return "V"
    if ent == "ENTITY_ID_SLIME_LORD":
        return "L"
    if ent in ("ENTITY_ID_SIGNPOST", "ENTITY_ID_AMULET"):
        return "?"
    name = props.get("display_name", "?")
    return name[0] if name else "?"


def actor_interaction(obj):
    props = obj.get("properties", {})
    if obj.get("type") == "enemy":
        return "INTERACTION_COMBAT"
    if "shop" in props:
        return "INTERACTION_SHOP"
    if "save" in props:
        return "INTERACTION_SAVE"
    if "dialogue" in props:
        return "INTERACTION_DIALOGUE"
    return "INTERACTION_NONE"


def emit_actor_row(obj):
    props = obj.get("properties", {})
    pos = obj.get("position", {})
    ent = props.get("entity_id")
    if not ent:
        raise ValueError(f"actor object '{obj.get('id')}' has no properties.entity_id")
    x = pos.get("x", 0)
    y = pos.get("y", 0)
    facing = "DIRECTION_" + props.get("facing", "DOWN")
    flags = props.get("flags")
    if flags is None:
        flags = default_actor_flags(obj.get("type"))
    flag_expr = " | ".join("ACTOR_FLAG_" + f for f in flags) if flags else "0"
    visual = "'" + props.get("visual", default_actor_visual(obj)) + "'"
    name = '"%s"' % props.get("display_name", ent)
    inter = actor_interaction(obj)
    shop = props.get("shop", 0)
    dlg = props.get("dialogue", "DIALOGUE_ID_NONE")
    battle = props.get("battle", "BATTLE_NONE")
    ai = props.get("ai", "AI_NONE")
    hp = props.get("hp", 0)
    max_hp = props.get("max_hp", 0)
    gold = props.get("gold_reward", 0)
    cur = props.get("reward_currency", "0")
    svar = props.get("quest_var", "0")
    sval = props.get("quest_val", 0)
    spk = resolve_sprite_kind(obj)
    aid = props.get("actor_id", 0)
    line1 = f"        {aid}, {ent}, {x}, {y}, {facing},"
    line2 = f"        {flag_expr},"
    line3 = (f"        {visual}, {name}, {inter}, {shop}, {dlg}, {battle}, {ai}, "
             f"{hp}, {max_hp}, {gold}, {cur},")
    line4 = f"        {svar}, {sval},"
    line5 = f"         {spk}"
    return "    {\n" + "\n".join([line1, line2, line3, line4, line5]) + "\n    },"


def emit_actors_code(levels_by_id):
    """Generate complete src/game/actors_content.c."""
    ordered = [sid for sid in ACTOR_TABLE_ORDER if sid in levels_by_id]
    for sid in levels_by_id:
        if sid not in ordered:
            ordered.append(sid)
    out = []
    out.append("/* Generated by tools/level_compiler/compile.py -- DO NOT EDIT DIRECTLY */")
    out.append("#pragma bank 2\n")
    out.append('#include "actor.h"')
    out.append('#include "game_ids.h"\n')
    out.append("/* ── Scene-owned actor definitions (game content) ──────────────────")
    out.append(" *")
    out.append(" * Friendly actors are pure static definitions.  Hostile actors are")
    out.append(" * spawned into World.actors runtime slots by actor_load_scene(), so a")
    out.append(" * scene can hold several hostile actors at once.  Each hostile definition")
    out.append(" * carries a stable ActorId (unique across scenes) so its defeat can be")
    out.append(" * recorded persistently in GameState.world and survive scene reloads.")
    out.append(" */\n")
    for sid in ordered:
        out.append(f"static const WorldActorDefinition g_{sid}_actors[] = {{")
        for obj in levels_by_id[sid].get("objects", []):
            props = obj.get("properties", {}) or {}
            if not props.get("entity_id"):
                continue  # decoration object: no engine row
            out.append(emit_actor_row(obj))
        out.append("};\n")
    out.append("const WorldActorTable g_actor_tables[] = {")
    for sid in ordered:
        map_enum = MAP_ENUM_MAP.get(sid, f"MAP_{sid.upper()}")
        out.append(f"    {{ {map_enum + ',':<20s} g_{sid}_actors,")
        out.append(f"        (uint8_t)(sizeof(g_{sid}_actors) / sizeof(g_{sid}_actors[0])) }},")
    out.append("};")
    return "\n".join(out) + "\n"


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
validate.py - Level validator for Game Boy RPG Level Editor

Validates level JSON files against schemas and semantic game rules:
- Schema validation
- Tileset and semantic tile validity
- Map bounds and dimensions (<= 40x24)
- Player spawn validity and collision
- Exit placement and target scene validity
- Object IDs uniqueness and placement
- Region bounds

Usage:
    python3 tools/level_compiler/validate.py levels/forest.json
    python3 tools/level_compiler/validate.py levels/*.json
"""

import sys
import os
import json
import glob
from pathlib import Path

MAX_WORLD_WIDTH = 40
MAX_WORLD_HEIGHT = 24

KNOWN_SCENES = {
    "field": "SCENE_FIELD",
    "town": "SCENE_TOWN",
    "forest": "SCENE_FOREST",
    "mountain_pass": "SCENE_MOUNTAIN_PASS",
    "castle": "SCENE_CASTLE",
    "south_field": "SCENE_SOUTH_FIELD",
    # Frozen harness-test fixtures (tools/scenarios/fixtures/levels/):
    # their exits legitimately retarget to the test scenes.
    "test_field": "SCENE_TEST_FIELD",
    "test_town": "SCENE_TEST_TOWN",
    "test_forest": "SCENE_TEST_FOREST",
    "test_mountain_pass": "SCENE_TEST_MOUNTAIN_PASS",
    "test_castle": "SCENE_TEST_CASTLE",
    "test_south_field": "SCENE_TEST_SOUTH_FIELD"
}

KNOWN_MAP_IDS = {
    "MAP_FIELD": 0,
    "MAP_TOWN": 1,
    "MAP_FOREST": 2,
    "MAP_MOUNTAIN_PASS": 3,
    "MAP_CASTLE": 4,
    "MAP_SOUTH_FIELD": 5,
    "MAP_TEST_FIELD": 6,
    "MAP_TEST_TOWN": 7,
    "MAP_TEST_FOREST": 8,
    "MAP_TEST_MOUNTAIN_PASS": 9,
    "MAP_TEST_CASTLE": 10,
    "MAP_TEST_SOUTH_FIELD": 11
}


def load_tilesets(tilesets_dir=None):
    """Load all available tilesets from standard directories."""
    tilesets = {}
    search_dirs = []
    if tilesets_dir:
        search_dirs.append(Path(tilesets_dir))
    
    script_dir = Path(__file__).resolve().parent
    repo_root = script_dir.parent.parent
    search_dirs.extend([
        repo_root / "tools" / "level_editor" / "tilesets",
        repo_root / "levels" / "tilesets",
        repo_root / "levels" / "schema"
    ])

    for sdir in search_dirs:
        if sdir.is_dir():
            for p in sdir.glob("*.json"):
                if "schema" in p.name:
                    continue
                try:
                    with open(p, "r", encoding="utf-8") as f:
                        data = json.load(f)
                        if "id" in data and "tiles" in data:
                            tilesets[data["id"]] = data
                except Exception:
                    pass
    return tilesets


_KNOWN_ENTITY_IDS = None


def known_entity_ids():
    """Parse ENTITY_ID_* #defines out of src/game/game_ids.h (the single
    source of truth for the game-layer entity id range)."""
    global _KNOWN_ENTITY_IDS
    if _KNOWN_ENTITY_IDS is None:
        import re
        path = (Path(__file__).resolve().parent.parent.parent
                / "src" / "game" / "game_ids.h")
        text = path.read_text()
        _KNOWN_ENTITY_IDS = set(re.findall(r"#define\s+(ENTITY_ID_\w+)", text))
        _KNOWN_ENTITY_IDS.add("ENTITY_ID_PLAYER")
    return _KNOWN_ENTITY_IDS


def validate_level(level_data, tilesets=None, all_level_ids=None):
    """Validate a loaded level dict. Returns (is_valid, list_of_errors, list_of_warnings, checks_passed)."""
    errors = []
    warnings = []
    passed = []

    if tilesets is None:
        tilesets = load_tilesets()

    # 1. Schema / Structure Check
    schema_ok = True
    required_top = ["id", "name", "map", "layers", "player"]
    for field in required_top:
        if field not in level_data:
            errors.append(f"Missing required field: '{field}'")
            schema_ok = False

    map_info = level_data.get("map", {})
    if not isinstance(map_info, dict):
        errors.append("'map' must be an object")
        schema_ok = False
    else:
        for f in ["width", "height", "tileset"]:
            if f not in map_info:
                errors.append(f"Missing map property: '{f}'")
                schema_ok = False

    if schema_ok:
        passed.append("Schema valid")

    level_id = level_data.get("id", "unknown")
    width = map_info.get("width", 0)
    height = map_info.get("height", 0)

    # Bounds check
    if width <= 0 or width > MAX_WORLD_WIDTH:
        errors.append(f"Map width {width} is out of bounds (1..{MAX_WORLD_WIDTH})")
    if height <= 0 or height > MAX_WORLD_HEIGHT:
        errors.append(f"Map height {height} is out of bounds (1..{MAX_WORLD_HEIGHT})")

    # 2. Tileset & Tiles Check
    tileset_id = map_info.get("tileset", "")
    tileset = tilesets.get(tileset_id)
    tiles_ok = True

    if not tileset:
        errors.append(f"Unknown tileset: '{tileset_id}' (available: {list(tilesets.keys())})")
        tiles_ok = False
    else:
        tile_map = {t["id"]: t for t in tileset.get("tiles", [])}
        layers = level_data.get("layers", {})
        terrain = layers.get("terrain", [])

        # Check 2D grid or blocks
        if isinstance(terrain, list):
            if len(terrain) > 0 and isinstance(terrain[0], list):
                # 2D Grid
                if len(terrain) != height:
                    errors.append(f"Terrain grid row count ({len(terrain)}) does not match height ({height})")
                    tiles_ok = False
                for r_idx, row in enumerate(terrain):
                    if len(row) != width:
                        errors.append(f"Terrain grid row {r_idx} column count ({len(row)}) does not match width ({width})")
                        tiles_ok = False
                    for c_idx, cell in enumerate(row):
                        # cell can be "forest.tree" or "tree"
                        tile_name = cell.split(".")[-1] if "." in cell else cell
                        if tile_name not in tile_map:
                            errors.append(f"Unknown tile '{cell}' at ({c_idx}, {r_idx})")
                            tiles_ok = False
            elif len(terrain) > 0 and isinstance(terrain[0], dict):
                # Block list
                for b_idx, block in enumerate(terrain):
                    bx = block.get("x", 0)
                    by = block.get("y", 0)
                    bw = block.get("width", 0)
                    bh = block.get("height", 0)
                    bt = block.get("tile", "")
                    t_name = bt.split(".")[-1] if "." in bt else bt
                    if t_name not in tile_map:
                        errors.append(f"Unknown tile '{bt}' in terrain block {b_idx}")
                        tiles_ok = False
                    if bx < 0 or bx + bw > width or by < 0 or by + bh > height:
                        errors.append(f"Terrain block {b_idx} at ({bx},{by},{bw},{bh}) exceeds map bounds ({width}x{height})")
                        tiles_ok = False

    if tiles_ok and tileset:
        passed.append("Tiles valid")

    # 2b. Default ground check (single source of truth for unpainted cells)
    default_walkable = level_data.get("default_walkable", "")
    if not default_walkable:
        # Implicit rule: first tile with 'plain' in manifest order.
        for t in (tileset or {}).get("tiles", []):
            if "plain" in t.get("id", ""):
                default_walkable = "%s.%s" % (tileset_id, t["id"])
                break
    if not default_walkable:
        errors.append("No default_walkable field and no 'plain' tile in tileset '%s'" % tileset_id)
    else:
        short = default_walkable.split(".")[-1]
        tile_map = {t["id"]: t for t in (tileset or {}).get("tiles", [])}
        if short not in tile_map:
            errors.append("default_walkable tile '%s' is not in tileset '%s'" % (default_walkable, tileset_id))
        elif not tile_map[short].get("walkable", True):
            errors.append("default_walkable tile '%s' is not walkable" % default_walkable)
        else:
            passed.append("Default ground valid")

    # 3. Collision & Player Spawn Check
    collision_ok = True
    spawn_info = level_data.get("player", {}).get("spawn", {})
    sp_x = spawn_info.get("x", -1)
    sp_y = spawn_info.get("y", -1)

    # The spawn is compiled into SceneDefinition (game_new_game reads the
    # FIELD row), so the facing spelling must map to a Direction enum.
    spawn_facing = str(spawn_info.get("facing", "DOWN")).upper()
    if spawn_facing not in ("UP", "DOWN", "LEFT", "RIGHT",
                            "NORTH", "SOUTH", "WEST", "EAST"):
        errors.append(f"Player spawn facing '{spawn_info.get('facing')}' is unknown "
                      f"(want UP/DOWN/LEFT/RIGHT or a compass alias)")
        collision_ok = False

    if sp_x < 0 or sp_x >= width or sp_y < 0 or sp_y >= height:
        errors.append(f"Player spawn ({sp_x}, {sp_y}) is outside map bounds ({width}x{height})")
        collision_ok = False
    else:
        # Check if spawn is on map boundary wall
        if sp_x == 0 or sp_x == width - 1 or sp_y == 0 or sp_y == height - 1:
            warnings.append(f"Player spawn ({sp_x}, {sp_y}) is on perimeter boundary wall")

        # Check if spawn tile is walkable
        if tileset:
            tile_map = {t["id"]: t for t in tileset.get("tiles", [])}
            layers = level_data.get("layers", {})
            terrain = layers.get("terrain", [])
            spawn_blocked = False
            if isinstance(terrain, list) and len(terrain) > 0 and isinstance(terrain[0], list):
                if sp_y < len(terrain) and sp_x < len(terrain[sp_y]):
                    cell = terrain[sp_y][sp_x]
                    t_name = cell.split(".")[-1] if "." in cell else cell
                    if t_name in tile_map and not tile_map[t_name].get("walkable", True):
                        spawn_blocked = True
            elif isinstance(terrain, list) and len(terrain) > 0 and isinstance(terrain[0], dict):
                for block in terrain:
                    bx = block.get("x", 0)
                    by = block.get("y", 0)
                    bw = block.get("width", 0)
                    bh = block.get("height", 0)
                    bt = block.get("tile", "")
                    if bx <= sp_x < bx + bw and by <= sp_y < by + bh:
                        t_name = bt.split(".")[-1] if "." in bt else bt
                        if t_name in tile_map and not tile_map[t_name].get("walkable", True):
                            spawn_blocked = True
            if spawn_blocked:
                errors.append(f"Player spawn at ({sp_x}, {sp_y}) is blocked by non-walkable tile")
                collision_ok = False

    if collision_ok:
        passed.append("Collision valid")
        passed.append("Player spawn valid")

    # 4. Exits Check
    exits_ok = True
    exits = level_data.get("exits", [])
    for e_idx, exit_obj in enumerate(exits):
        ex = exit_obj.get("x", -1)
        ey = exit_obj.get("y", -1)
        target = exit_obj.get("target_scene", "")
        tx = exit_obj.get("target_x", -1)
        ty = exit_obj.get("target_y", -1)

        if ex < 0 or ex >= width or ey < 0 or ey >= height:
            errors.append(f"Exit {e_idx} gate position ({ex}, {ey}) is outside map bounds ({width}x{height})")
            exits_ok = False

        if target not in KNOWN_SCENES and (all_level_ids is None or target not in all_level_ids):
            warnings.append(f"Exit {e_idx} target '{target}' is not in known scenes list")

    if exits_ok:
        passed.append("Exits valid")

    # 5. Objects Check (full-fidelity actor slots: JSON must be able to
    # roundtrip the C WorldActorDefinition rows -- see decompile.py)
    objects_ok = True
    objects = level_data.get("objects", [])
    seen_obj_ids = set()
    for obj in objects:
        oid = obj.get("id")
        if not oid:
            errors.append("Object missing 'id'")
            objects_ok = False
        elif oid in seen_obj_ids:
            errors.append(f"Duplicate object ID: '{oid}'")
            objects_ok = False
        else:
            seen_obj_ids.add(oid)

        pos = obj.get("position", {})
        ox = pos.get("x", -1)
        oy = pos.get("y", -1)
        if ox < 0 or ox >= width or oy < 0 or oy >= height:
            errors.append(f"Object '{oid}' position ({ox}, {oy}) is outside map bounds")
            objects_ok = False

        otype = obj.get("type", "")
        props = obj.get("properties", {})
        if not isinstance(props, dict):
            errors.append(f"Object '{oid}' properties must be an object")
            objects_ok = False
            continue
        if not props.get("entity_id"):
            # Decorative objects (e.g. the south_field campfire) have no
            # engine actor row: decompile.py preserves them verbatim and
            # compile.py ignores them. Not an error, but they cannot
            # carry actor slots.
            warnings.append(f"Object '{oid}' has no entity_id: kept verbatim, ignored by compile")
            for key in ("actor_id", "hp", "max_hp", "battle", "ai"):
                if key in props:
                    errors.append(f"Object '{oid}' has no entity_id but carries actor slot '{key}'")
                    objects_ok = False
            continue
        # Entity ids must exist in the game layer (src/game/game_ids.h
        # defines; single source of truth, parsed so the list never
        # drifts).  Unknown ids would raise at C-compile time anyway —
        # surface them here with the fix instead.
        known_entities = known_entity_ids()
        ent_id = props.get("entity_id")
        if ent_id not in known_entities:
            errors.append(
                f"Object '{oid}': unknown entity_id '{ent_id}' — add "
                f"#define {ent_id} (ENTITY_ID_FIRST_GAME + N) to src/game/game_ids.h")
            objects_ok = False
        aid = props.get("actor_id", 0)
        if not isinstance(aid, int) or aid < 0 or aid > 65535:
            errors.append(f"Object '{oid}' has invalid actor_id '{aid}' (0..65535)")
            objects_ok = False
        facing = props.get("facing", "DOWN")
        if facing not in ("UP", "DOWN", "LEFT", "RIGHT"):
            errors.append(f"Object '{oid}' has invalid facing '{facing}'")
            objects_ok = False
        for flag in props.get("flags", []):
            if flag not in ("HOSTILE", "BLOCKING", "INTERACTABLE"):
                errors.append(f"Object '{oid}' has unknown flag '{flag}'")
                objects_ok = False
        visual = props.get("visual", "?")
        if not isinstance(visual, str) or len(visual) != 1:
            errors.append(f"Object '{oid}' visual must be a single character")
            objects_ok = False
        if otype == "enemy":
            for key in ("hp", "max_hp", "ai"):
                if key not in props:
                    errors.append(f"Enemy object '{oid}' is missing properties.{key}")
                    objects_ok = False
            if "battle" not in props:
                errors.append(f"Enemy object '{oid}' is missing properties.battle")
                objects_ok = False
        for key in ("hp", "max_hp", "gold_reward"):
            if key in props and (not isinstance(props[key], int) or props[key] < 0):
                errors.append(f"Object '{oid}' has invalid {key} '{props[key]}'")
                objects_ok = False

    # Engine actor-slot caps: actor_load_banked() spawns hostile rows into
    # World.actors[MAX_WORLD_ACTORS=4] and friendly rows into
    # g_static_actors (MAX_STATIC_ACTORS, parsed from src/world/actor.h —
    # single source of truth).  Rows beyond the cap are SILENTLY DROPPED
    # at runtime -- catch the overflow at validation time instead.
    def _engine_cap(name):
        import re
        src = (Path(__file__).resolve().parent.parent.parent
               / "src" / "world" / "actor.h").read_text()
        m = re.search(r"#define %s\s+(\d+)" % name, src)
        return int(m.group(1)) if m else None

    hostile_cap = _engine_cap("MAX_WORLD_ACTORS")
    static_cap = _engine_cap("MAX_STATIC_ACTORS")
    hostile_rows = [o.get("id") for o in objects
                    if (o.get("properties") or {}).get("entity_id")
                    and "HOSTILE" in ((o.get("properties") or {}).get("flags") or [])]
    static_rows = [o.get("id") for o in objects
                   if (o.get("properties") or {}).get("entity_id")
                   and "HOSTILE" not in ((o.get("properties") or {}).get("flags") or [])]
    if hostile_cap and len(hostile_rows) > hostile_cap:
        errors.append(
            f"Level has {len(hostile_rows)} hostile actors but the engine spawns at most "
            f"MAX_WORLD_ACTORS={hostile_cap}; extra would be silently dropped: {hostile_rows[hostile_cap:]}")
        objects_ok = False
    if static_cap and len(static_rows) > static_cap:
        errors.append(
            f"Level has {len(static_rows)} friendly actors but the engine loads at most "
            f"{static_cap} static rows (MAX_STATIC_ACTORS, src/world/actor.h); extra would be "
            f"silently dropped: {static_rows[static_cap:]}")
        objects_ok = False

    if objects_ok:
        passed.append("Objects valid")

    is_valid = (len(errors) == 0)
    return is_valid, errors, warnings, passed


def main():
    if len(sys.argv) < 2:
        print("Usage: validate.py <level1.json> [level2.json ...]")
        sys.exit(1)

    tilesets = load_tilesets()
    file_paths = []
    for arg in sys.argv[1:]:
        matches = glob.glob(arg)
        if matches:
            file_paths.extend(matches)
        else:
            file_paths.append(arg)

    # Collect all level IDs first
    all_level_ids = set()
    loaded_levels = []
    for p in file_paths:
        try:
            with open(p, "r", encoding="utf-8") as f:
                data = json.load(f)
                loaded_levels.append((p, data))
                if "id" in data:
                    all_level_ids.add(data["id"])
        except Exception as e:
            print(f"Error loading {p}: {e}")
            sys.exit(1)

    overall_success = True

    # Cross-file check: nonzero actor_id values drive persistent defeat
    # tracking in GameState.world and must be unique across scenes.
    seen_actor_ids = {}
    for p, data in loaded_levels:
        for obj in data.get("objects", []):
            props = obj.get("properties", {}) or {}
            aid = props.get("actor_id", 0)
            if isinstance(aid, int) and aid != 0:
                if aid in seen_actor_ids:
                    print(f"ERROR: actor_id {aid} used by both "
                          f"'{seen_actor_ids[aid]}' and '{obj.get('id')}' (must be unique)")
                    overall_success = False
                else:
                    seen_actor_ids[aid] = obj.get("id")

    for p, data in loaded_levels:
        basename = os.path.basename(p)
        print(f"\n{basename}\n")
        valid, errors, warnings, passed = validate_level(data, tilesets, all_level_ids)

        for p_item in passed:
            print(f"✓ {p_item}")

        for w_item in warnings:
            print(f"WARNING: {w_item}")

        if errors:
            overall_success = False
            for e_item in errors:
                print(f"ERROR: {e_item}")
            print("\nLEVEL INVALID")
        else:
            print("\nLEVEL VALID")

    sys.exit(0 if overall_success else 1)


if __name__ == "__main__":
    main()

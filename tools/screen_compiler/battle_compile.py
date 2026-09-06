#!/usr/bin/env python3
"""
battle_compile.py - Battle Screen Content Compiler

Translates screens/battle/*.json and screens/enemy_types/*.json into
bank-4 C data tables that the battle screen renderer reads.

Pipeline:
    JSON Content -> validate -> emit C (src/game/battle_screens.c, src/game/battle_types.c)

Usage:
    python3 tools/screen_compiler/battle_compile.py --all -o src/game/
    python3 tools/screen_compiler/battle_compile.py --battle screens/battle/default.json --enemy-type screens/enemy_types/slime.json -o src/game/

The output is #pragma bank 4 C files that battle_screen.c reads via extern
declarations so the generated data lives in bank 4 where the renderer can
read it directly.
"""

import sys
import os
import json
import argparse
import glob
import re
from pathlib import Path

# REPO_ROOT is the repository root. When run from repo root with
# python3 tools/screen_compiler/battle_compile.py, __file__ is a relative
# path so we need to go up 3 levels: script_dir -> screen_compiler -> tools -> repo_root
REPO_ROOT = Path(__file__).resolve().parent.parent.parent
SCRIPT_DIR = Path(__file__).resolve().parent

sys.path.insert(0, str(SCRIPT_DIR))
sys.path.insert(0, str(REPO_ROOT / "tools"))

DEFAULT_OUT_DIR = str(REPO_ROOT / "src" / "game")

# Combat-art sets (screens/combat_art/*.json) replace the old hardcoded
# ART_SETS table: each set names curated sheet tiles per frame plus its
# dimensions and CGB palette.  Tile names resolve through
# compose_battle_sprites.TILE_COORDS (null = the all-white blank cell).
# The Makefile gfx rule emits battle_enemy_art.h in set-order via
# --gfx-coords, so blob offset N*tiles is stable and art set order must
# never be renumbered once committed (append new sets at the end).
from compose_battle_sprites import TILE_COORDS, BLANK_COORD
from compose_enemy_sprites import TILE_COORDS as ENEMY_TILE_COORDS
from compose_hero_sprites import TILE_COORDS as HERO_TILE_COORDS

# Shared overworld enemy OAM base (must match ENEMY_OW_BASE in src/ui/ui.h).
# Blob: concatenated per-enemy frames in sorted enemy-id order; OAM ids
# 128+ alias BG tiles, so the blob must stay below 128 (clear error below).
ENEMY_OW_BASE = 100
ENEMY_OW_LIMIT = 128

# Hero overworld is always first in the OW blob at ENEMY_OW_BASE (100).
# Hero has 1-2 frames; enemies follow after.
HERO_OW_MAX_FRAMES = 2


def load_combat_art():
    """Load and validate screens/combat_art/*.json.  Returns (sets, order,
    offsets): sets keyed by id, order = ids sorted by explicit order field,
    offsets = blob tile offset per set id."""
    sets = {}
    pattern = str(REPO_ROOT / "screens" / "combat_art" / "*.json")
    for json_file in sorted(glob.glob(pattern)):
        path = Path(json_file)
        with open(path) as f:
            data = json.load(f)
        sid = data.get('id', path.stem)
        for field in ['order', 'width', 'height', 'palette', 'frame0']:
            if field not in data:
                print("WARNING: %s: missing required field '%s'" % (path.name, field))
        w, h = data.get('width', 0), data.get('height', 0)
        if not (1 <= w <= 6):
            print("WARNING: %s: width %d must be 1-6" % (path.name, w))
        if not (1 <= h <= 4):
            print("WARNING: %s: height %d must be 1-4" % (path.name, h))
        for frame in ['frame0', 'frame1']:
            cells = data.get(frame)
            if cells is None:
                continue
            if len(cells) != w * h:
                print("WARNING: %s: %s has %d cells, need width*height=%d"
                      % (path.name, frame, len(cells), w * h))
            for name in cells:
                if name is not None and name not in TILE_COORDS:
                    print("WARNING: %s: unknown combat tile '%s'" % (path.name, name))
        if sid in sets:
            print("WARNING: %s: duplicate combat art id '%s'" % (path.name, sid))
        sets[sid] = data
    order = sorted(sets.keys(), key=lambda k: sets[k].get('order', 0))
    seen_orders = [sets[k].get('order', 0) for k in order]
    if len(set(seen_orders)) != len(seen_orders):
        print("WARNING: duplicate combat art order values %s" % seen_orders)
    # Blob offsets (tiles): frame0 cells then frame1 cells per set.
    offsets = {}
    at = 0
    for sid in order:
        offsets[sid] = at
        w, h = sets[sid]['width'], sets[sid]['height']
        at += 2 * w * h
    return sets, order, offsets


def set_frame_cells(entry, frame):
    """Resolved sheet cells for a set frame: frame1 defaults to frame0."""
    cells = entry.get(frame)
    if cells is None:
        cells = entry['frame0']
    out = []
    for name in cells:
        out.append(BLANK_COORD if name is None else TILE_COORDS[name])
    return out


# BattleScreenDef HUD tail, in struct order: (struct field, JSON key,
# default).  This list is the SINGLE source of truth for both the C
# emitter below and the check_battle_struct_order() guard: the emitter
# walks it positionally, so the generated initializer cannot drift from
# the struct without the guard failing first.  Note the remaps: the
# hud_* struct fields read legacy JSON keys (deck_row, timer_row, ...).
HUD_STRUCT_FIELDS = [
    ('turn_banner_row', 'turn_banner_row', 0),
    ('enemy_hp_row', 'enemy_hp_row', 1),
    ('enemy_sprite_row', 'enemy_sprite_row', 2),
    ('enemy_cursor_row', 'enemy_cursor_row', 4),
    ('enemy_col_start', 'enemy_col_start', 0),
    ('enemy_col_step', 'enemy_col_step', 7),
    ('hero_label_row', 'hero_label_row', 6),
    ('hero_label_col', 'hero_label_col', 1),
    ('hero_hp_row', 'hero_hp_row', 6),
    ('hero_hp_col', 'hero_hp_col', 13),
    ('deck_row', 'deck_row', 7),
    ('deck_col', 'deck_col', 1),
    ('ap_row', 'ap_row', 7),
    ('ap_col', 'ap_col', 13),
    ('combo_row', 'combo_row', 9),
    ('cards_row', 'cards_row', 10),
    ('card_cursor_row', 'card_cursor_row', 14),
    ('card_desc_row', 'card_desc_row', 15),
    ('timer_row', 'timer_row', 16),
    ('timer_col', 'timer_col', 0),
    ('timer_width', 'timer_width', 20),
    ('hud_enemy_row_start', 'enemy_row_start', 2),
    ('hud_enemy_row_step', 'enemy_row_step', 1),
    ('hud_deck_row', 'deck_row', 7),
    ('hud_combo_row_start', 'combo_row_start', 13),
    ('hud_combo_row_step', 'combo_row_step', 1),
    ('hud_timer_row', 'timer_row', 15),
    ('hud_caret_x', 'caret_x', 3),
]


def check_battle_struct_order():
    """Fail loudly if BattleScreenDef's field order in battle_data.h no
    longer matches HUD_STRUCT_FIELDS.  The emitter is positional, so a
    struct reorder without a matching list update would silently
    misassign every HUD row.  Runs on every invocation (emit, --check,
    --validate, --gfx-coords)."""
    header = REPO_ROOT / "src" / "battle" / "battle_data.h"
    try:
        text = header.read_text(encoding="utf-8")
    except FileNotFoundError:
        print("WARNING: battle_data.h not found; skipping struct-order check")
        return True
    m = re.search(r"typedef struct BattleScreenDef \{(.*?)\} BattleScreenDef;", text, re.S)
    if not m:
        print("ERROR: BattleScreenDef not found in battle_data.h", file=sys.stderr)
        return False
    names = re.findall(r"(?:uint8_t|char|int|const char \*)\s*(\w+)", m.group(1))
    try:
        tail = names[names.index('enemy_positions'):]
    except ValueError:
        print("ERROR: enemy_positions not found in BattleScreenDef", file=sys.stderr)
        return False
    expected = (['enemy_positions', 'timer_overworld_ticks', 'timer_battle_ticks']
                + [f[0] for f in HUD_STRUCT_FIELDS])
    if tail != expected:
        print("ERROR: BattleScreenDef field order drifted from HUD_STRUCT_FIELDS:", file=sys.stderr)
        print("  header: %s" % tail, file=sys.stderr)
        print("  expect: %s" % expected, file=sys.stderr)
        return False
    return True


def validate_enemy_type(path: Path, art_ids) -> dict:
    """Validate and return an enemy type JSON."""
    with open(path) as f:
        data = json.load(f)

    # Basic validation
    required = ['id', 'label', 'category', 'sprite', 'name', 'hp', 'max_hp', 'battle_id', 'gold_reward', 'reward_currency']
    for field in required:
        if field not in data:
            print("WARNING: %s: missing required field '%s'" % (path.name, field))

    # Validate category
    if data.get('category') not in ['minion', 'elite', 'boss']:
        print("WARNING: %s: invalid category '%s', must be minion/elite/boss" % (path.name, data.get('category')))

    # Validate HP ranges
    for field in ['hp', 'max_hp']:
        val = data.get(field, 0)
        if val > 255:
            print("WARNING: %s: %s %d > 255; will be truncated" % (path.name, field, val))

    # Validate strings (sprite is an object-or-null art selection, not a string)
    for field, max_len in [('label', 20), ('name', 20), ('battle_id', 30)]:
        val = data.get(field, '')
        if val is None:
            val = ''
        if len(val) > max_len:
            print(
                "WARNING: %s: %s length %d > %d; will be truncated"
                % (path.name, field, len(val), max_len)
            )

    # Validate category
    cat = data.get('category')
    if cat not in ['minion', 'elite', 'boss']:
        print("WARNING: %s: category '%s' not in [minion, elite, boss]" % (path.name, cat))

    # Validate AI types
    valid_ai = ['AI_NONE', 'AI_PATROL_CROSS', 'AI_PATROL_CIRCLE']
    for ai in data.get('ai_types', []):
        if ai not in valid_ai:
            print("WARNING: %s: invalid AI type '%s'" % (path.name, ai))

    # Validate reward currency
    currency = data.get('reward_currency', '')
    if not currency.startswith('CURRENCY_ID_'):
        print("WARNING: %s: reward_currency '%s' should start with CURRENCY_ID_" % (path.name, currency))

    # Validate battle-sprite art selection (null = text fallback)
    sprite = data.get('sprite')
    if sprite is not None:
        if not isinstance(sprite, dict) or sprite.get('art') not in art_ids:
            sys.stderr.write("WARNING: %s: sprite.art '%s' not in %s\n" % (path.name, (sprite or {}).get('art'), sorted(art_ids)))
        elif sprite.get('frames') not in (1, 2):
            sys.stderr.write("WARNING: %s: sprite.frames '%s' must be 1 or 2\n" % (path.name, sprite.get('frames')))

    return data


def validate_battle_screen(path: Path) -> dict:
    """Validate a battle screen JSON."""
    with open(path) as f:
        data = json.load(f)

    # Required fields
    required = ['id', 'label', 'max_enemies', 'allowed_categories', 'enemy_positions', 'timer_config', 'hud_layout']
    for field in required:
        if field not in data:
            print("WARNING: %s: missing required field '%s'" % (path.name, field))

    # Validate max_enemies
    max_enemies = data.get('max_enemies', 0)
    if not (1 <= max_enemies <= 3):
        print("WARNING: %s: max_enemies %d must be 1-3" % (path.name, max_enemies))

    # Validate allowed_categories
    allowed = data.get('allowed_categories', [])
    valid_cats = {'minion', 'elite', 'boss'}
    for cat in allowed:
        if cat not in valid_cats:
            print("WARNING: %s: invalid category '%s' in allowed_categories" % (path.name, cat))

    # Validate enemy_positions
    positions = data.get('enemy_positions', [])
    max_enemies = data.get('max_enemies', 0)
    if len(positions) != max_enemies:
        print("WARNING: %s: enemy_positions count %d != max_enemies %d" % (path.name, len(positions), max_enemies))
    for i, pos in enumerate(positions):
        if pos.get('x', 0) > 19:
            print("WARNING: %s: position %d x > 19" % (path.name, i))
        if pos.get('y', 0) > 17:
            print("WARNING: %s: position %d y > 17" % (path.name, i))

    # Validate timer_config
    timer = data.get('timer_config', {})
    for field, max_val in [('overworld_ticks', 255), ('battle_ticks', 255)]:
        val = timer.get(field, 0)
        if val > max_val:
            print("WARNING: %s: timer_config.%s %d > %d" % (path.name, field, val, max_val))

    # Validate hud_layout
    hud = data.get('hud_layout', {})
    for key in ['enemy_row_start', 'enemy_row_step', 'deck_row', 'combo_row_start', 'combo_row_step', 'timer_row', 'caret_x']:
        val = hud.get(key, 0)
        if val > 17:
            print("WARNING: %s: hud_layout.%s %d > 17" % (path.name, key, val))

    return data


def c_escape(s):
    """Escape a string for safe inclusion in a C string literal."""
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


def build_battle_screens_output(battle_screens, enemy_types):
    """Generate C code for battle screen definitions."""
    lines = []

    lines.append("/**")
    lines.append(" * Generated by tools/screen_compiler/battle_compile.py.")
    lines.append(" * Do not edit directly -- edit screens/battle/ and re-run.")
    lines.append(" */")
    lines.append("")
    lines.append("#pragma bank 4")
    lines.append("")
    lines.append("#include <stdint.h>")
    lines.append('#include "battle_data.h"')
    lines.append("")

    # Category bitmasks
    cat_bits = {'minion': 1, 'elite': 2, 'boss': 4}

    screen_ids = sorted(battle_screens.keys())
    for screen_id in screen_ids:
        screen = battle_screens[screen_id]
        allowed = screen.get('allowed_categories', [])
        cat_mask = sum(cat_bits.get(c, 0) for c in allowed)
        positions = screen.get('enemy_positions', [])
        timer = screen.get('timer_config', {})
        hud = screen.get('hud_layout', {})

        lines.append("static const BattleScreenDef g_battle_screen_%s = {" % screen['id'])
        lines.append('    %s,' % c_escape(screen['id']))
        lines.append('    %s,' % c_escape(screen['label']))
        lines.append("    %d," % screen['max_enemies'])
        lines.append("    %d," % cat_mask)

        # Enemy positions
        lines.append("    {")
        for pos in positions:
            lines.append("        { %d, %d }," % (pos['x'], pos['y']))
        # Pad to 3 entries
        for _ in range(len(positions), 3):
            lines.append("        { 0, 0 },")
        lines.append("    },")

        lines.append("    %d," % timer.get('overworld_ticks', 43))
        lines.append("    %d," % timer.get('battle_ticks', 17))
        # HUD tail: positional, in HUD_STRUCT_FIELDS order (guarded by
        # check_battle_struct_order(); the last field omits the comma).
        for pos, (field, key, default) in enumerate(HUD_STRUCT_FIELDS):
            if pos < len(HUD_STRUCT_FIELDS) - 1:
                lines.append("    %d," % hud.get(key, default))
            else:
                lines.append("    %d" % hud.get(key, default))
        lines.append("};")
        lines.append("")

    # Battle screen array
    lines.append("const BattleScreenDef* const g_battle_screens[%d] = {" % len(screen_ids))
    for sid in screen_ids:
        lines.append("    &g_battle_screen_%s," % sid)
    lines.append("};")
    lines.append("const uint8_t g_battle_screen_count = %d;" % len(screen_ids))
    lines.append("")

    return "\n".join(lines)


def ow_blob_layout(enemy_types, hero_json=None):
    """Shared overworld blob layout: returns (offsets, cells) where offsets
    maps id -> blob tile offset and cells is the ordered tile-name
    list (hero first, then sorted enemy-id order, append-only stability).  
    Returns (None, None) on budget overflow (error already printed)."""
    # Hero always comes first
    offsets = {}
    cells = []
    at = 0
    
    if hero_json is not None:
        hero_ow = hero_json.get('overworld') or None
        if hero_ow is not None:
            names = hero_ow.get('cells', [])
            if not (1 <= len(names) <= 2):
                print("WARNING: hero: overworld.cells has %d entries, need 1-2" % len(names))
            for name in names:
                if name not in HERO_TILE_COORDS:
                    print("WARNING: hero: unknown overworld tile '%s'" % name)
            pal = hero_json['overworld'].get('palette', 0)
            if not (0 <= pal <= 7):
                print("WARNING: hero: overworld.palette %s out of 0-7" % pal)
            offsets['hero'] = at
            cells.extend(names)
            at += len(names)
    
    # Then enemies in sorted order
    et_ids = sorted(enemy_types.keys())
    for et_id in et_ids:
        ow = (enemy_types[et_id].get('overworld') or None)
        if ow is None:
            continue
        names = ow.get('cells', [])
        if not (1 <= len(names) <= 2):
            print("WARNING: %s: overworld.cells has %d entries, need 1-2" % (et_id, len(names)))
        for name in names:
            if name not in ENEMY_TILE_COORDS:
                print("WARNING: %s: unknown overworld tile '%s'" % (et_id, name))
        pal = ow.get('palette', 0)
        if not (0 <= pal <= 7):
            print("WARNING: %s: overworld.palette %s out of 0-7" % (et_id, pal))
        offsets[et_id] = at
        cells.extend(names)
        at += len(names)
    if ENEMY_OW_BASE + at > ENEMY_OW_LIMIT:
        print("ERROR: overworld blob needs %d OAM tiles, budget is %d (base %d, limit %d)"
              % (at, ENEMY_OW_LIMIT - ENEMY_OW_BASE, ENEMY_OW_BASE, ENEMY_OW_LIMIT), file=sys.stderr)
        return None, None
    return offsets, cells


def build_enemy_types_output(enemy_types, art_sets, art_order, art_offsets, hero_json=None):
    """Generate C code for enemy type definitions + hero data."""
    lines = []

    lines.append("/**")
    lines.append(" * Generated by tools/screen_compiler/battle_compile.py.")
    lines.append(" * Do not edit directly -- edit screens/enemy_types/ and re-run.")
    lines.append(" */")
    lines.append("")
    lines.append("#pragma bank 4")
    lines.append("")
    lines.append("#include <stdint.h>")
    lines.append('#include "battle_data.h"')
    lines.append('#include "game_ids.h"')
    lines.append("")

    # Hero data (first in OW blob)
    lines.append("/* Hero overworld sprite data (shared, type-owned) */")
    if hero_json is not None:
        hero_ow = hero_json.get('overworld') or None
        if hero_ow is not None:
            names = hero_ow.get('cells', [])
            pal = hero_json['overworld'].get('palette', 0)
            lines.append("const uint8_t g_hero_ow_tile = %d;" % ENEMY_OW_BASE)
            lines.append("const uint8_t g_hero_ow_frames = %d;" % len(hero_ow.get('cells', [])))
            lines.append("const uint8_t g_hero_ow_palette = %d;" % hero_json['overworld'].get('palette', 0))
        else:
            lines.append("const uint8_t g_hero_ow_tile = 0xFF;")
            lines.append("const uint8_t g_hero_ow_frames = 0;")
            lines.append("const uint8_t g_hero_ow_palette = 0;")
    else:
        lines.append("const uint8_t g_hero_ow_tile = 0xFF;")
        lines.append("const uint8_t g_hero_ow_frames = 0;")
        lines.append("const uint8_t g_hero_ow_palette = 0;")
    lines.append("")

    # Enemy type definitions
    cat_map = {'minion': 0, 'elite': 1, 'boss': 2}

    et_ids = sorted(enemy_types.keys())
    ow_offsets, _ow_cells = ow_blob_layout(enemy_types, None)  # hero handled separately
    if ow_offsets is None:
        return None
    for et_id in et_ids:
        ow = (enemy_types[et_id].get('overworld') or None)
        if ow is not None and et_id in ow_offsets:
            ow_tile = ENEMY_OW_BASE + ow_offsets[et_id]
            ow_frames = len(ow.get('cells', []))
            ow_palette = ow.get('palette', 0)
        else:
            ow_tile = 0xFF
            ow_frames = 0
            ow_palette = 0
        et = enemy_types[et_id]
        sprite = et.get('sprite') or {}
        art_id = sprite.get('art')
        art_index = 0xFF  # text fallback: no battle art
        art_palette = 0
        art_w = 0
        art_h = 0
        art_offset = 0
        if art_id in art_sets:
            art_index = art_order.index(art_id)
            art_palette = art_sets[art_id]['palette']
            art_w = art_sets[art_id]['width']
            art_h = art_sets[art_id]['height']
            art_offset = art_offsets[art_id]
        elif art_id is not None:
            print("WARNING: %s: sprite.art '%s' has no combat art set" % (et_id, art_id))
        art_frames = sprite.get('frames', 0) if art_index != 0xFF else 0
        lines.append("static const EnemyTypeDef g_enemy_type_%s = {" % et['id'])
        lines.append('    %s,' % c_escape(et['id']))
        lines.append('    %s,' % c_escape(et['label']))
        lines.append('    %d,' % cat_map.get(et['category'], 0))
        lines.append('    %s,' % c_escape(et['name']))
        lines.append('    %d,' % et['hp'])
        lines.append('    %d,' % et['max_hp'])
        lines.append('    %s,' % c_escape(et['battle_id']))
        lines.append('    %d,' % et['gold_reward'])
        lines.append('    %s,' % et['reward_currency'])
        lines.append('    %d,' % art_index)
        lines.append('    %d,' % art_frames)
        lines.append('    %d,' % art_palette)
        lines.append('    %d,' % art_w)
        lines.append('    %d,' % art_h)
        lines.append('    %d,' % art_offset)
        lines.append('    %d,' % ow_tile)
        lines.append('    %d,' % ow_frames)
        lines.append('    %d' % ow_palette)
        lines.append("};")
        lines.append("")

    # Enemy type array
    lines.append("const EnemyTypeDef* const g_enemy_types[%d] = {" % len(et_ids))
    for eid in et_ids:
        lines.append("    &g_enemy_type_%s," % eid)
    lines.append("};")
    lines.append("const uint8_t g_enemy_type_count = %d;" % len(et_ids))
    lines.append("")
    # Shared overworld blob size (tiles) for the ui_init OAM stream.
    # Includes hero tiles + enemy tiles
    _ow_hero = 0
    if hero_json is not None:
        hero_ow = hero_json.get('overworld') or None
        if hero_ow is not None:
            _ow_hero = len(hero_ow.get('cells', []))
    _ow_enemy = len(_ow_cells) if _ow_cells is not None else 0
    _ow_total = _ow_enemy + _ow_hero
    lines.append("const uint8_t g_enemy_ow_tile_count = %d;" % _ow_total)
    lines.append("")

    return "\n".join(lines)


def main(args=None):
    parser = argparse.ArgumentParser(description="Battle screen content compiler")
    parser.add_argument("--all", action="store_true",
                        help="Compile all battle screens and enemy types from screens/battle/ and screens/enemy_types/")
    parser.add_argument("--battle", action="append", default=[],
                        help="Specific battle screen JSON to compile (can repeat)")
    parser.add_argument("--enemy-type", action="append", default=[],
                        help="Specific enemy type JSON to compile (can repeat)")
    parser.add_argument("-o", "--output", default=None,
                        help="Output directory (default: src/game/)")
    parser.add_argument("--validate", action="store_true",
                        help="Only validate JSON, don't emit C")
    parser.add_argument("--check", action="store_true",
                        help="Do not write; exit nonzero if fresh output differs from the files")
    parser.add_argument("--gfx-coords", action="store_true",
                        help="Print the png2gb --tile-coords string for battle_enemy_art.h (set order, frame0 then frame1 per set) and exit")
    parser.add_argument("--ow-coords", action="store_true",
                        help="Print the png2gb --tile-coords string for enemy_ow_tiles.h (sorted enemy-id order) and exit")

    args = parser.parse_args(args)

    # Combat art sets back every enemy-type sprite.art reference.
    art_sets, art_order, art_offsets = load_combat_art()
    # The emitter is positional: refuse to run on a drifted struct.
    if not check_battle_struct_order():
        return 1

    # Load hero.json for OW blob
    hero_json = None
    hero_path = REPO_ROOT / "screens" / "hero.json"
    if hero_json is None and hero_path.exists():
        with open(hero_path) as f:
            hero_json = json.load(f)

    if args.gfx_coords:
        coords = []
        for sid in art_order:
            for frame in ['frame0', 'frame1']:
                for (x, y) in set_frame_cells(art_sets[sid], frame):
                    coords.append("%d,%d" % (x, y))
        print(" ".join(coords))
        return 0

    output_dir = Path(args.output) if args.output else REPO_ROOT / "src" / "game"
    output_dir.mkdir(parents=True, exist_ok=True)

    # Load all data
    enemy_types = {}
    battle_screens = {}

    if args.all or (not args.battle and not args.enemy_type):
        # Load all enemy types
        for json_file in sorted(glob.glob(str(REPO_ROOT / "screens" / "enemy_types" / "*.json"))):
            path = Path(json_file)
            data = validate_enemy_type(path, art_sets)
            enemy_types[data['id']] = data

        # Load hero.json
        hero_path = REPO_ROOT / "screens" / "hero.json"
        hero_json = None
        if hero_path.exists():
            with open(hero_path) as f:
                hero_json = json.load(f)
            # Validate hero.json
            required = ['id', 'name', 'start_hp', 'start_gold', 'starter_deck', 'overworld']
            for field in required:
                if field not in hero_json:
                    print("WARNING: hero.json: missing required field '%s'" % field)
            if hero_json.get('id') != 'hero':
                print("WARNING: hero.json: id must be 'hero'")
            if 'starter_deck' in hero_json:
                if len(hero_json['starter_deck']) > 20:
                    print("WARNING: hero.json: starter_deck has %d cards, max 20" % len(hero_json['starter_deck']))
                for card in hero_json['starter_deck']:
                    if not card.startswith('CARD_'):
                        print("WARNING: hero.json: invalid card id '%s' (must start with CARD_)" % card)
            if 'overworld' in hero_json:
                ow = hero_json['overworld']
                if not (1 <= len(ow.get('cells', [])) <= 2):
                    print("WARNING: hero.json: overworld.cells has %d entries, need 1-2" % len(ow.get('cells', [])))
                for name in ow.get('cells', []):
                    if name not in HERO_TILE_COORDS:
                        print("WARNING: hero.json: unknown overworld tile '%s'" % name)
                pal = ow.get('palette', 0)
                if not (0 <= pal <= 7):
                    print("WARNING: hero.json: overworld.palette %s out of 0-7" % pal)
        else:
            print("WARNING: screens/hero.json not found")

        # Load all battle screens
        for json_file in sorted(glob.glob(str(REPO_ROOT / "screens" / "battle" / "*.json"))):
            path = Path(json_file)
            data = validate_battle_screen(path)
            battle_screens[data['id']] = data
    else:
        # Load specific files
        for json_file in args.battle:
            path = Path(json_file)
            data = validate_battle_screen(path)
            battle_screens[data['id']] = data
        for json_file in args.enemy_type:
            path = Path(json_file)
            data = validate_enemy_type(path, art_sets)
            enemy_types[data['id']] = data

    if not enemy_types:
        print("WARNING: No enemy types loaded")
    if not battle_screens:
        print("WARNING: No battle screens loaded")

    if args.validate:
        print("JSON validation passed (%d battle screen(s), %d enemy type(s))"
              % (len(battle_screens), len(enemy_types)))
        return 0

    if args.ow_coords:
        _offsets, names = ow_blob_layout(enemy_types, hero_json)
        if names is None:
            return 1
        # Hero tiles use HERO_TILE_COORDS, enemy tiles use ENEMY_TILE_COORDS
        coords = []
        hero_cells = []
        if hero_json is not None:
            hero_ow = hero_json.get('overworld') or None
            if hero_ow is not None:
                hero_cells = hero_json['overworld'].get('cells', [])
                for n in hero_cells:
                    coords.append("%d,%d" % HERO_TILE_COORDS[n])
        # Filter out hero cells from names list
        for n in names:
            if n not in hero_cells:
                coords.append("%d,%d" % ENEMY_TILE_COORDS[n])
        print(" ".join(coords))
        return 0

# Generate outputs
    battle_screens_output = build_battle_screens_output(battle_screens, {})
    enemy_types_output = build_enemy_types_output(enemy_types, art_sets, art_order, art_offsets, hero_json)
    if enemy_types_output is None:
        return 1

    # Write battle_screens.c
    battle_screens_path = output_dir / "battle_screens.c"
    # Write battle_types.c
    battle_types_path = output_dir / "battle_types.c"

    if args.check:
        for path, fresh in ((battle_screens_path, battle_screens_output),
                            (battle_types_path, enemy_types_output)):
            try:
                committed = path.read_text(encoding="utf-8")
            except FileNotFoundError:
                committed = None
            if committed is None or committed != fresh:
                print("DRIFT: fresh compile differs from %s" % path, file=sys.stderr)
                return 1
        print("battle compile --check OK: %s and %s match fresh output"
              % (battle_screens_path, battle_types_path))
        return 0

    with open(battle_screens_path, "w") as f:
        f.write(battle_screens_output)
    print("Wrote %s" % battle_screens_path)

    with open(battle_types_path, "w") as f:
        f.write(enemy_types_output)
    print("Wrote %s" % battle_types_path)

    return 0


if __name__ == "__main__":
    sys.exit(main())
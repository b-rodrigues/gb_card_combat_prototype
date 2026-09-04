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
from pathlib import Path

# REPO_ROOT is the repository root. When run from repo root with
# python3 tools/screen_compiler/battle_compile.py, __file__ is a relative
# path so we need to go up 3 levels: script_dir -> screen_compiler -> tools -> repo_root
REPO_ROOT = Path(__file__).resolve().parent.parent.parent
SCRIPT_DIR = Path(__file__).resolve().parent

sys.path.insert(0, str(SCRIPT_DIR))

DEFAULT_OUT_DIR = str(REPO_ROOT / "src" / "game")

# Battle-sprite art sets.  Each set is 12 sheet cells (2 frames x 3x2) into
# assets/battle_sprites.png (3 cols x 8 rows; see tools/compose_battle_sprites.py):
# frame 0 = cells[0:6], frame 1 = cells[6:12].  Single-frame sets repeat
# frame 0.  The Makefile gfx rule emits the header in this same order, so
# art set N lives at tile offset N*12 in battle_enemy_art.h.
# BLANK is the all-white cell that pads 3x1 art (bat) to 3x2 slots.
# Each set also names its CGB battle palette (ui_color_* indices in ui.h):
# slime = poison emerald, bat = dim gray, boss = fire red.  DMG hardware
# ignores attributes and falls back to grayscale via BGP 0xE4.
BLANK = (0, 7)
ART_SETS = {
    "slime": ([(0, 0), (1, 0), (2, 0), (0, 1), (1, 1), (2, 1)],
              [(0, 2), (1, 2), (2, 2), (0, 1), (1, 1), (2, 1)], 4),
    "bat": ([(0, 3), (1, 3), (2, 3), BLANK, BLANK, BLANK],
            [(0, 4), (1, 4), (2, 4), BLANK, BLANK, BLANK], 7),
    "boss": ([(0, 5), (1, 5), (2, 5), (0, 6), (1, 6), (2, 6)],
             [(0, 5), (1, 5), (2, 5), (0, 6), (1, 6), (2, 6)], 1),
}
ART_ORDER = ["slime", "bat", "boss"]
ART_CELLS_PER_SET = 12


def validate_enemy_type(path: Path) -> dict:
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
        if not isinstance(sprite, dict) or sprite.get('art') not in ART_SETS:
            print("WARNING: %s: sprite.art '%s' not in %s" % (path.name, (sprite or {}).get('art'), sorted(ART_SETS)))
        elif sprite.get('frames') not in (1, 2):
            print("WARNING: %s: sprite.frames '%s' must be 1 or 2" % (path.name, sprite.get('frames')))

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
        lines.append("    %d," % hud.get('turn_banner_row', 0))
        lines.append("    %d," % hud.get('enemy_hp_row', 1))
        lines.append("    %d," % hud.get('enemy_sprite_row', 2))
        lines.append("    %d," % hud.get('enemy_cursor_row', 4))
        lines.append("    %d," % hud.get('enemy_col_start', 0))
        lines.append("    %d," % hud.get('enemy_col_step', 7))
        lines.append("    %d," % hud.get('hero_label_row', 6))
        lines.append("    %d," % hud.get('hero_label_col', 1))
        lines.append("    %d," % hud.get('hero_hp_row', 6))
        lines.append("    %d," % hud.get('hero_hp_col', 13))
        lines.append("    %d," % hud.get('deck_row', 7))
        lines.append("    %d," % hud.get('deck_col', 1))
        lines.append("    %d," % hud.get('ap_row', 7))
        lines.append("    %d," % hud.get('ap_col', 13))
        lines.append("    %d," % hud.get('combo_row', 9))
        lines.append("    %d," % hud.get('cards_row', 10))
        lines.append("    %d," % hud.get('card_cursor_row', 14))
        lines.append("    %d," % hud.get('card_desc_row', 15))
        lines.append("    %d," % hud.get('timer_row', 16))
        lines.append("    %d," % hud.get('timer_col', 0))
        lines.append("    %d," % hud.get('timer_width', 20))
        lines.append("    %d," % hud.get('enemy_row_start', 2))
        lines.append("    %d," % hud.get('enemy_row_step', 1))
        lines.append("    %d," % hud.get('deck_row', 7))
        lines.append("    %d," % hud.get('combo_row_start', 13))
        lines.append("    %d," % hud.get('combo_row_step', 1))
        lines.append("    %d," % hud.get('timer_row', 15))
        lines.append("    %d" % hud.get('caret_x', 3))
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


def build_enemy_types_output(enemy_types):
    """Generate C code for enemy type definitions."""
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

    # Enemy type definitions
    cat_map = {'minion': 0, 'elite': 1, 'boss': 2}

    et_ids = sorted(enemy_types.keys())
    for et_id in et_ids:
        et = enemy_types[et_id]
        sprite = et.get('sprite') or {}
        art_id = sprite.get('art')
        try:
            art_index = ART_ORDER.index(art_id)
            art_palette = ART_SETS[art_id][2]
        except ValueError:
            art_index = 0xFF  # text fallback: no battle art
            art_palette = 0
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
        lines.append('    %d' % art_palette)
        lines.append("};")
        lines.append("")

    # Enemy type array
    lines.append("const EnemyTypeDef* const g_enemy_types[%d] = {" % len(et_ids))
    for eid in et_ids:
        lines.append("    &g_enemy_type_%s," % eid)
    lines.append("};")
    lines.append("const uint8_t g_enemy_type_count = %d;" % len(et_ids))
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

    args = parser.parse_args(args)

    output_dir = Path(args.output) if args.output else REPO_ROOT / "src" / "game"
    output_dir.mkdir(parents=True, exist_ok=True)

    # Load all data
    enemy_types = {}
    battle_screens = {}

    if args.all or (not args.battle and not args.enemy_type):
        # Load all enemy types
        for json_file in sorted(glob.glob(str(REPO_ROOT / "screens" / "enemy_types" / "*.json"))):
            path = Path(json_file)
            data = validate_enemy_type(path)
            enemy_types[data['id']] = data

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
            data = validate_enemy_type(path)
            enemy_types[data['id']] = data

    if not enemy_types:
        print("WARNING: No enemy types loaded")
    if not battle_screens:
        print("WARNING: No battle screens loaded")

    if args.validate:
        print("JSON validation passed (%d battle screen(s), %d enemy type(s))"
              % (len(battle_screens), len(enemy_types)))
        return 0

    # Generate outputs
    battle_screens_output = build_battle_screens_output(battle_screens, {})
    enemy_types_output = build_enemy_types_output(enemy_types)

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
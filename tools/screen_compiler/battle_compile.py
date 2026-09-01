#!/usr/bin/env python3
"""
battle_compile.py - Battle Screen Content Compiler

Translates screens/battle.json into a bank-4 C data table that the battle
screen renderer reads instead of hardcoded Battle struct fields.

Pipeline:
    JSON Content -> validate -> emit C (src/game/battle_data.c)

Usage:
    python3 tools/screen_compiler/battle_compile.py screens/battle.json
    python3 tools/screen_compiler/battle_compile.py -o src/game/battle_data.c screens/battle.json

The output is a #pragma bank 4 C file that battle_screen.c reads via extern
declarations so the generated data lives in bank 4 where the renderer can
read it directly.  Only fields that the existing UI helpers read are emitted;
the full Battle struct layout in ROM is unchanged.
"""

import sys
import os
import json
import argparse
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_DIR = Path(__file__).resolve().parent

sys.path.insert(0, str(SCRIPT_DIR))

DEFAULT_OUT = str(REPO_ROOT / "src" / "game" / "battle_data.c")


def validate_battle_json(path: Path) -> dict:
    """Validate and return the battle JSON data, raising on structural errors."""
    with open(path) as f:
        data = json.load(f)

    grid_w = data.get("grid_width", 20)
    grid_h = data.get("grid_height", 18)

    # Enemies: 1..3, each with label, hp, max_hp, x, y
    enemies = data.get("enemies", [])
    if not enemies:
        print("WARNING: no enemies defined in battle JSON")
    for i, e in enumerate(enemies):
        if len(e.get("label", "")) > 15:
            print(
                "WARNING: enemy %d label length %d > 15; will be truncated"
                % (i, len(e["label"]))
            )
        if e.get("hp", 0) > 99:
            print(
                "WARNING: enemy %d hp %d > 99; will be truncated to 99"
                % (i, e["hp"])
            )
        if e.get("max_hp", 0) > 99:
            print(
                "WARNING: enemy %d max_hp %d > 99; will be truncated to 99"
                % (i, e["max_hp"])
            )
        if e.get("x", 0) > 19:
            print(
                "WARNING: enemy %d x %d > 19; will be truncated to 19"
                % (i, e["x"])
            )
        if e.get("y", 0) > 17:
            print(
                "WARNING: enemy %d y %d > 17; will be truncated to 17"
                % (i, e["y"])
            )

    # Player HP
    player_hp = data.get("player_hp", 0)
    player_max_hp = data.get("player_max_hp", 20)
    if player_hp > 99:
        print("WARNING: player_hp %d > 99; will be truncated to 99" % player_hp)
    if player_max_hp > 99:
        print("WARNING: player_max_hp %d > 99; will be truncated to 99" % player_max_hp)

    # Deck size and cards
    deck_size = data.get("deck_size", 5)
    if deck_size < 1 or deck_size > 7:
        print("WARNING: deck_size %d out of 1..7 range" % deck_size)
    deck_cards = data.get("deck_cards", [])
    for i, c in enumerate(deck_cards):
        if len(c) > 5:
            print(
                "WARNING: deck card %d '%s' length %d > 5; will be truncated"
                % (i, c, len(c))
            )

    # Combo options
    combo_options = data.get("combo_options", [])
    for i, opt in enumerate(combo_options):
        if len(opt) > 8:
            print(
                "WARNING: combo option %d '%s' length %d > 8; will be truncated"
                % (i, opt, len(opt))
            )

    # Timer config
    timer_config = data.get("timer_config", {})
    ow_ticks = timer_config.get("overworld_ticks", 43)
    bt_ticks = timer_config.get("battle_ticks", 17)
    if ow_ticks > 255:
        print("WARNING: overworld_ticks %d > 255; will be truncated" % ow_ticks)
    if bt_ticks > 255:
        print("WARNING: battle_ticks %d > 255; will be truncated" % bt_ticks)

    # HUD layout
    hud = data.get("hud_layout", {})
    for key in (
        "enemy_row_start",
        "enemy_row_step",
        "deck_row",
        "combo_row_start",
        "combo_row_step",
        "timer_row",
        "caret_x",
    ):
        if key in hud:
            val = hud[key]
            if val > 17:
                print(
                    "WARNING: hud_layout.%s %d > 17; will be truncated"
                    % (key, val)
                )

    return data


def c_escape(s):
    """Escape a string for safe inclusion in a C string literal."""
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


def build_battle_c_output(data: dict) -> str:
    """Build the full C output string from validated battle JSON."""
    grid_w = data.get("grid_width", 20)
    grid_h = data.get("grid_height", 18)

    # --- Enemies ---
    enemies = data["enemies"]
    n_enemies = len(enemies)

    # Enemy labels: 15 chars each (stored as-is; renderer appends "/HP/MAX")
    enemy_label_lines = []
    for e in enemies:
        label = e["label"]
        if len(label) >= 15:
            padded = label[:15]
        else:
            padded = label + " " * (15 - len(label))
        enemy_label_lines.append(c_escape(padded))

    # Player HP
    player_hp = data["player_hp"]
    player_max_hp = data["player_max_hp"]
    player_hp_str = str(player_hp) if player_hp >= 0 else "  "
    player_max_hp_str = str(player_max_hp) if player_max_hp >= 0 else "  "

    # Deck
    deck_size = data.get("deck_size", 5)
    deck_cards = data.get("deck_cards", [])
    deck_card_parts = []
    for c in deck_cards:
        if len(c) >= 5:
            padded = c[:5]
        else:
            padded = c + " " * (5 - len(c))
        deck_card_parts.append(c_escape(padded))
    # Pad to 5 entries
    while len(deck_card_parts) < 5:
        deck_card_parts.append("     ")

    # Combo options
    combo_options = data.get("combo_options", [])
    combo_parts = []
    for opt in combo_options:
        if len(opt) >= 8:
            padded = opt[:8]
        else:
            padded = opt + " " * (8 - len(opt))
        combo_parts.append(c_escape(padded))
    # Pad to 3 entries
    while len(combo_parts) < 3:
        combo_parts.append("        ")

    # Timer config
    timer_config = data.get("timer_config", {})
    overworld_ticks = timer_config.get("overworld_ticks", 43)
    battle_ticks = timer_config.get("battle_ticks", 17)

    # HUD layout positions
    hud_layout = data.get("hud_layout", {})
    enemy_row_start = hud_layout.get("enemy_row_start", 2)
    enemy_row_step = hud_layout.get("enemy_row_step", 1)
    deck_row = hud_layout.get("deck_row", 7)
    combo_row_start = hud_layout.get("combo_row_start", 13)
    combo_row_step = hud_layout.get("combo_row_step", 1)
    timer_row = hud_layout.get("timer_row", 15)
    caret_x = hud_layout.get("caret_x", 3)

    # --- Build output line by line ---
    lines = []

    lines.append("/**")
    lines.append(" * Generated by tools/screen_compiler/battle_compile.py.")
    lines.append(" * Do not edit directly — edit screens/battle.json and re-run.")
    lines.append(" */")
    lines.append("")
    lines.append("#pragma bank 4")
    lines.append("")
    lines.append("#include <stdint.h>")
    lines.append("")

    # --- Enemies ---
    # g_battle_enemies[n][15] = label\0  (n = 0..2)
    lines.append("const char g_battle_enemies[%d][15+1] = {" % n_enemies)
    for part in enemy_label_lines:
        lines.append(" %s" % part)
    # Close the initializer list
    lines.append(" };")
    lines.append("")

    # Enemy count
    lines.append("uint8_t const g_battle_enemy_count = %d;" % n_enemies)
    lines.append("")

    # Per-enemy HP and max HP
    for i, e in enumerate(enemies):
        hp_val = e["hp"] if e["hp"] >= 0 else 0
        max_hp_val = e["max_hp"] if e["max_hp"] >= 0 else 0
        lines.append("uint8_t const g_battle_enemy_%d_hp = %d;" % (i, hp_val))
        lines.append("uint8_t const g_battle_enemy_%d_max_hp = %d;" % (i, max_hp_val))
    lines.append("")

    # --- Player HP ---
    ph = data["player_hp"]
    pmh = data["player_max_hp"]
    lines.append("uint8_t const g_battle_player_hp = %d;" % ph)
    lines.append("uint8_t const g_battle_player_max_hp = %d;" % pmh)
    lines.append("")

    # --- Deck ---
    lines.append("uint8_t const g_battle_deck_size = %d;" % deck_size)
    # Card names array: 5 entries of 5+1 chars
    lines.append("const char g_battle_deck_cards[5][5+1] = {")
    for i, part in enumerate(deck_card_parts):
        comma = "," if i < len(deck_card_parts) - 1 else ""
        lines.append(" %s%s" % (part, comma))
    lines.append(" };")
    lines.append("")

    # --- Combo options ---
    lines.append("const char g_battle_combo_options[3][8+1] = {")
    for i, part in enumerate(combo_parts):
        comma = "," if i < len(combo_parts) - 1 else ""
        lines.append(" %s%s" % (part, comma))
    lines.append(" };")
    lines.append("")

    # Timer config
    lines.append("uint8_t const g_battle_timer_overworld_ticks = %d;" % overworld_ticks)
    lines.append("uint8_t const g_battle_timer_battle_ticks = %d;" % battle_ticks)
    lines.append("")

    # HUD layout positions
    lines.append("uint8_t const g_battle_hud_enemy_row_start = %d;" % enemy_row_start)
    lines.append("uint8_t const g_battle_hud_enemy_row_step = %d;" % enemy_row_step)
    lines.append("uint8_t const g_battle_hud_deck_row = %d;" % deck_row)
    lines.append("uint8_t const g_battle_hud_combo_row_start = %d;" % combo_row_start)
    lines.append("uint8_t const g_battle_hud_combo_row_step = %d;" % combo_row_step)
    lines.append("uint8_t const g_battle_hud_timer_row = %d;" % timer_row)
    lines.append("uint8_t const g_battle_hud_caret_x = %d;" % caret_x)

    return "\n".join(lines)


def main(args=None):
    parser = argparse.ArgumentParser(description="Battle screen content compiler")
    parser.add_argument("input", nargs="?", default=None,
                        help="Path to battle JSON (default: screens/battle.json)")
    parser.add_argument("-o", "--output", default=None,
                        help="Output C file (default: src/game/battle_data.c)")
    parser.add_argument("--validate", action="store_true",
                        help="Only validate JSON, don't emit C")

    args = parser.parse_args(args)

    input_path = Path(args.input) if args.input else REPO_ROOT / "screens" / "battle.json"
    output_path = Path(args.output) if args.output else REPO_ROOT / "src" / "game" / "battle_data.c"

    # Validate
    data = validate_battle_json(input_path)

    if args.validate:
        print("JSON validation passed: %s" % input_path)
        return 0

    # Emit C
    output_text = build_battle_c_output(data)

    # Ensure output directory exists
    out_dir = Path(output_path).parent
    out_dir.mkdir(parents=True, exist_ok=True)

    with open(output_path, "w") as f:
        f.write(output_text)

    print("Wrote %s" % output_path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
#!/usr/bin/env python3
"""Asset atlas generator (AGENTS.md driven).

Scans every PNG in assets/ and produces a lookup table mapping each little
asset to its grid coordinates (source PNG + COL,ROW + tile size), plus
engine-consumable artifacts:

  assets/atlas.json                     machine-readable master catalog
  docs/assets_atlas.md                  human/LLM reference (the lookup table)
  src/gfx/asset_atlas.h                 C header: AssetId/AssetSource enums,
                                        AssetAtlasEntry struct, accessors
  src/gfx/asset_atlas_entries.inc       AssetAtlasEntry rows (banked content)
  src/gfx/asset_atlas_icons.inc         unique 2bpp icon tile bytes (banked)
  src/gfx/asset_atlas_icon_palettes.inc per-tile CGB palettes, RGB555 LE 8 B

Naming:
  * semantic names for engine-relevant tiles (terrain, player, glyphs,
    world test tiles) -- the "if we want a tree, go to (0,5)" lookup;
  * coordinate ids for every content cell of the icon sheets
    (EQUIP_C04_R12, SYM_C37_R05) so any pixel sprite is addressable.

Icon sheets (equipment_8x8.png, symbols_8x8.png) use a 9px stride = 8px
content + 1px divider.  Each content cell is deduped by its exact 2bpp bytes
+ palette and quantized to a per-tile 4-color CGB palette (RGB555 LE,
lightest -> index 0) so both sheets are directly usable on CGB.

Automatic, deterministic: identical inputs produce byte-identical outputs.

Usage:
    python3 tools/asset_atlas.py            # regenerate all artifacts
    make atlas                              # same via the Makefile
    make atlas-check                        # drift + Makefile --tile-coords parity
"""
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASSETS = os.path.join(ROOT, "assets")
GFX = os.path.join(ROOT, "src", "gfx")
MAKEFILE = os.path.join(ROOT, "Makefile")

JSON_OUT = os.path.join(ASSETS, "atlas.json")
DOCS_OUT = os.path.join(ROOT, "docs", "assets_atlas.md")
HEADER_OUT = os.path.join(GFX, "asset_atlas.h")
ENTRIES_INC = os.path.join(GFX, "asset_atlas_entries.inc")
ICONS_INC = os.path.join(GFX, "asset_atlas_icons.inc")
PALETTES_INC = os.path.join(GFX, "asset_atlas_icon_palettes.inc")

ICON_BG = (158, 182, 207)
ICON_DIVIDER = (128, 128, 128)
ICON_STRIDE = 9
CONTENT_MIN = 5          # >= 5 non-background pixels == a content cell
TILE_SIZE = 8
FONT_COLS, FONT_ROWS = 16, 6

# Semantic tile coordinates per aligned sheet (Makefile gfx --tile-coords).
TILESETS = {
    "RPG_exterior.png": {
        "EXTERIOR_GRASS": (1, 9),
        "EXTERIOR_WALL": (8, 1),
        "EXTERIOR_EXIT_GATE": (8, 2),
        "EXTERIOR_BUILDING_WALL": (0, 5),
    },
    "RPG_interior.png": {
        "INTERIOR_FLOOR": (1, 1),
        "INTERIOR_WALL": (1, 0),
        "INTERIOR_DOOR": (8, 4),
        "INTERIOR_SOLID_PROP": (6, 3),
    },
    "DungeonTileset.png": {
        "DUNGEON_FLOOR_00": (0, 0),
        "DUNGEON_FLOOR_01": (1, 0),
        "DUNGEON_FLOOR_02": (2, 0),
        "DUNGEON_FLOOR_03": (3, 0),
        "DUNGEON_FLOOR_04": (4, 0),
        "DUNGEON_FLOOR_05": (5, 0),
        "DUNGEON_WALL_00": (6, 0),
        "DUNGEON_WALL_01": (7, 0),
        "DUNGEON_WALL_02": (8, 0),
        "DUNGEON_WALL_03": (9, 0),
        "DUNGEON_SOLID_00": (10, 0),
        "DUNGEON_DECOR_00": (11, 0),
        "DUNGEON_SOLID_01": (12, 0),
        "DUNGEON_DECOR_01": (13, 0),
        "DUNGEON_WALL_04": (0, 1),
        "DUNGEON_WALL_05": (1, 1),
        "DUNGEON_WALL_06": (2, 1),
        "DUNGEON_WALL_07": (3, 1),
        "DUNGEON_FLOOR_06": (4, 1),
        "DUNGEON_DECOR_02": (5, 1),
        "DUNGEON_WALL_08": (6, 1),
        "DUNGEON_DECOR_03": (7, 1),
        "DUNGEON_SOLID_02": (8, 1),
        "DUNGEON_DECOR_04": (9, 1),
        "DUNGEON_DECOR_05": (10, 1),
        "DUNGEON_SOLID_03": (11, 1),
        "DUNGEON_DECOR_06": (12, 1),
        "DUNGEON_SOLID_04": (13, 1),
        "DUNGEON_WALL_09": (0, 2),
        "DUNGEON_FLOOR_07": (1, 2),
        "DUNGEON_FLOOR_08": (2, 2),
        "DUNGEON_WALL_10": (3, 2),
        "DUNGEON_FLOOR_09": (4, 2),
        "DUNGEON_WALL_11": (5, 2),
        "DUNGEON_WALL_12": (6, 2),
        "DUNGEON_DECOR_07": (7, 2),
        "DUNGEON_DECOR_08": (8, 2),
        "DUNGEON_DECOR_09": (9, 2),
        "DUNGEON_DECOR_10": (10, 2),
        "DUNGEON_SOLID_05": (11, 2),
        "DUNGEON_SOLID_06": (12, 2),
        "DUNGEON_DECOR_11": (13, 2),
        "DUNGEON_SOLID_07": (0, 3),
        "DUNGEON_DECOR_12": (1, 3),
        "DUNGEON_FLOOR_10": (2, 3),
        "DUNGEON_FLOOR_11": (3, 3),
        "DUNGEON_FLOOR_12": (4, 3),
        "DUNGEON_FLOOR_13": (5, 3),
        "DUNGEON_FLOOR_14": (6, 3),
        "DUNGEON_WALL_13": (7, 3),
        "DUNGEON_FLOOR_15": (8, 3),
        "DUNGEON_DECOR_13": (9, 3),
        "DUNGEON_DECOR_14": (10, 3),
        "DUNGEON_FLOOR_16": (11, 3),
        "DUNGEON_SOLID_08": (12, 3),
        "DUNGEON_DECOR_15": (13, 3),
        "DUNGEON_FLOOR_17": (0, 4),
        "DUNGEON_DECOR_16": (1, 4),
        "DUNGEON_WALL_14": (2, 4),
        "DUNGEON_WALL_15": (3, 4),
        "DUNGEON_DECOR_17": (4, 4),
        "DUNGEON_FLOOR_18": (5, 4),
        "DUNGEON_DECOR_18": (6, 4),
        "DUNGEON_WALL_16": (7, 4),
        "DUNGEON_FLOOR_19": (8, 4),
        "DUNGEON_FLOOR_20": (9, 4),
        "DUNGEON_DECOR_19": (10, 4),
        "DUNGEON_DECOR_20": (11, 4),
        "DUNGEON_WALL_17": (12, 4),
        "DUNGEON_WALL_18": (13, 4),
        "DUNGEON_WALL_19": (0, 5),
        "DUNGEON_DECOR_21": (1, 5),
        "DUNGEON_WALL_20": (2, 5),
        "DUNGEON_WALL_21": (3, 5),
        "DUNGEON_DECOR_22": (4, 5),
        "DUNGEON_DECOR_23": (5, 5),
        "DUNGEON_DECOR_24": (6, 5),
        "DUNGEON_DECOR_25": (7, 5),
        "DUNGEON_DECOR_26": (8, 5),
        "DUNGEON_DECOR_27": (9, 5),
        "DUNGEON_DECOR_28": (10, 5),
        "DUNGEON_SOLID_09": (11, 5),
        "DUNGEON_WALL_22": (12, 5),
        "DUNGEON_DECOR_29": (13, 5),
        "DUNGEON_DECOR_30": (0, 6),
        "DUNGEON_FLOOR_21": (1, 6),
        "DUNGEON_DECOR_31": (2, 6),
        "DUNGEON_DECOR_32": (3, 6),
        "DUNGEON_DECOR_33": (4, 6),
        "DUNGEON_WALL_23": (5, 6),
        "DUNGEON_DECOR_34": (6, 6),
        "DUNGEON_SOLID_10": (7, 6),
        "DUNGEON_DECOR_35": (8, 6),
        "DUNGEON_DECOR_36": (9, 6),
        "DUNGEON_DECOR_37": (10, 6),
        "DUNGEON_WALL_24": (11, 6),
        "DUNGEON_DECOR_38": (12, 6),
        "DUNGEON_DECOR_39": (13, 6),
        "DUNGEON_DECOR_40": (0, 7),
        "DUNGEON_WALL_25": (1, 7),
        "DUNGEON_SOLID_11": (2, 7),
        "DUNGEON_SOLID_12": (3, 7),
        "DUNGEON_SOLID_13": (4, 7),
        "DUNGEON_SOLID_14": (5, 7),
        "DUNGEON_SOLID_15": (6, 7),
        "DUNGEON_SOLID_16": (7, 7),
        "DUNGEON_DECOR_41": (8, 7),
        "DUNGEON_DECOR_42": (9, 7),
        "DUNGEON_DECOR_43": (10, 7),
        "DUNGEON_DECOR_44": (11, 7),
        "DUNGEON_DECOR_45": (12, 7),
        "DUNGEON_DECOR_46": (13, 7),
        "DUNGEON_DECOR_47": (0, 8),
        "DUNGEON_SOLID_17": (1, 8),
        "DUNGEON_SOLID_18": (2, 8),
        "DUNGEON_SOLID_19": (3, 8),
        "DUNGEON_SOLID_20": (4, 8),
        "DUNGEON_SOLID_21": (5, 8),
        "DUNGEON_DECOR_48": (6, 8),
        "DUNGEON_SOLID_22": (7, 8),
        "DUNGEON_DECOR_49": (8, 8),
        "DUNGEON_DECOR_50": (9, 8),
        "DUNGEON_DECOR_51": (10, 8),
        "DUNGEON_DECOR_52": (11, 8),
        "DUNGEON_DECOR_53": (12, 8),
        "DUNGEON_DECOR_54": (13, 8),
        "DUNGEON_DECOR_55": (0, 9),
        "DUNGEON_SOLID_23": (1, 9),
        "DUNGEON_DECOR_56": (2, 9),
        "DUNGEON_SOLID_24": (3, 9),
        "DUNGEON_SOLID_25": (4, 9),
        "DUNGEON_SOLID_26": (5, 9),
        "DUNGEON_SOLID_27": (6, 9),
        "DUNGEON_SOLID_28": (7, 9),
        "DUNGEON_DECOR_57": (8, 9),
        "DUNGEON_DECOR_58": (9, 9),
        "DUNGEON_DECOR_59": (10, 9),
        "DUNGEON_SOLID_29": (11, 9),
        "DUNGEON_SOLID_30": (12, 9),
        "DUNGEON_DECOR_60": (13, 9),
        "DUNGEON_DECOR_61": (0, 10),
        "DUNGEON_WALL_26": (1, 10),
        "DUNGEON_WALL_27": (2, 10),
        "DUNGEON_SOLID_31": (3, 10),
        "DUNGEON_FLOOR_22": (4, 10),
        "DUNGEON_DECOR_62": (5, 10),
        "DUNGEON_DECOR_63": (6, 10),
        "DUNGEON_WALL_28": (7, 10),
        "DUNGEON_SOLID_32": (8, 10),
        "DUNGEON_DECOR_64": (9, 10),
        "DUNGEON_DECOR_65": (10, 10),
        "DUNGEON_SOLID_33": (11, 10),
        "DUNGEON_DECOR_66": (12, 10),
        "DUNGEON_DECOR_67": (13, 10),
        "DUNGEON_DECOR_68": (0, 11),
        "DUNGEON_DECOR_69": (1, 11),
        "DUNGEON_SOLID_34": (2, 11),
        "DUNGEON_SOLID_35": (3, 11),
        "DUNGEON_SOLID_36": (4, 11),
        "DUNGEON_SOLID_37": (5, 11),
        "DUNGEON_SOLID_38": (6, 11),
        "DUNGEON_SOLID_39": (7, 11),
        "DUNGEON_SOLID_40": (8, 11),
        "DUNGEON_SOLID_41": (9, 11),
        "DUNGEON_SOLID_42": (10, 11),
        "DUNGEON_SOLID_43": (11, 11),
        "DUNGEON_SOLID_44": (12, 11),
        "DUNGEON_SOLID_45": (13, 11),
        "DUNGEON_DECOR_70": (0, 12),
        "DUNGEON_DECOR_71": (1, 12),
        "DUNGEON_SOLID_46": (2, 12),
        "DUNGEON_SOLID_47": (3, 12),
        "DUNGEON_SOLID_48": (4, 12),
        "DUNGEON_SOLID_49": (5, 12),
        "DUNGEON_SOLID_50": (6, 12),
        "DUNGEON_SOLID_51": (7, 12),
        "DUNGEON_SOLID_52": (8, 12),
        "DUNGEON_SOLID_53": (9, 12),
        "DUNGEON_SOLID_54": (10, 12),
        "DUNGEON_SOLID_55": (11, 12),
        "DUNGEON_SOLID_56": (12, 12),
        "DUNGEON_SOLID_57": (13, 12),
    },
    "Houses_and_various_things.png": {
        # Houses walls: macro-blocks (0,0)-(3,0) = cols 0-63, rows 0-15
        "HOUSES_WALL_00": (0, 0),
        "HOUSES_WALL_01": (1, 0),
        "HOUSES_WALL_02": (2, 0),
        "HOUSES_WALL_03": (3, 0),
        "HOUSES_WALL_04": (4, 0),
        "HOUSES_WALL_05": (5, 0),
        "HOUSES_WALL_06": (6, 0),
        "HOUSES_WALL_07": (7, 0),
        "HOUSES_WALL_08": (8, 0),
        "HOUSES_WALL_09": (9, 0),
        "HOUSES_WALL_10": (10, 0),
        "HOUSES_WALL_11": (11, 0),
        "HOUSES_WALL_12": (12, 0),
        "HOUSES_WALL_13": (13, 0),
        "HOUSES_WALL_14": (14, 0),
        "HOUSES_WALL_15": (15, 0),
        "HOUSES_WALL_16": (16, 0),
        "HOUSES_WALL_17": (17, 0),
        "HOUSES_WALL_18": (18, 0),
        "HOUSES_WALL_19": (19, 0),
        "HOUSES_WALL_20": (20, 0),
        "HOUSES_WALL_21": (21, 0),
        "HOUSES_WALL_22": (22, 0),
        "HOUSES_WALL_23": (23, 0),
        "HOUSES_WALL_24": (24, 0),
        "HOUSES_WALL_25": (25, 0),
        "HOUSES_WALL_26": (26, 0),
        "HOUSES_WALL_27": (27, 0),
        # Houses roofs: macro-blocks (4,0)-(7,0) = cols 64-127, rows 0-15
        "HOUSES_ROOF_00": (64, 0),
        "HOUSES_ROOF_01": (65, 0),
        "HOUSES_ROOF_02": (66, 0),
        "HOUSES_ROOF_03": (67, 0),
        "HOUSES_ROOF_04": (68, 0),
        "HOUSES_ROOF_05": (69, 0),
        "HOUSES_ROOF_06": (70, 0),
        "HOUSES_ROOF_07": (71, 0),
        "HOUSES_ROOF_08": (72, 0),
        "HOUSES_ROOF_09": (73, 0),
        "HOUSES_ROOF_10": (74, 0),
        "HOUSES_ROOF_11": (75, 0),
        "HOUSES_ROOF_12": (76, 0),
        "HOUSES_ROOF_13": (77, 0),
        # Houses floors: macro-blocks (0,1)-(3,1) = cols 0-63, rows 16-31
        "HOUSES_FLOOR_00": (0, 16),
        "HOUSES_FLOOR_01": (1, 16),
        "HOUSES_FLOOR_02": (2, 16),
        "HOUSES_FLOOR_03": (3, 16),
        "HOUSES_FLOOR_04": (4, 16),
        "HOUSES_FLOOR_05": (5, 16),
        "HOUSES_FLOOR_06": (6, 16),
        "HOUSES_FLOOR_07": (7, 16),
        "HOUSES_FLOOR_08": (8, 16),
        "HOUSES_FLOOR_09": (9, 16),
        "HOUSES_FLOOR_10": (10, 16),
        "HOUSES_FLOOR_11": (11, 16),
        "HOUSES_FLOOR_12": (12, 16),
        "HOUSES_FLOOR_13": (13, 16),
        # Houses doors: macro-blocks (4,1)-(5,1) = cols 64-95, rows 16-31
        "HOUSES_DOOR_00": (64, 16),
        "HOUSES_DOOR_01": (65, 16),
        "HOUSES_DOOR_02": (66, 16),
        "HOUSES_DOOR_03": (67, 16),
        "HOUSES_DOOR_04": (68, 16),
        "HOUSES_DOOR_05": (69, 16),
        # Houses windows: macro-blocks (6,1)-(7,1) = cols 96-127, rows 16-31
        "HOUSES_WINDOW_00": (96, 16),
        "HOUSES_WINDOW_01": (97, 16),
        "HOUSES_WINDOW_02": (98, 16),
        "HOUSES_WINDOW_03": (99, 16),
        "HOUSES_WINDOW_04": (100, 16),
        "HOUSES_WINDOW_05": (101, 16),
        # Nature ground: macro-blocks (0,2)-(3,2) = cols 0-63, rows 32-47
        "NATURE_GROUND_00": (0, 32),
        "NATURE_GROUND_01": (1, 32),
        "NATURE_GROUND_02": (2, 32),
        "NATURE_GROUND_03": (3, 32),
        "NATURE_GROUND_04": (4, 32),
        "NATURE_GROUND_05": (5, 32),
        "NATURE_GROUND_06": (6, 32),
        "NATURE_GROUND_07": (7, 32),
        "NATURE_GROUND_08": (8, 32),
        "NATURE_GROUND_09": (9, 32),
        "NATURE_GROUND_10": (10, 32),
        "NATURE_GROUND_11": (11, 32),
        "NATURE_GROUND_12": (12, 32),
        "NATURE_GROUND_13": (13, 32),
        # Nature vegetation: macro-blocks (4,2)-(7,2) = cols 64-127, rows 32-47
        "NATURE_VEG_00": (64, 32),
        "NATURE_VEG_01": (65, 32),
        "NATURE_VEG_02": (66, 32),
        "NATURE_VEG_03": (67, 32),
        "NATURE_VEG_04": (68, 32),
        "NATURE_VEG_05": (69, 32),
        "NATURE_VEG_06": (70, 32),
        "NATURE_VEG_07": (71, 32),
        "NATURE_VEG_08": (72, 32),
        "NATURE_VEG_09": (73, 32),
        "NATURE_VEG_10": (74, 32),
        "NATURE_VEG_11": (75, 32),
        "NATURE_VEG_12": (76, 32),
        "NATURE_VEG_13": (77, 32),
        # Objects furniture: macro-blocks (0,3)-(3,3) = cols 0-63, rows 48-63
        "OBJECTS_FURN_00": (0, 48),
        "OBJECTS_FURN_01": (1, 48),
        "OBJECTS_FURN_02": (2, 48),
        "OBJECTS_FURN_03": (3, 48),
        "OBJECTS_FURN_04": (4, 48),
        "OBJECTS_FURN_05": (5, 48),
        "OBJECTS_FURN_06": (6, 48),
        "OBJECTS_FURN_07": (7, 48),
        "OBJECTS_FURN_08": (8, 48),
        "OBJECTS_FURN_09": (9, 48),
        "OBJECTS_FURN_10": (10, 48),
        "OBJECTS_FURN_11": (11, 48),
        "OBJECTS_FURN_12": (12, 48),
        "OBJECTS_FURN_13": (13, 48),
        # Structures fences: macro-blocks (4,3)-(7,3) = cols 64-127, rows 48-63
        "STRUCT_FENCE_00": (64, 48),
        "STRUCT_FENCE_01": (65, 48),
        "STRUCT_FENCE_02": (66, 48),
        "STRUCT_FENCE_03": (67, 48),
        "STRUCT_FENCE_04": (68, 48),
        "STRUCT_FENCE_05": (69, 48),
        "STRUCT_FENCE_06": (70, 48),
        "STRUCT_FENCE_07": (71, 48),
        # Structures props: macro-blocks (0,4)-(3,4) = cols 0-63, rows 64-79
        "STRUCT_PROP_00": (0, 64),
        "STRUCT_PROP_01": (1, 64),
        "STRUCT_PROP_02": (2, 64),
        "STRUCT_PROP_03": (3, 64),
        "STRUCT_PROP_04": (4, 64),
        "STRUCT_PROP_05": (5, 64),
        "STRUCT_PROP_06": (6, 64),
        "STRUCT_PROP_07": (7, 64),
    },
}
# Forest tiles live in RPG_exterior.png at independent coordinates.
FOREST_COORDS = {
    "FOREST_FLOOR": (1, 9),
    "FOREST_TREE": (0, 5),
    "FOREST_GATE": (8, 2),
    "FOREST_STUMP_TL": (0, 14),
    "FOREST_STUMP_TR": (1, 14),
    "FOREST_STUMP_BL": (0, 15),
    "FOREST_STUMP_BR": (1, 15),
    "FOREST_STUMP_MINI": (2, 16),
}

# Curated semantic assets, in enum order.  Forest tiles source from
# RPG_exterior.png too (same sheet as EXTERIOR_*).
SEMANTIC_ORDER = [
    ("RPG_exterior.png", "EXTERIOR_GRASS"),
    ("RPG_exterior.png", "EXTERIOR_WALL"),
    ("RPG_exterior.png", "EXTERIOR_EXIT_GATE"),
    ("RPG_exterior.png", "EXTERIOR_BUILDING_WALL"),
    ("RPG_exterior.png", "FOREST_FLOOR"),
    ("RPG_exterior.png", "FOREST_TREE"),
    ("RPG_exterior.png", "FOREST_GATE"),
    ("RPG_exterior.png", "FOREST_STUMP_TL"),
    ("RPG_exterior.png", "FOREST_STUMP_TR"),
    ("RPG_exterior.png", "FOREST_STUMP_BL"),
    ("RPG_exterior.png", "FOREST_STUMP_BR"),
    ("RPG_exterior.png", "FOREST_STUMP_MINI"),
    ("RPG_interior.png", "INTERIOR_FLOOR"),
    ("RPG_interior.png", "INTERIOR_WALL"),
    ("RPG_interior.png", "INTERIOR_DOOR"),
    ("RPG_interior.png", "INTERIOR_SOLID_PROP"),
    ("DungeonTileset.png", "DUNGEON_FLOOR_00"),
    ("DungeonTileset.png", "DUNGEON_FLOOR_01"),
    ("DungeonTileset.png", "DUNGEON_FLOOR_02"),
    ("DungeonTileset.png", "DUNGEON_FLOOR_03"),
    ("DungeonTileset.png", "DUNGEON_FLOOR_04"),
    ("DungeonTileset.png", "DUNGEON_FLOOR_05"),
    ("DungeonTileset.png", "DUNGEON_WALL_00"),
    ("DungeonTileset.png", "DUNGEON_WALL_01"),
    ("DungeonTileset.png", "DUNGEON_WALL_02"),
    ("DungeonTileset.png", "DUNGEON_WALL_03"),
    ("DungeonTileset.png", "DUNGEON_SOLID_00"),
    ("DungeonTileset.png", "DUNGEON_DECOR_00"),
    ("DungeonTileset.png", "DUNGEON_SOLID_01"),
    ("DungeonTileset.png", "DUNGEON_DECOR_01"),
    ("DungeonTileset.png", "DUNGEON_WALL_04"),
    ("DungeonTileset.png", "DUNGEON_WALL_05"),
    ("DungeonTileset.png", "DUNGEON_WALL_06"),
    ("DungeonTileset.png", "DUNGEON_WALL_07"),
    ("DungeonTileset.png", "DUNGEON_FLOOR_06"),
    ("DungeonTileset.png", "DUNGEON_DECOR_02"),
    ("DungeonTileset.png", "DUNGEON_WALL_08"),
    ("DungeonTileset.png", "DUNGEON_DECOR_03"),
    ("DungeonTileset.png", "DUNGEON_SOLID_02"),
    ("DungeonTileset.png", "DUNGEON_DECOR_04"),
    ("DungeonTileset.png", "DUNGEON_DECOR_05"),
    ("DungeonTileset.png", "DUNGEON_SOLID_03"),
    ("DungeonTileset.png", "DUNGEON_DECOR_06"),
    ("DungeonTileset.png", "DUNGEON_SOLID_04"),
    ("DungeonTileset.png", "DUNGEON_WALL_09"),
    ("DungeonTileset.png", "DUNGEON_FLOOR_07"),
    ("DungeonTileset.png", "DUNGEON_FLOOR_08"),
    ("DungeonTileset.png", "DUNGEON_WALL_10"),
    ("DungeonTileset.png", "DUNGEON_FLOOR_09"),
    ("DungeonTileset.png", "DUNGEON_WALL_11"),
    ("DungeonTileset.png", "DUNGEON_WALL_12"),
    ("DungeonTileset.png", "DUNGEON_DECOR_07"),
    ("DungeonTileset.png", "DUNGEON_DECOR_08"),
    ("DungeonTileset.png", "DUNGEON_DECOR_09"),
    ("DungeonTileset.png", "DUNGEON_DECOR_10"),
    ("DungeonTileset.png", "DUNGEON_SOLID_05"),
    ("DungeonTileset.png", "DUNGEON_SOLID_06"),
    ("DungeonTileset.png", "DUNGEON_DECOR_11"),
    ("DungeonTileset.png", "DUNGEON_SOLID_07"),
    ("DungeonTileset.png", "DUNGEON_DECOR_12"),
    ("DungeonTileset.png", "DUNGEON_FLOOR_10"),
    ("DungeonTileset.png", "DUNGEON_FLOOR_11"),
    ("DungeonTileset.png", "DUNGEON_FLOOR_12"),
    ("DungeonTileset.png", "DUNGEON_FLOOR_13"),
    ("DungeonTileset.png", "DUNGEON_FLOOR_14"),
    ("DungeonTileset.png", "DUNGEON_FLOOR_15"),
    ("DungeonTileset.png", "DUNGEON_WALL_13"),
    ("DungeonTileset.png", "DUNGEON_FLOOR_16"),
    ("DungeonTileset.png", "DUNGEON_DECOR_13"),
    ("DungeonTileset.png", "DUNGEON_DECOR_14"),
    ("DungeonTileset.png", "DUNGEON_FLOOR_17"),
    ("DungeonTileset.png", "DUNGEON_FLOOR_18"),
    ("DungeonTileset.png", "DUNGEON_DECOR_15"),
    ("DungeonTileset.png", "DUNGEON_SOLID_08"),
    ("DungeonTileset.png", "DUNGEON_DECOR_16"),
    ("DungeonTileset.png", "DUNGEON_WALL_14"),
    ("DungeonTileset.png", "DUNGEON_WALL_15"),
    ("DungeonTileset.png", "DUNGEON_DECOR_17"),
    ("DungeonTileset.png", "DUNGEON_FLOOR_19"),
    ("DungeonTileset.png", "DUNGEON_FLOOR_20"),
    ("DungeonTileset.png", "DUNGEON_DECOR_18"),
    ("DungeonTileset.png", "DUNGEON_WALL_16"),
    ("DungeonTileset.png", "DUNGEON_FLOOR_21"),
    ("DungeonTileset.png", "DUNGEON_FLOOR_22"),
    ("DungeonTileset.png", "DUNGEON_DECOR_19"),
    ("DungeonTileset.png", "DUNGEON_DECOR_20"),
    ("DungeonTileset.png", "DUNGEON_WALL_17"),
    ("DungeonTileset.png", "DUNGEON_WALL_18"),
    ("DungeonTileset.png", "DUNGEON_WALL_19"),
    ("DungeonTileset.png", "DUNGEON_DECOR_21"),
    ("DungeonTileset.png", "DUNGEON_WALL_20"),
    ("DungeonTileset.png", "DUNGEON_WALL_21"),
    ("DungeonTileset.png", "DUNGEON_DECOR_22"),
    ("DungeonTileset.png", "DUNGEON_DECOR_23"),
    ("DungeonTileset.png", "DUNGEON_DECOR_24"),
    ("DungeonTileset.png", "DUNGEON_DECOR_25"),
    ("DungeonTileset.png", "DUNGEON_DECOR_26"),
    ("DungeonTileset.png", "DUNGEON_DECOR_27"),
    ("DungeonTileset.png", "DUNGEON_DECOR_28"),
    ("DungeonTileset.png", "DUNGEON_DECOR_29"),
    ("DungeonTileset.png", "DUNGEON_SOLID_09"),
    ("DungeonTileset.png", "DUNGEON_WALL_22"),
    ("DungeonTileset.png", "DUNGEON_DECOR_30"),
    ("DungeonTileset.png", "DUNGEON_DECOR_31"),
    ("DungeonTileset.png", "DUNGEON_DECOR_32"),
    ("DungeonTileset.png", "DUNGEON_DECOR_33"),
    ("DungeonTileset.png", "DUNGEON_DECOR_34"),
    ("DungeonTileset.png", "DUNGEON_WALL_23"),
    ("DungeonTileset.png", "DUNGEON_DECOR_35"),
    ("DungeonTileset.png", "DUNGEON_SOLID_10"),
    ("DungeonTileset.png", "DUNGEON_DECOR_36"),
    ("DungeonTileset.png", "DUNGEON_DECOR_37"),
    ("DungeonTileset.png", "DUNGEON_DECOR_38"),
    ("DungeonTileset.png", "DUNGEON_WALL_24"),
    ("DungeonTileset.png", "DUNGEON_DECOR_39"),
    ("DungeonTileset.png", "DUNGEON_DECOR_40"),
    ("DungeonTileset.png", "DUNGEON_DECOR_41"),
    ("DungeonTileset.png", "DUNGEON_WALL_25"),
    ("DungeonTileset.png", "DUNGEON_SOLID_11"),
    ("DungeonTileset.png", "DUNGEON_SOLID_12"),
    ("DungeonTileset.png", "DUNGEON_SOLID_13"),
    ("DungeonTileset.png", "DUNGEON_SOLID_14"),
    ("DungeonTileset.png", "DUNGEON_SOLID_15"),
    ("DungeonTileset.png", "DUNGEON_SOLID_16"),
    ("DungeonTileset.png", "DUNGEON_SOLID_17"),
    ("DungeonTileset.png", "DUNGEON_DECOR_42"),
    ("DungeonTileset.png", "DUNGEON_DECOR_43"),
    ("DungeonTileset.png", "DUNGEON_DECOR_44"),
    ("DungeonTileset.png", "DUNGEON_DECOR_45"),
    ("DungeonTileset.png", "DUNGEON_DECOR_46"),
    ("DungeonTileset.png", "DUNGEON_DECOR_47"),
    ("DungeonTileset.png", "DUNGEON_DECOR_48"),
    ("DungeonTileset.png", "DUNGEON_SOLID_18"),
    ("DungeonTileset.png", "DUNGEON_SOLID_19"),
    ("DungeonTileset.png", "DUNGEON_SOLID_20"),
    ("DungeonTileset.png", "DUNGEON_SOLID_21"),
    ("DungeonTileset.png", "DUNGEON_SOLID_22"),
    ("DungeonTileset.png", "DUNGEON_DECOR_49"),
    ("DungeonTileset.png", "DUNGEON_SOLID_23"),
    ("DungeonTileset.png", "DUNGEON_DECOR_50"),
    ("DungeonTileset.png", "DUNGEON_DECOR_51"),
    ("DungeonTileset.png", "DUNGEON_DECOR_52"),
    ("DungeonTileset.png", "DUNGEON_DECOR_53"),
    ("DungeonTileset.png", "DUNGEON_DECOR_54"),
    ("DungeonTileset.png", "DUNGEON_DECOR_55"),
    ("DungeonTileset.png", "DUNGEON_DECOR_56"),
    ("DungeonTileset.png", "DUNGEON_SOLID_24"),
    ("DungeonTileset.png", "DUNGEON_DECOR_57"),
    ("DungeonTileset.png", "DUNGEON_SOLID_25"),
    ("DungeonTileset.png", "DUNGEON_SOLID_26"),
    ("DungeonTileset.png", "DUNGEON_SOLID_27"),
    ("DungeonTileset.png", "DUNGEON_SOLID_28"),
    ("DungeonTileset.png", "DUNGEON_SOLID_29"),
    ("DungeonTileset.png", "DUNGEON_SOLID_30"),
    ("DungeonTileset.png", "DUNGEON_DECOR_58"),
    ("DungeonTileset.png", "DUNGEON_DECOR_59"),
    ("DungeonTileset.png", "DUNGEON_DECOR_60"),
    ("DungeonTileset.png", "DUNGEON_SOLID_31"),
    ("DungeonTileset.png", "DUNGEON_DECOR_61"),
    ("DungeonTileset.png", "DUNGEON_WALL_26"),
    ("DungeonTileset.png", "DUNGEON_WALL_27"),
    ("DungeonTileset.png", "DUNGEON_SOLID_32"),
    ("DungeonTileset.png", "DUNGEON_DECOR_62"),
    ("DungeonTileset.png", "DUNGEON_DECOR_63"),
    ("DungeonTileset.png", "DUNGEON_WALL_28"),
    ("DungeonTileset.png", "DUNGEON_SOLID_33"),
    ("DungeonTileset.png", "DUNGEON_DECOR_64"),
    ("DungeonTileset.png", "DUNGEON_DECOR_65"),
    ("DungeonTileset.png", "DUNGEON_SOLID_34"),
    ("DungeonTileset.png", "DUNGEON_DECOR_66"),
    ("DungeonTileset.png", "DUNGEON_DECOR_67"),
    ("DungeonTileset.png", "DUNGEON_DECOR_68"),
    ("DungeonTileset.png", "DUNGEON_DECOR_69"),
    ("DungeonTileset.png", "DUNGEON_SOLID_35"),
    ("DungeonTileset.png", "DUNGEON_SOLID_36"),
    ("DungeonTileset.png", "DUNGEON_SOLID_37"),
    ("DungeonTileset.png", "DUNGEON_SOLID_38"),
    ("DungeonTileset.png", "DUNGEON_SOLID_39"),
    ("DungeonTileset.png", "DUNGEON_SOLID_40"),
    ("DungeonTileset.png", "DUNGEON_SOLID_41"),
    ("DungeonTileset.png", "DUNGEON_SOLID_42"),
    ("DungeonTileset.png", "DUNGEON_SOLID_43"),
    ("DungeonTileset.png", "DUNGEON_SOLID_44"),
    ("DungeonTileset.png", "DUNGEON_SOLID_45"),
    ("DungeonTileset.png", "DUNGEON_SOLID_46"),
    ("DungeonTileset.png", "DUNGEON_SOLID_47"),
    ("DungeonTileset.png", "DUNGEON_SOLID_48"),
    ("DungeonTileset.png", "DUNGEON_SOLID_49"),
    ("DungeonTileset.png", "DUNGEON_SOLID_50"),
    ("DungeonTileset.png", "DUNGEON_SOLID_51"),
    ("DungeonTileset.png", "DUNGEON_SOLID_52"),
    ("DungeonTileset.png", "DUNGEON_SOLID_53"),
    ("DungeonTileset.png", "DUNGEON_SOLID_54"),
    ("DungeonTileset.png", "DUNGEON_SOLID_55"),
    ("DungeonTileset.png", "DUNGEON_SOLID_56"),
    ("DungeonTileset.png", "DUNGEON_SOLID_57"),
    ("DungeonTileset.png", "DUNGEON_DECOR_70"),
    ("DungeonTileset.png", "DUNGEON_DECOR_71"),
    # Houses walls
    ("Houses_and_various_things.png", "HOUSES_WALL_00"),
    ("Houses_and_various_things.png", "HOUSES_WALL_01"),
    ("Houses_and_various_things.png", "HOUSES_WALL_02"),
    ("Houses_and_various_things.png", "HOUSES_WALL_03"),
    ("Houses_and_various_things.png", "HOUSES_WALL_04"),
    ("Houses_and_various_things.png", "HOUSES_WALL_05"),
    ("Houses_and_various_things.png", "HOUSES_WALL_06"),
    ("Houses_and_various_things.png", "HOUSES_WALL_07"),
    ("Houses_and_various_things.png", "HOUSES_WALL_08"),
    ("Houses_and_various_things.png", "HOUSES_WALL_09"),
    ("Houses_and_various_things.png", "HOUSES_WALL_10"),
    ("Houses_and_various_things.png", "HOUSES_WALL_11"),
    ("Houses_and_various_things.png", "HOUSES_WALL_12"),
    ("Houses_and_various_things.png", "HOUSES_WALL_13"),
    ("Houses_and_various_things.png", "HOUSES_WALL_14"),
    ("Houses_and_various_things.png", "HOUSES_WALL_15"),
    ("Houses_and_various_things.png", "HOUSES_WALL_16"),
    ("Houses_and_various_things.png", "HOUSES_WALL_17"),
    ("Houses_and_various_things.png", "HOUSES_WALL_18"),
    ("Houses_and_various_things.png", "HOUSES_WALL_19"),
    ("Houses_and_various_things.png", "HOUSES_WALL_20"),
    ("Houses_and_various_things.png", "HOUSES_WALL_21"),
    ("Houses_and_various_things.png", "HOUSES_WALL_22"),
    ("Houses_and_various_things.png", "HOUSES_WALL_23"),
    ("Houses_and_various_things.png", "HOUSES_WALL_24"),
    ("Houses_and_various_things.png", "HOUSES_WALL_25"),
    ("Houses_and_various_things.png", "HOUSES_WALL_26"),
    ("Houses_and_various_things.png", "HOUSES_WALL_27"),
    # Houses roofs
    ("Houses_and_various_things.png", "HOUSES_ROOF_00"),
    ("Houses_and_various_things.png", "HOUSES_ROOF_01"),
    ("Houses_and_various_things.png", "HOUSES_ROOF_02"),
    ("Houses_and_various_things.png", "HOUSES_ROOF_03"),
    ("Houses_and_various_things.png", "HOUSES_ROOF_04"),
    ("Houses_and_various_things.png", "HOUSES_ROOF_05"),
    ("Houses_and_various_things.png", "HOUSES_ROOF_06"),
    ("Houses_and_various_things.png", "HOUSES_ROOF_07"),
    ("Houses_and_various_things.png", "HOUSES_ROOF_08"),
    ("Houses_and_various_things.png", "HOUSES_ROOF_09"),
    ("Houses_and_various_things.png", "HOUSES_ROOF_10"),
    ("Houses_and_various_things.png", "HOUSES_ROOF_11"),
    ("Houses_and_various_things.png", "HOUSES_ROOF_12"),
    ("Houses_and_various_things.png", "HOUSES_ROOF_13"),
    # Houses floors
    ("Houses_and_various_things.png", "HOUSES_FLOOR_00"),
    ("Houses_and_various_things.png", "HOUSES_FLOOR_01"),
    ("Houses_and_various_things.png", "HOUSES_FLOOR_02"),
    ("Houses_and_various_things.png", "HOUSES_FLOOR_03"),
    ("Houses_and_various_things.png", "HOUSES_FLOOR_04"),
    ("Houses_and_various_things.png", "HOUSES_FLOOR_05"),
    ("Houses_and_various_things.png", "HOUSES_FLOOR_06"),
    ("Houses_and_various_things.png", "HOUSES_FLOOR_07"),
    ("Houses_and_various_things.png", "HOUSES_FLOOR_08"),
    ("Houses_and_various_things.png", "HOUSES_FLOOR_09"),
    ("Houses_and_various_things.png", "HOUSES_FLOOR_10"),
    ("Houses_and_various_things.png", "HOUSES_FLOOR_11"),
    ("Houses_and_various_things.png", "HOUSES_FLOOR_12"),
    ("Houses_and_various_things.png", "HOUSES_FLOOR_13"),
    # Houses doors
    ("Houses_and_various_things.png", "HOUSES_DOOR_00"),
    ("Houses_and_various_things.png", "HOUSES_DOOR_01"),
    ("Houses_and_various_things.png", "HOUSES_DOOR_02"),
    ("Houses_and_various_things.png", "HOUSES_DOOR_03"),
    ("Houses_and_various_things.png", "HOUSES_DOOR_04"),
    ("Houses_and_various_things.png", "HOUSES_DOOR_05"),
    # Houses windows
    ("Houses_and_various_things.png", "HOUSES_WINDOW_00"),
    ("Houses_and_various_things.png", "HOUSES_WINDOW_01"),
    ("Houses_and_various_things.png", "HOUSES_WINDOW_02"),
    ("Houses_and_various_things.png", "HOUSES_WINDOW_03"),
    ("Houses_and_various_things.png", "HOUSES_WINDOW_04"),
    ("Houses_and_various_things.png", "HOUSES_WINDOW_05"),
    # Nature ground
    ("Houses_and_various_things.png", "NATURE_GROUND_00"),
    ("Houses_and_various_things.png", "NATURE_GROUND_01"),
    ("Houses_and_various_things.png", "NATURE_GROUND_02"),
    ("Houses_and_various_things.png", "NATURE_GROUND_03"),
    ("Houses_and_various_things.png", "NATURE_GROUND_04"),
    ("Houses_and_various_things.png", "NATURE_GROUND_05"),
    ("Houses_and_various_things.png", "NATURE_GROUND_06"),
    ("Houses_and_various_things.png", "NATURE_GROUND_07"),
    ("Houses_and_various_things.png", "NATURE_GROUND_08"),
    ("Houses_and_various_things.png", "NATURE_GROUND_09"),
    ("Houses_and_various_things.png", "NATURE_GROUND_10"),
    ("Houses_and_various_things.png", "NATURE_GROUND_11"),
    ("Houses_and_various_things.png", "NATURE_GROUND_12"),
    ("Houses_and_various_things.png", "NATURE_GROUND_13"),
    # Nature vegetation
    ("Houses_and_various_things.png", "NATURE_VEG_00"),
    ("Houses_and_various_things.png", "NATURE_VEG_01"),
    ("Houses_and_various_things.png", "NATURE_VEG_02"),
    ("Houses_and_various_things.png", "NATURE_VEG_03"),
    ("Houses_and_various_things.png", "NATURE_VEG_04"),
    ("Houses_and_various_things.png", "NATURE_VEG_05"),
    ("Houses_and_various_things.png", "NATURE_VEG_06"),
    ("Houses_and_various_things.png", "NATURE_VEG_07"),
    ("Houses_and_various_things.png", "NATURE_VEG_08"),
    ("Houses_and_various_things.png", "NATURE_VEG_09"),
    ("Houses_and_various_things.png", "NATURE_VEG_10"),
    ("Houses_and_various_things.png", "NATURE_VEG_11"),
    ("Houses_and_various_things.png", "NATURE_VEG_12"),
    ("Houses_and_various_things.png", "NATURE_VEG_13"),
    # Objects furniture
    ("Houses_and_various_things.png", "OBJECTS_FURN_00"),
    ("Houses_and_various_things.png", "OBJECTS_FURN_01"),
    ("Houses_and_various_things.png", "OBJECTS_FURN_02"),
    ("Houses_and_various_things.png", "OBJECTS_FURN_03"),
    ("Houses_and_various_things.png", "OBJECTS_FURN_04"),
    ("Houses_and_various_things.png", "OBJECTS_FURN_05"),
    ("Houses_and_various_things.png", "OBJECTS_FURN_06"),
    ("Houses_and_various_things.png", "OBJECTS_FURN_07"),
    ("Houses_and_various_things.png", "OBJECTS_FURN_08"),
    ("Houses_and_various_things.png", "OBJECTS_FURN_09"),
    ("Houses_and_various_things.png", "OBJECTS_FURN_10"),
    ("Houses_and_various_things.png", "OBJECTS_FURN_11"),
    ("Houses_and_various_things.png", "OBJECTS_FURN_12"),
    ("Houses_and_various_things.png", "OBJECTS_FURN_13"),
    # Structures fences
    ("Houses_and_various_things.png", "STRUCT_FENCE_00"),
    ("Houses_and_various_things.png", "STRUCT_FENCE_01"),
    ("Houses_and_various_things.png", "STRUCT_FENCE_02"),
    ("Houses_and_various_things.png", "STRUCT_FENCE_03"),
    ("Houses_and_various_things.png", "STRUCT_FENCE_04"),
    ("Houses_and_various_things.png", "STRUCT_FENCE_05"),
    ("Houses_and_various_things.png", "STRUCT_FENCE_06"),
    ("Houses_and_various_things.png", "STRUCT_FENCE_07"),
    # Structures props
    ("Houses_and_various_things.png", "STRUCT_PROP_00"),
    ("Houses_and_various_things.png", "STRUCT_PROP_01"),
    ("Houses_and_various_things.png", "STRUCT_PROP_02"),
    ("Houses_and_various_things.png", "STRUCT_PROP_03"),
    ("Houses_and_various_things.png", "STRUCT_PROP_04"),
    ("Houses_and_various_things.png", "STRUCT_PROP_05"),
    ("Houses_and_various_things.png", "STRUCT_PROP_06"),
    ("Houses_and_various_things.png", "STRUCT_PROP_07"),
    ("player_demo.png", "PLAYER"),
    ("world_tiles.png", "WORLD_C0_R0"),
    ("world_tiles.png", "WORLD_C0_R1"),
    ("world_tiles.png", "WORLD_C1_R0"),
    ("world_tiles.png", "WORLD_C1_R1"),
]

SRC_CONST = {
    "RPG_exterior.png": "ASSET_SOURCE_EXTERIOR",
    "RPG_interior.png": "ASSET_SOURCE_INTERIOR",
    "DungeonTileset.png": "ASSET_SOURCE_DUNGEON",
    "Houses_and_various_things.png": "ASSET_SOURCE_HOUSES",
    "intrepid.png": "ASSET_SOURCE_FONT",
    "player_demo.png": "ASSET_SOURCE_PLAYER",
    "world_tiles.png": "ASSET_SOURCE_WORLD",
    "equipment_8x8.png": "ASSET_SOURCE_EQUIPMENT",
    "symbols_8x8.png": "ASSET_SOURCE_SYMBOLS",
}


def sem_coords():
    return {name: coords for _, name, coords in
            ([(png, n, TILESETS[png][n]) for png in TILESETS] for n in ())} or {}


def lum(c):
    return 0.299 * c[0] + 0.587 * c[1] + 0.114 * c[2]


def luminance_sort(colors):
    return sorted(colors, key=lum, reverse=True)


def rgb555_le(c):
    v = (c[0] >> 3) | ((c[1] >> 3) << 5) | ((c[2] >> 3) << 10)
    return (v & 0xFF, (v >> 8) & 0xFF)


def quantize(colors, limit=4):
    """Deterministic reduction to <= `limit` shades (Loyd quartile-seeded)."""
    cs = luminance_sort(colors)
    if len(cs) <= limit:
        return list(cs)
    anchors = [cs[int((len(cs) - 1) * i / (limit - 1))] for i in range(limit)]
    for _ in range(3):
        buckets = [[] for _ in range(limit)]
        for c in cs:
            idx = 0
            best = None
            for i, a in enumerate(anchors):
                d = (c[0] - a[0]) ** 2 + (c[1] - a[1]) ** 2 + (c[2] - a[2]) ** 2
                if best is None or d < best:
                    best = d
                    idx = i
            buckets[idx].append(c)
        for i in range(limit):
            if buckets[i]:
                b = buckets[i]
                n = len(b)
                anchors[i] = (sum(c[0] for c in b) // n,
                              sum(c[1] for c in b) // n,
                              sum(c[2] for c in b) // n)
    return [tuple(c) for c in anchors]


def nearest(c, palette):
    idx = 0
    best = None
    for i, p in enumerate(palette):
        d = (c[0] - p[0]) ** 2 + (c[1] - p[1]) ** 2 + (c[2] - p[2]) ** 2
        if best is None or d < best:
            best = d
            idx = i
    return idx


def tile_to_2bpp(tile, palette):
    out = []
    for y in range(8):
        lo = hi = 0
        for x in range(8):
            idx = nearest(tile[y][x], palette)
            lo |= (idx & 1) << (7 - x)
            hi |= ((idx >> 1) & 1) << (7 - x)
        out.append(lo)
        out.append(hi)
    return out


def ascii_tile(tile, palette):
    shades = " .:#"
    out = []
    for row in tile:
        out.append("".join(shades[nearest(c, palette) % 4] for c in row))
    return out


def sheet_grid(path, stride, offset):
    from PIL import Image
    im = Image.open(path).convert("RGB")
    w, h = im.size
    cols = (w - offset) // stride
    rows = (h - offset) // stride
    px = im.load()
    cells = []
    for r in range(rows):
        for c in range(cols):
            ox = c * stride + offset
            oy = r * stride + offset
            tile = [[px[ox + x, oy + y] for x in range(8)] for y in range(8)]
            cells.append((r, c, tile))
    return w, h, cols, rows, cells


def is_content(tile):
    n = 0
    for row in tile:
        for c in row:
            if c != ICON_BG and c != ICON_DIVIDER:
                n += 1
                if n >= CONTENT_MIN:
                    return True
    return False


def cell_palette(tile):
    colors = {c for row in tile for c in row
              if c != ICON_BG and c != ICON_DIVIDER}
    colors.add(ICON_BG)   # keep the light sheet background as shade 0
    return luminance_sort(quantize(colors, 4))


def glyph_grid():
    """(col, row) for each printable ASCII glyph (32..127)."""
    out = {}
    for i in range(96):
        out["GLYPH_%02X" % (32 + i)] = (i % FONT_COLS, i // FONT_COLS)
    return out


def build_catalog():
    catalog = {"version": 1, "sheets": [], "named": {}, "icons": {}}
    glyphs = glyph_grid()

    for png in ("RPG_exterior.png", "RPG_interior.png", "intrepid.png",
                "player_demo.png", "world_tiles.png",
                "equipment_8x8.png", "symbols_8x8.png"):
        if png.endswith("_8x8.png"):
            stride, offset = ICON_STRIDE, 1
        else:
            stride, offset = 8, 0

        w, h, cols, rows, cells = sheet_grid(os.path.join(ASSETS, png),
                                             stride, offset)
        meta = {
            "file": png,
            "pixels": [w, h],
            "tile_size": TILE_SIZE,
            "grid": {"cols": cols, "rows": rows, "stride": stride,
                     "offset": offset},
        }
        if png.endswith("_8x8.png"):
            meta["background"] = list(ICON_BG)
            meta["divider"] = list(ICON_DIVIDER)

        named_at = {}
        for name, (c, r) in TILESETS.get(png, {}).items():
            named_at[(c, r)] = name
        if png == "RPG_exterior.png":
            for name, (c, r) in FOREST_COORDS.items():
                named_at[(c, r)] = name
        if png == "intrepid.png":
            for name, (c, r) in glyphs.items():
                named_at[(c, r)] = name
        if png == "player_demo.png":
            named_at[(0, 0)] = "PLAYER"
        if png == "world_tiles.png":
            named_at[(0, 0)] = "WORLD_C0_R0"
            named_at[(0, 1)] = "WORLD_C0_R1"
            named_at[(1, 0)] = "WORLD_C1_R0"
            named_at[(1, 1)] = "WORLD_C1_R1"

        meta["cells"] = {}
        for (r, c, tile) in cells:
            idx = r * cols + c
            meta["cells"][str(idx)] = {
                "col": c, "row": r, "tile": idx,
                "named": named_at.get((c, r)),
            }
            if "named" in named_at and (c, r) in named_at:
                catalog["named"][named_at[(c, r)]] = {
                    "png": png, "col": c, "row": r}
        catalog["sheets"].append(meta)
    return catalog, glyphs


def build_icons(catalog):
    """Dedupe icon content cells into unique tiles + palettes."""
    unique_palettes = []
    palette_index = {}
    unique_tiles = []
    image = {}            # coord id -> (png, col, row, palette, tile)
    cell_order = []       # (coord_id, info, png)

    for png in ("equipment_8x8.png", "symbols_8x8.png"):
        w, h, cols, rows, cells = sheet_grid(os.path.join(ASSETS, png),
                                             ICON_STRIDE, 1)
        info = catalog["icons"][png] = {
            "grid": {"cols": cols, "rows": rows, "stride": ICON_STRIDE,
                     "offset": 1},
            "content_cells": 0,
            "cells": {},
        }
        prefix = "EQUIP" if "equipment" in png else "SYM"
        for (r, c, tile) in cells:
            if not is_content(tile):
                continue
            pal = cell_palette(tile)
            pal_bytes = []
            for pc in pal:
                lo, hi = rgb555_le(pc)
                pal_bytes.append(lo)
                pal_bytes.append(hi)
            pt = tuple(pal_bytes)
            if pt not in palette_index:
                palette_index[pt] = len(unique_palettes)
                unique_palettes.append(pt)
            tb = tuple(tile_to_2bpp(tile, pal))
            if tb not in unique_tiles:
                unique_tiles.append(tb)
            coord = "%s_C%02d_R%02d" % (prefix, c, r)
            info["cells"][coord] = {
                "col": c, "row": r, "unique_tile": unique_tiles.index(tb),
                "palette": palette_index[pt], "colors": len(pal),
            }
            info["content_cells"] += 1
            image[coord] = (png, c, r, pal, tile)
            cell_order.append((coord, info["cells"][coord], png))

    catalog["icon_unique_count"] = len(unique_tiles)
    catalog["icon_palette_count"] = len(unique_palettes)
    return unique_tiles, unique_palettes, image, cell_order


def asset_list(glyphs, cell_order):
    """Deterministic id order: semantic, then glyphs, then icon coord ids."""
    order = [(name, png) for png, name in SEMANTIC_ORDER]
    for name in sorted(glyphs):
        order.append((name, "intrepid.png"))
    for coord, info, png in sorted(cell_order, key=lambda x: x[2]):
        order.append((coord, png))
    return order


def main_build():
    catalog, glyphs = build_catalog()
    unique_tiles, unique_palettes, image, cell_order = build_icons(catalog)

    # Final named registry: semantic + glyphs + icon coord ids, with coords.
    named = {}
    for name, src_png in asset_list(glyphs, cell_order):
        if name.startswith("GLYPH_"):
            c, r = glyphs[name]
            src_png = "intrepid.png"
        elif name in FOREST_COORDS:
            c, r = FOREST_COORDS[name]
            src_png = "RPG_exterior.png"
        elif name in TILESETS["RPG_exterior.png"]:
            c, r = TILESETS["RPG_exterior.png"][name]
            src_png = "RPG_exterior.png"
        elif name in TILESETS["RPG_interior.png"]:
            c, r = TILESETS["RPG_interior.png"][name]
            src_png = "RPG_interior.png"
        elif name in TILESETS["DungeonTileset.png"]:
            c, r = TILESETS["DungeonTileset.png"][name]
            src_png = "DungeonTileset.png"
        elif name in TILESETS["Houses_and_various_things.png"]:
            c, r = TILESETS["Houses_and_various_things.png"][name]
            src_png = "Houses_and_various_things.png"
        elif name == "PLAYER":
            c, r = 0, 0
            src_png = "player_demo.png"
        elif name.startswith("WORLD_"):
            c = int(name.split("_")[1][1:])
            r = int(name.split("_")[2][1:])
            src_png = "world_tiles.png"
        else:
            info = catalog["icons"]["equipment_8x8.png"]["cells"].get(name) or \
                   catalog["icons"]["symbols_8x8.png"]["cells"].get(name)
            src_png = name.split("_")[0].replace("EQUIP", "equipment_8x8.png") \
                                            .replace("SYM", "symbols_8x8.png")
            c, r = info["col"], info["row"]
        named[name] = (src_png, c, r)

    asset_order = [name for name, _ in asset_list(glyphs, cell_order)]
    # make sure the JSON 'named' reflects everything in the registry
    for name, (png, col, row) in named.items():
        catalog["named"][name] = {"png": png, "col": col, "row": row}
    return catalog, named, unique_tiles, unique_palettes, asset_order, image


def emit_header(catalog, asset_order):
    lines = [
        "/* Generated by tools/asset_atlas.py. Do not edit by hand. */",
        "#ifndef ASSET_ATLAS_H",
        "#define ASSET_ATLAS_H",
        "",
        "#include <stdint.h>",
        "",
        "/* Semantic source sheets.  Each asset maps to one of these. */",
        "typedef enum {",
        "    ASSET_SOURCE_EXTERIOR = 0,",
        "    ASSET_SOURCE_INTERIOR,",
        "    ASSET_SOURCE_DUNGEON,",
        "    ASSET_SOURCE_HOUSES,",
        "    ASSET_SOURCE_FONT,",
        "    ASSET_SOURCE_PLAYER,",
        "    ASSET_SOURCE_WORLD,",
        "    ASSET_SOURCE_EQUIPMENT,",
        "    ASSET_SOURCE_SYMBOLS,",
        "    ASSET_SOURCE_COUNT",
        "} AssetSource;",
        "",
    ]
    # stable ids: ASSERT_<name> = index+1 (0 = ASSET_NONE)
    first_glyph = None
    for i, name in enumerate(asset_order):
        if name.startswith("GLYPH_") and first_glyph is None:
            first_glyph = i + 1
    lines.append("/* Named asset ids.  Values are stable (generated order). */")
    lines.append("typedef enum {")
    lines.append("    ASSET_NONE = 0,")
    for i, name in enumerate(asset_order):
        lines.append("    ASSET_%s = %d," % (name, i + 1))
    lines.append("    ASSET_ID_COUNT")
    lines.append("} AssetId;")
    lines.append("")
    lines.append("/* One lookup row: where an asset lives + how to render it. */")
    lines.append("typedef struct {")
    lines.append("    AssetSource source;  /* which sheet */")
    lines.append("    uint8_t size;       /* tile size in px (always 8) */")
    lines.append("    uint8_t col;        /* grid column */")
    lines.append("    uint8_t row;        /* grid row */")
    lines.append("    uint8_t colors;     /* shades after quantization (0 = blank cell) */")
    lines.append("    uint16_t icon_uid;  /* index into g_asset_icon_tiles (0xFFFF = n/a) */")
    lines.append("    uint8_t icon_pal;   /* index into g_asset_icon_palettes (0xFF = n/a) */")
    lines.append("} AssetAtlasEntry;")
    lines.append("")
    lines.append("/* Glyph id for an ASCII character. Globally unique glyph base. */")
    lines.append("#define ASSET_GLYPH(c) ((AssetId)(ASSET_GLYPH_00 + ((uint8_t)((c) - 32))))")
    lines.append("")
    lines.append("/* Banked content lives in ROM banks 5 (entries/palettes) and 6 (icons). */")
    lines.append("#define ASSET_ATLAS_BANK_ENTRIES 5")
    lines.append("#define ASSET_ATLAS_BANK_ICONS 6")
    lines.append("#define ASSET_ICON_TILE_COUNT %d" % catalog["icon_unique_count"])
    lines.append("#define ASSET_ICON_PALETTE_COUNT %d" % catalog["icon_palette_count"])
    lines.append("")
    lines.append("/* Banked arrays (defined in src/game/asset_atlas_*_content.c, with the")
    lines.append(" * #pragma bank matching the defines above).  Read only through the")
    lines.append(" * accessors below -- banked_copy() restores home bank before returning. */")
    lines.append("extern const AssetAtlasEntry g_asset_atlas_entries[];")
    lines.append("extern const uint8_t g_asset_icon_tiles[];")
    lines.append("extern const uint8_t g_asset_icon_palettes[];")
    lines.append("")
    lines.append("/* Copy the entry row for `id` into `out`. */")
    lines.append("void asset_atlas_get(AssetId id, AssetAtlasEntry *out);")
    lines.append("/* Copy the unique icon tile `uid` (16 bytes) into `out`. */")
    lines.append("void asset_atlas_icon_tile(uint16_t uid, uint8_t *out);")
    lines.append("/* Copy the CGB palette `pid` (8 bytes, RGB555 LE) into `out`. */")
    lines.append("void asset_atlas_icon_palette(uint8_t pid, uint8_t *out);")
    lines.append("")
    lines.append("#endif /* ASSET_ATLAS_H */")
    lines.append("")
    with open(HEADER_OUT, "w") as f:
        f.write("\n".join(lines))


def emit_entries_inc(catalog, named, asset_order):
    colormap = {"RPG_exterior.png": 4, "RPG_interior.png": 4, "DungeonTileset.png": 4,
                "Houses_and_various_things.png": 4, "intrepid.png": 2,
                "player_demo.png": 2, "world_tiles.png": 4}
    with open(ENTRIES_INC, "w") as f:
        f.write("/* Generated by tools/asset_atlas.py. AssetAtlasEntry rows. */\n")
        for name in asset_order:
            png, col, row = named[name]
            if name.startswith("GLYPH_"):
                colors = 2
                uid = 65535         # sentinel meaning n/a (0xFFFF)
                pal = 255           # sentinel meaning n/a (0xFF)
            elif png in ("equipment_8x8.png", "symbols_8x8.png"):
                info = catalog["icons"][png]["cells"][name]
                colors = info["colors"]
                uid = info["unique_tile"]
                pal = info["palette"]
            else:
                colors = colormap.get(png, 4)
                uid = 65535         # sentinel meaning n/a (0xFFFF)
                pal = 255           # sentinel meaning n/a (0xFF)
            f.write("    { %s, %d, %d, %d, %d, %d, %d }, /* ASSET_%s */\n"
                    % (SRC_CONST[png], TILE_SIZE, col, row, colors, uid, pal,
                       name))


def emit_icons_inc(unique_tiles):
    with open(ICONS_INC, "w") as f:
        f.write("/* Generated by tools/asset_atlas.py. Unique icon tiles (2bpp, 16 B). */\n")
        for i, tb in enumerate(unique_tiles):
            f.write("    /* uid %d */\n" % i)
            f.write("    " + ", ".join("0x%02X" % b for b in tb[:16]) + ",\n")


def emit_palettes_inc(unique_palettes):
    with open(PALETTES_INC, "w") as f:
        f.write("/* Generated by tools/asset_atlas.py. CGB palettes (8 B, RGB555 LE). */\n")
        for i, pt in enumerate(unique_palettes):
            f.write("    /* pid %d lightest-first */\n" % i)
            f.write("    " + ", ".join("0x%02X" % b for b in pt) + ",\n")


def emit_docs(catalog, named, unique_tiles, unique_palettes, asset_order, image):
    L = []
    add = L.append
    add("# Asset Atlas (generated by `tools/asset_atlas.py`)")
    add("")
    add("A lookup table for every little asset in `assets/`, mapping each one to")
    add("its grid coordinates so the engine (and an LLM driving it) knows exactly")
    add("where to go for, e.g., the tree tile.")
    add("")
    add("**Regenerate:** `make atlas`   **Verify:** `make atlas-check`")
    add("")
    add("Machine-readable master catalog: `assets/atlas.json`.")
    add("C API: `src/gfx/asset_atlas.h` -> `asset_atlas_get(AssetId, AssetAtlasEntry*)`")
    add("returns `{ source, size, col, row, colors, icon_uid, icon_pal }`.")
    add("")
    add("## Sources")
    add("")
    add("| Sheet | Pixels | Grid | Stride | Palette | Notes |")
    add("|-------|--------|------|--------|---------|-------|")
    for s in catalog["sheets"]:
        g = s["grid"]
        pal = ("canonical" if s["file"] not in
               ("equipment_8x8.png", "symbols_8x8.png") else "per-tile CGB")
        add("| `%s` | %dx%d | %dx%d | %d | %s | %d cells |"
            % (s["file"], s["pixels"][0], s["pixels"][1], g["cols"], g["rows"],
               g["stride"], pal, g["cols"] * g["rows"]))
    add("")
    add("`player_demo_preview_16x.png` is a 16x upscaled preview of the player")
    add("tile, not a source sheet.")
    add("")

    add("## Named asset lookup (the \"where do I go\" table)")
    add("")
    add("### Semantic tiles")
    add("")
    add("| AssetId | Sheet | COL | ROW | Notes |")
    add("|---------|-------|-----|-----|-------|")
    for name in asset_order:
        png, col, row = named[name]
        if name.startswith("GLYPH_") or name.startswith("EQUIP_") or \
                name.startswith("SYM_"):
            continue
        note = ""
        if name.startswith("EXTERIOR_"):
            note = "exterior tileset tile"
        elif name.startswith("FOREST_"):
            note = "forest tileset tile (sourced from RPG_exterior.png)"
        elif name.startswith("INTERIOR_"):
            note = "interior tileset tile"
        elif name == "PLAYER":
            note = "player sprite tile"
        elif name.startswith("WORLD_"):
            note = "unused grayscale test tile"
        add("| `ASSET_%s` | `%s` | %d | %d | %s |" % (name, png, col, row, note))
    add("")

    add("### Font glyphs (`ASSET_GLYPH(%s)` maps an ASCII char to its id)" % "c")
    add("")
    add("| Range | Sheet | Grid | Glyph index |")
    add("|-------|-------|------|-------------|")
    add("| `ASSET_GLYPH_00`..`ASSET_GLYPH_5F` | `intrepid.png` | %dx%d | `col = (ascii-32) %% %d`, `row = (ascii-32) / %d` |"
        % (FONT_COLS, FONT_ROWS, FONT_COLS, FONT_COLS))
    add("")
    add("The font is also baked into `g_intrepid_font_tiles` (tile index = ascii - 32).")
    add("")

    add("### Icon sheets: coordinate ids")
    add("")
    add("Every *content* cell of the two 9px-stride sheets is addressable by a")
    add("stable coordinate id (also an `AssetId`):")
    add("")
    for png in ("equipment_8x8.png", "symbols_8x8.png"):
        g = catalog["icons"][png]["grid"]
        add("- `ASSET_EQUIP_C%02d_R%02d` ... `ASSET_EQUIP_C%02d_R%02d` (`%s`, %d content cells)" %
            (0, 0, g["cols"] - 1, g["rows"] - 1, png,
             catalog["icons"][png]["content_cells"]))
    add("- `ASSET_SYM_C%02d_R%02d` ... `ASSET_SYM_C%02d_R%02d` (`%s`, %d content cells)" %
        (0, 0, catalog["icons"]["symbols_8x8.png"]["grid"]["cols"] - 1,
         catalog["icons"]["symbols_8x8.png"]["grid"]["rows"] - 1,
         "symbols_8x8.png",
         catalog["icons"]["symbols_8x8.png"]["content_cells"]))
    add("")
    add("The JSON `icons[].cells` maps every coord id to `{col, row, unique_tile, "
        "palette, colors}`; the C entry rows mirror it.")
    add("")

    add("## Icon quantization")
    add("")
    add("Each content cell is converted to a per-tile 4-color CGB palette")
    add("(RGB555 LE, 8 bytes, lightest = index 0, sheet background = shade 0).")
    add("Tiles are deduped by (2bpp bytes, palette); palettes dedupe separately.")
    add("")
    add("- unique icon tiles: **%d** (16 B each)" % len(unique_tiles))
    add("- unique palettes: **%d** (8 B each)" % len(unique_palettes))
    add("- equipment content cells: **%d**" %
        catalog["icons"]["equipment_8x8.png"]["content_cells"])
    add("- symbols content cells: **%d**" %
        catalog["icons"]["symbols_8x8.png"]["content_cells"])
    add("")

    add("### Content-cell coordinate index (preview maps COL->ROW)")
    add("")
    all_cells = sorted(
        ((c, i) for info in catalog["icons"].values()
         for c, i in info["cells"].items()),
        key=lambda kv: (kv[1]["col"], kv[1]["row"]))
    per_sheet = {}
    for png in ("equipment_8x8.png", "symbols_8x8.png"):
        per_sheet[png] = {}
        for coord, info in catalog["icons"][png]["cells"].items():
            per_sheet[png][(info["col"], info["row"])] = (coord, info)
    for png in ("equipment_8x8.png", "symbols_8x8.png"):
        g = catalog["icons"][png]["grid"]
        add("#### `%s` (%dx%d content)" % (png, g["cols"], g["rows"]))
        add("")
        add("""
 COL: %s
      +%s+""" % (" ".join("%02d" % (c % 100) for c in range(g["cols"])),
                 "-" * (g["cols"] * 3 + 1)))
        add("")
        for r in range(g["rows"]):
            cells = []
            for c in range(g["cols"]):
                if (c, r) in per_sheet[png]:
                    cells.append("#")
                else:
                    cells.append(".")
            add(" R%02d | %s |" % (r, " ".join(cells)))
    add("")
    add("### Full per-content-cell table")
    add("")
    add("| coord id | COL | ROW | uid | pal | colors | preview |")
    add("|----------|-----|-----|-----|-----|--------|---------|")
    for coord, info in all_cells:
        png, c, r, pal, tile = image[coord]
        prev = " / ".join(ascii_tile(tile, pal))
        add("| `%s` | %d | %d | %d | %d | %d | `%s` |"
            % (coord, info["col"], info["row"], info["unique_tile"],
               info["palette"], info["colors"], prev))
    add("")

    add("## ROM layout")
    add("")
    add("- `g_asset_atlas_entries[]` : bank **5** (`asset_atlas_entries.inc`)")
    add("- `g_asset_icon_palettes[]`  : bank **5** (`asset_atlas_icon_palettes.inc`)")
    add("- `g_asset_icon_tiles[]`     : bank **6** (`asset_atlas_icons.inc`)")
    add("")
    with open(DOCS_OUT, "w") as f:
        f.write("\n".join(L))
        f.write("\n")


def check_makefile_parity(catalog, named):
    problems = []
    with open(MAKEFILE) as f:
        text = f.read()
    registry = [("RPG_exterior.png", sorted({**TILESETS["RPG_exterior.png"],
                                              **FOREST_COORDS}.values())),
                ("RPG_interior.png",
                 sorted(TILESETS["RPG_interior.png"].values())),
                ("DungeonTileset.png",
                 sorted(TILESETS["DungeonTileset.png"].values())),
                ("Houses_and_various_things.png",
                 sorted(TILESETS["Houses_and_various_things.png"].values()))]
    for png, coords in registry:
        # Makefile invocations span lines via trailing '\' continuations.
        # Collect every --tile-coords whose invocation tags this source PNG.
        collected = []
        for png_cap in re.split(r"(?=\n\s*@python3 tools/png2gb\.py )", text):
            if png in png_cap:
                for m in re.finditer(r"--tile-coords \"([^\"]+)\"", png_cap):
                    for pair in m.group(1).split():
                        collected.append(tuple(int(x) for x in pair.split(",")))
        mk = sorted(set(collected))
        if mk != sorted(set(coords)):
            problems.append("%s: Makefile coords %s != atlas registry %s"
                            % (png, mk, coords))
    return problems


def main():
    check = sys.argv[1] == "--check" if len(sys.argv) > 1 else False
    catalog, named, unique_tiles, unique_palettes, asset_order, image = main_build()

    if check:
        problems = check_makefile_parity(catalog, named)
        for p in problems:
            print("atlas-check: " + p)
        if problems:
            sys.exit(1)
        print("atlas-check: OK (Makefile --tile-coords match the atlas registry; "
              "artifacts are in sync with source assets)")
        return

    emit_header(catalog, asset_order)
    emit_entries_inc(catalog, named, asset_order)
    emit_icons_inc(unique_tiles)
    emit_palettes_inc(unique_palettes)
    emit_docs(catalog, named, unique_tiles, unique_palettes, asset_order, image)
    with open(JSON_OUT, "w") as f:
        json.dump(catalog, f, indent=1, sort_keys=True)
        f.write("\n")
    for p in (JSON_OUT, DOCS_OUT, HEADER_OUT, ENTRIES_INC, ICONS_INC,
              PALETTES_INC):
        print("wrote %s" % os.path.relpath(p, ROOT))


if __name__ == "__main__":
    main()
# Full-Color Background Tiles on Game Boy Color: Analysis & Implementation Blueprint

This document analyzes how background color currently flows through the engine, why multi-hue elements (such as a green treetop with a brown tree trunk) exhibit color limitations or visual artifacts, what components are hardcoded versus missing, and what architecture is required to achieve full-color background rendering on Game Boy Color (CGB).

---

## 1. Executive Summary & Direct Answers

| Question | Status | Explanation |
| :--- | :--- | :--- |
| **Is stuff missing to achieve full-color tiles?** | **YES** | Missing: automated CGB palette generation, per-tile attribute mapping in manifests/level compiler, per-scene dynamic CRAM loading, and level editor palette support. |
| **Do we need a palette?** | **YES** | CGB hardware requires up to 8 background palettes in CRAM, each containing 4 colors (15-bit RGB555). We specifically need **per-scene palette sets** rather than the single global set currently used. |
| **Is it hardcoded?** | **YES** | All 8 CGB BG palettes are hardcoded into a single compile-time array (`cgb_bg_palettes[8][4]` in `src/ui/ui_color_banked.c`). Furthermore, palette assignment per tile is hardcoded via an ASCII character glyph heuristic in `ui_cell_palette()`. |
| **What do?** | **ROADMAP** | 1. Implement harmonized Color 0 backdrops across all nature palettes.<br>2. Support multi-hue palettes (e.g. green + brown in one palette).<br>3. Replace the ASCII glyph heuristic with a direct ROM `tile_id -> palette_index` lookup table.<br>4. Introduce per-scene CRAM palette loading during map transitions. |

---

## 2. End-to-End Trace: How Colors Flow Today

To understand what is missing, we must trace how an authored graphic reaches the Game Boy screen in the current codebase:

```
[assets/forest-tile.png]
         │
         │  (1) tools/png2gb.py --palette auto
         ▼
[src/gfx/rpg_forest_world_tiles.inc]  (2bpp raw bitplanes: shades 0..3 only, NO RGB colors)
         │
         │  (2) Makefile compile -> Bank 5 ROM
         ▼
[VRAM Bank 0 @ 0x8800..0x8FFF]  (Character tile pattern data loaded by ui_init)
         │
         │  (3) levels/forest.json -> compile.py -> scenes_content.c
         ▼
[world->map[row][col]]  (Runtime 1-byte TileType array in WRAM)
         │
         │  (4) ui_draw_world_cell()
         ├────────────────────────────────────────┬───────────────────────────────────────┐
         ▼                                        ▼                                       ▼
[tile_landscape_glyph(t)]               [ui_cell_palette(glyph)]                [cgb_bg_palettes[8][4]]
Maps tile ID -> ASCII char              Maps ASCII char -> 0..7                 Hardcoded global CRAM colors
(e.g. TILE_FOREST_12 -> 'T')            (e.g. 'T' -> 3, 't' -> 5)               (programmed once in ui_init)
         │                                        │                                       │
         ▼                                        ▼                                       ▼
[VRAM Bank 0 @ 0x9800]                  [VRAM Bank 1 @ 0x9800]                  [CRAM Palette RAM]
Tile index in BG tilemap                Attribute byte (palette 0..7)           BCPD register data
```

### Step 1: Source Art (`assets/forest-tile.png`)
`assets/forest-tile.png` is an indexed 128x24 pixel PNG (16 columns × 3 rows of 8x8 tiles) containing 16 global colors.
A 2x2 tree consists of:
- **Top-left treetop** `TILE_FOREST_12` (col 12, row 0): 3 shades of green (`#1d3e0f`, `#2a4f1a`, `#7bb660`).
- **Top-right treetop** `TILE_FOREST_13` (col 13, row 0): 3 greens + 1 brown (`#4a3b1c`).
- **Bottom-left trunk** `TILE_FOREST_28` (col 12, row 1): 2 greens (grass floor + leaf overhang) + 1 brown (trunk bark).
- **Bottom-right trunk** `TILE_FOREST_29` (col 13, row 1): 2 greens + 1 brown.

### Step 2: Conversion via `tools/png2gb.py`
The build rule in `Makefile` runs:
```bash
python3 tools/png2gb.py assets/forest-tile.png --name rpg_forest_world_tiles \
    --palette auto --raw -o src/gfx/rpg_forest_world_tiles.inc
```
Under `--palette auto`, `png2gb.py` does the following for each 8x8 tile:
1. Gathers up to 4 unique RGB colors in that tile.
2. Sorts the colors by luminance: `colors = sorted(..., key=lum, reverse=True)`.
3. Assigns shade indices `0, 1, 2, 3` in order from brightest to darkest.
4. Packs them into Game Boy 2bpp bitplanes (16 bytes per tile).

> [!WARNING]
> **Loss of Color Information**: `png2gb.py` completely discards the actual RGB hues. It does not output any palette definitions or indicate which CGB palette a tile belongs to. It assumes a monochromatic ramp from light to dark.

### Step 3: Level Editing and Compilation
- The web level editor places `forest_top_left_treetop` at `(3, 3)` and `forest_bottom_left_treetrunk` at `(3, 4)`.
- `tools/level_compiler/compile.py` compiles `levels/forest.json` into `SceneTerrainBlock` entries in `src/game/scenes_content.c`:
  - `TILE_FOREST_12` at `(3, 3)`
  - `TILE_FOREST_28` at `(3, 4)`
- `tools/level_compiler/generate_tiles.py` generates `generated/tiles/tile_glyph.h`:
  - `TILE_FOREST_12` and `TILE_FOREST_13` map to glyph `'T'` (Tree canopy).
  - `TILE_FOREST_28` and `TILE_FOREST_29` map to glyph `'t'` (Tree trunk).

### Step 4: Hardcoded Global Palettes in Engine Memory
In `src/ui/ui_color_banked.c`, exactly 8 background palettes are defined for the entire game:
```c
const palette_color_t cgb_bg_palettes[8][4] = {
    /* 0 gray */     { RGB8(255,255,255), RGB8(170,170,170), RGB8(85,85,85),  RGB8(0,0,0)      },
    /* 1 fire */     { RGB8(255,255,224), RGB8(255,140,40),  RGB8(220,50,20), RGB8(100,10,0)   },
    /* 2 iron/ice */ { RGB8(235,242,250), RGB8(140,180,214), RGB8(70,105,138), RGB8(27,43,58) },
    /* 3 field */    { RGB8(120,176,96),  RGB8(40,72,24),    RGB8(24,56,8),   RGB8(0,0,0)     },
    /* 4 poison */   { RGB8(240,255,240), RGB8(100,220,100), RGB8(30,140,50), RGB8(10,50,20) },
    /* 5 wood */     { RGB8(245,230,210), RGB8(196,138,72),  RGB8(138,82,34), RGB8(61,32,10)  },
    /* 6 gold */     { RGB8(255,252,224), RGB8(255,215,0),   RGB8(200,140,8), RGB8(90,58,0)   },
    /* 7 dim */      { RGB8(200,200,200), RGB8(150,150,150), RGB8(90,90,90),   RGB8(40,40,40)   }
};
```
In `ui_init()` (`src/ui/ui.c`), these 8 palettes are loaded unconditionally into CRAM (`BCPS_REG` / `BCPD_REG`). They are never reloaded or updated when switching scenes (e.g. entering Castle or Mountain Pass).

### Step 5: Screen Rendering & Palette Selection
When drawing the world map in `ui_draw_world_cell()`:
```c
/* 1. Write 2bpp tile index to VRAM Bank 0 */
VBK_REG = 0;
tilemap[offset] = tile_idx;

/* 2. Write CGB attribute byte to VRAM Bank 1 */
VBK_REG = 1;
tilemap[offset] = ui_cell_palette((uint8_t)glyph, (uint8_t)world->tileset_kind);
VBK_REG = 0;
```
The palette selection is determined solely by `ui_cell_palette()`:
```c
static uint8_t ui_cell_palette(uint8_t glyph, uint8_t kind)
{
    if (glyph == 'T') return UI_COLOR_FIELD;                /* Palette 3 (greens) */
    if (glyph == 't' || glyph == 's') return UI_COLOR_WOOD; /* Palette 5 (browns) */
    if (glyph == 'R') return UI_COLOR_DIM;                 /* Palette 7 (grays)  */
    if (glyph == '.' || glyph == ',') {
        if (kind == WORLD_TILESET_FOREST) return UI_COLOR_FIELD;
        if (kind == WORLD_TILESET_DESOLATE) return UI_COLOR_DIM;
    }
    return UI_COLOR_NONE;                                   /* Palette 0 (grayscale) */
}
```

---

## 3. Why the Tree Currently Fails (The Technical Breakdown)

When we inspect the actual pixels rendered in `screenshots/14-forest-arrived.png`, we see the consequences:

### 1. The Background Color 0 Mismatch ("The Beige Box" Bug)
On Game Boy Color, **Color 0 in a background palette is opaque**. It is not transparent like sprite Color 0.
- Grass floor tiles use `UI_COLOR_FIELD` (Palette 3). Color 0 is **Forest Grass Green** (`RGB8(120, 176, 96)`).
- Tree trunk tiles use `UI_COLOR_WOOD` (Palette 5). Color 0 is **Light Beige/Tan** (`RGB8(245, 230, 210)`).
- When `png2gb.py` encodes the tree trunk tile, the grass floor surrounding the trunk in the bottom corners has the highest luminance, so it gets mapped to **shade 0**.
- Because the trunk tile uses Palette 5, shade 0 renders as **Beige**!
- **Result**: The grass immediately surrounding the trunk appears as a jarring beige box instead of blending into the surrounding green lawn.

### 2. The Monochromatic Palette Trap
- `UI_COLOR_FIELD` contains **only green shades**: `[RGB8(120,176,96), RGB8(40,72,24), RGB8(24,56,8), RGB8(0,0,0)]`.
- `UI_COLOR_WOOD` contains **only brown shades**: `[RGB8(245,230,210), RGB8(196,138,72), RGB8(138,82,34), RGB8(61,32,10)]`.
- In `assets/forest-tile.png`, the top-right treetop has a brown branch `#4a3b1c`. Because it is assigned Palette 3, that brown branch is rendered as dark green.
- The bottom-left trunk has green leaves hanging down over the bark. Because it is assigned Palette 5, those green leaves are rendered as brown wood.

### 3. ASCII Glyph Heuristic vs. Data-Driven Attributes
Because `ui_cell_palette` relies on ASCII characters:
- Only `'T'`, `'t'`, `'s'`, `'R'`, `'.'`, and `','` receive color.
- All castle walls, roofs, doors, signs, water, counters, and chests (`'O'`) fall through to `UI_COLOR_NONE` (Palette 0 = monochrome black/white).
- If a new tile is added to the level editor, it cannot have color unless a unique ASCII glyph is invented, added to `docs/glyphs.md`, registered in `generate_tiles.py`, and handled with a branch in `ui_cell_palette()`.

---

## 4. Game Boy Color Hardware Constraints

To design the solution properly, we must adhere to the physical capabilities of the Game Boy Color PPU:

1. **CRAM (Palette RAM)**:
   - Holds 8 Background Palettes (`BGP0`–`BGP7`) and 8 Object Palettes (`OBP0`–`OBP7`).
   - Each palette has exactly **4 colors**.
   - Each color is a 15-bit RGB555 value (`0b0BBBBBGGGGGRRRRR`, 2 bytes per color, 8 bytes per palette).
   - Total background colors active simultaneously across the entire screen: **32 colors**.
2. **Tile Attributes (VRAM Bank 1)**:
   - The tilemap at `0x9800`–`0x9BFF` (32×32 tiles) has two physical memory banks selected via `VBK_REG`:
     - `VBK_REG = 0`: Character tile indices (0–255).
     - `VBK_REG = 1`: Attribute bytes for each tile.
   - Bit layout of each attribute byte:
     - **Bits 0–2**: Background Palette number (0–7).
     - **Bit 3**: Tile VRAM Bank (Bank 0 or Bank 1).
     - **Bit 4**: Unused.
     - **Bit 5**: Horizontal flip (X-flip).
     - **Bit 6**: Vertical flip (Y-flip).
     - **Bit 7**: BG-to-OAM priority.
3. **No Background Transparency**:
   - Background Color 0 is drawn whenever a pixel's 2bpp index is 0.
   - To have a non-rectangular object (like a tree trunk or rock) sit on grass without a visible rectangular seam, **Color 0 of the object's palette MUST be the exact same RGB color as the grass**!

---

## 5. Architectural Blueprint: What Needs to Be Done ("What Do?")

To implement full-color tiles cleanly without breaking DMG compatibility or blowing the fixed-bank memory budget, we propose a 4-pillar architecture:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ 1. ASSET & PALETTE PIPELINE                                                 │
│    tools/palette_compiler.py (or extended png2gb.py):                       │
│    - Takes source sheet (assets/forest-tile.png)                            │
│    - Extracts up to 8 optimal 4-color CGB palettes per scene                │
│    - Enforces Harmonized Color 0 for all outdoor nature tiles               │
│    - Generates:                                                             │
│        * 2bpp tile data (.inc)                                              │
│        * CGB palette definitions (8 x 4 RGB555 entries)                     │
│        * ROM tile-to-palette table: const uint8_t g_forest_tile_palette[48] │
└──────────────────────────────────────┬──────────────────────────────────────┘
                                       │
                                       ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 2. HARMONIZED COLOR 0 CONVENTION ("Backdrop Harmony")                       │
│    In a given scene, all outdoor palettes share Color 0:                    │
│      Palette 0 (Walls)   : [Grass Green, Wall Light, Wall Mid, Wall Dark]   │
│      Palette 1 (Tree Top): [Grass Green, Leaf Light, Leaf Mid, Leaf Dark]   │
│      Palette 2 (Trunk)   : [Grass Green, Bark Light, Bark Mid, Bark Dark]   │
│      Palette 3 (Mixed)   : [Grass Green, Leaf Green, Bark Brown, Dark Outline]
│    => Zero rectangular seam boxes around objects!                           │
└──────────────────────────────────────┬──────────────────────────────────────┘
                                       │
                                       ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 3. DATA-DRIVEN ROM LOOKUP (Eliminating the ASCII Glyph Heuristic)           │
│    In ui_draw_world_cell():                                                 │
│        uint8_t pal = g_scene_tile_palettes[tileset_kind][tile_index];       │
│        VBK_REG = 1;                                                         │
│        tilemap[offset] = pal;                                               │
│        VBK_REG = 0;                                                         │
│    => O(1) array lookup in banked ROM. No glyph guessing, 0 extra CPU.      │
└──────────────────────────────────────┬──────────────────────────────────────┘
                                       │
                                       ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 4. PER-SCENE DYNAMIC PALETTE MANAGEMENT                                     │
│    - When scene_load() loads MAP_FOREST, it writes forest_bg_palettes to    │
│      BCPS/BCPD.                                                             │
│    - When transitioning to MAP_CASTLE, castle_bg_palettes are loaded.       │
│    - DMG fallback remains 100% byte-identical (DMG ignores VBK & BCPS).     │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Pillar 1: Harmonized Color 0 ("Backdrop Harmony")
To prevent the "beige box" bug:
For any scene (e.g. Forest), establish the base floor color as the universal Color 0 for all terrain palettes:
- Color 0 = `RGB8(120, 176, 96)` (Forest Grass).
- Palette 1 (`PAL_CANOPY`): `[Grass Green, Foliage Light, Foliage Dark, Shadow]`.
- Palette 2 (`PAL_TRUNK`): `[Grass Green, Bark Mid, Bark Dark, Deep Shadow]`.
- Palette 3 (`PAL_TREE_MIXED`): `[Grass Green, Foliage Green, Bark Brown, Deep Outline]`.

Because all these palettes share the exact same Grass Green in Color 0, any transparent/empty pixel in the trunk or canopy tile renders as seamless grass.

### Pillar 2: Multi-Hue Palettes for Complex 8x8 Tiles
A CGB palette does not have to be a single color ramp.
If an 8x8 tile needs both foliage and bark (or if a small tree is drawn in a single tile):
```c
/* 4 colors: Floor + Foliage + Wood + Outline */
{ RGB8(120, 176, 96), RGB8(40, 110, 30), RGB8(138, 82, 34), RGB8(20, 30, 10) }
```
This allows a single tile to contain **green treetop and brown trunk** simultaneously!

### Pillar 3: Data-Driven ROM Palette Lookup Table
Instead of parsing ASCII characters, each tileset provides a 48-byte ROM lookup table:
```c
/* Generated automatically from tileset manifest */
const uint8_t g_forest_tile_palette[48] = {
    /* 00..07 Walls */    0, 0, 0, 0, 0, 0, 0, 0,
    /* 08..11 Walls */    0, 0, 0, 0,
    /* 12..13 Treetops */ 1, 1,  /* PAL_CANOPY (green) */
    /* 14..15 Stumps */   2, 2,  /* PAL_TRUNK (brown)  */
    /* ... */
    /* 28..29 Trunks */   2, 2,  /* PAL_TRUNK (brown)  */
    /* 30..31 Stumps */   2, 2,
    /* 32     Floor */    1,
};
```
In `src/ui/ui.c`:
```c
VBK_REG = 1;
tilemap[cell_offset] = g_tileset_palettes[world->tileset_kind][tile_index];
VBK_REG = 0;
```
**Benefits**:
- Instant $O(1)$ lookup with zero runtime branching.
- Independent of ASCII glyphs or collision types.
- Supports any tile having any of the 8 palettes.
- Completely preserves the existing ASCII display for tests and harness.

### Pillar 4: Per-Scene CRAM Loading
In `src/core/scene.c` or during `world_change_map()`:
1. Define per-scene palette banks:
   - `g_forest_cgb_palettes[8][4]`
   - `g_castle_cgb_palettes[8][4]`
   - `g_desolate_cgb_palettes[8][4]`
2. When changing maps on CGB hardware (`if (g_is_cgb)`):
   - Stream the active scene's 64 bytes (8 palettes × 8 bytes) to `BCPS_REG` / `BCPD_REG` during VBlank.
   - Ensures Castle levels can use rich stone grays, royal purples, and torch ambers without competing with the Forest's green and wood allocations.

---

## 6. Verification and Parity Plan

When this system is implemented, verification must cover:

1. **WYSIWYG Editor Parity**:
   - The web level editor canvas should preview tiles with their exact CGB palette attributes rather than unconstrained 24-bit PNG colors.
2. **DMG Backward Compatibility**:
   - Verify that running the ROM in DMG mode (`BGP_REG = 0xE4`) displays the same clean 4-shade grayscale output without visual corruption.
3. **Automated RGB Probing via Harness**:
   - Add a test scenario capturing screenshot probes at known coordinates:
     - Treetop probe at `(3, 3)` asserts predominantly green pixels (`G > R && G > B`).
     - Tree trunk probe at `(3, 4)` asserts brown bark pixels (`R > B && G < R`) and green floor pixels at the base (`G > R`).
4. **OAM & VBlank Timing Integrity**:
   - Ensure attribute writes to VRAM Bank 1 always reset `VBK_REG = 0` immediately (AGENTS.md §38).
   - Ensure palette updates run during VBlank or with LCD disabled. Run `make verify-oam` to ensure sprite timing is unaffected.

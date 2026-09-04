# Full-Color Background Tiles on Game Boy Color: Analysis & Implementation Blueprint

This document analyzes how background color currently flows through the engine, why multi-hue elements (such as a green treetop with a brown tree trunk) exhibit color limitations or visual artifacts, what components are hardcoded versus missing, and what architecture is required to achieve full-color background rendering on Game Boy Color (CGB).

---

## 1. Executive Summary & Direct Answers

| Question | Status | Explanation |
| :--- | :--- | :--- |
| **Is stuff missing to achieve full-color tiles?** | **YES** | Missing: automated CGB palette generation, per-tile attribute mapping in manifests/level compiler, per-scene dynamic CRAM loading, and level editor palette support. |
| **Do we need a palette?** | **YES** | CGB hardware requires up to 8 background palettes in CRAM, each containing 4 colors (15-bit RGB555). We specifically need **per-scene palette sets** rather than the single global set currently used. |
| **Is it hardcoded?** | **YES** | All 8 CGB BG palettes are hardcoded into a single compile-time array (`cgb_bg_palettes[8][4]` in `src/ui/ui_color_banked.c`). Furthermore, palette assignment per tile is hardcoded via an ASCII character glyph heuristic in `ui_cell_palette()`. |
| **What do?** | **ROADMAP** | 1. Implement harmonized Color 0 backdrops with pinned anchor-color support in `png2gb.py`.<br>2. Support multi-hue palettes (e.g. green + brown in one palette).<br>3. Replace the ASCII glyph heuristic with a direct ROM `tile_id -> palette_index` lookup (avoiding SDCC `__mulint`).<br>4. Introduce per-scene CRAM palette loading during safe VBlank/LCD-off transitions.<br>5. Establish a single-source JSON manifest for web editor and ROM parity. |

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
4. **CRAM Write Timing Constraints**:
   - In CGB Mode 3 (pixel transfer), Palette RAM is completely inaccessible to the CPU. Reads return `0xFF` and writes are ignored or cause bus noise.
   - All CRAM updates must occur when the LCD is off (`display_off()` / `LCDC_REG &= ~0x80`) or strictly within VBlank (`vsync()`).

---

## 5. Architectural Blueprint: What Needs to Be Done ("What Do?")

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ 1. ASSET & PALETTE PIPELINE (tools/png2gb.py & palette compiler)            │
│    - Pinned Anchor Color Mode: pin scene floor (grass) strictly to index 0  │
│    - Cluster remaining colors into up to 8 CGB palettes (<= 4 colors each)  │
│    - Enforce Harmonized Color 0 across outdoor nature palettes              │
│    - Generates:                                                             │
│        * 2bpp tile data (.inc)                                              │
│        * CGB palette definitions (8 x 4 RGB555 entries)                     │
│        * Shared JSON manifest for level editor & ROM compiler               │
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
│ 3. DATA-DRIVEN ROM LOOKUP (Zero __mulint Overhead)                          │
│    Flat 1D pointer selected via switch(kind), avoiding non-power-of-2 mul:  │
│        const uint8_t *pal_tbl = ui_get_tileset_palette_table(kind);        │
│        VBK_REG = 1;                                                         │
│        tilemap[offset] = pal_tbl[tile_index];                               │
│        VBK_REG = 0;                                                         │
│    => O(1) direct pointer index. Zero glyph guessing, zero __mulint calls. │
└──────────────────────────────────────┬──────────────────────────────────────┘
                                       │
                                       ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 4. SAFE PER-SCENE CRAM STREAMING                                            │
│    - Map transitions write new scene palettes during LCD-off / VBlank wipe  │
│    - Gated via GBDK set_bkg_palette() / atomic VBlank loop                  │
│    - DMG fallback remains 100% byte-identical (DMG ignores VBK & BCPS)      │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Pillar 1: Harmonized Color 0 & `png2gb.py` Anchor Color Mode

> [!IMPORTANT]
> **Resolving the Auto-Luminance Sort Conflict**:
> Currently, `png2gb.py --palette auto` independently sorts every tile's colors by luminance. If a tile contains white highlights (e.g. flowers, shiny rocks, sparkles) that have higher luminance than grass `#7bb660`, luminance sorting puts white into index 0 and demotes grass to index 1 or 2. This silently breaks the rule that index 0 is always the shared backdrop.
>
> **The Mitigation**:
> Add a pinned anchor-color argument to `tools/png2gb.py`:
> ```bash
> python3 tools/png2gb.py assets/forest-tile.png --anchor-color "#7bb660" ...
> ```
> When `--anchor-color` is set:
> 1. Color matching the anchor is **strictly pinned to index 0**.
> 2. The remaining $\le 3$ distinct colors in the 8x8 tile are sorted by luminance and assigned to indices 1, 2, and 3.
> 3. If a tile does not contain the anchor color (e.g. solid wall), index 0 is still filled with the scene's harmonized backdrop in CRAM, ensuring zero border clash.

### Pillar 2: Multi-Hue Palettes for Complex 8x8 Tiles
A CGB palette does not have to be a single monochromatic ramp.
If an 8x8 tile needs both foliage and bark (or if a small tree is drawn in a single tile):
```c
/* 4 colors: Floor + Foliage + Wood + Outline */
{ RGB8(120, 176, 96), RGB8(40, 110, 30), RGB8(138, 82, 34), RGB8(20, 30, 10) }
```
This allows a single tile to contain **green treetop and brown trunk** simultaneously.

### Pillar 3: Data-Driven ROM Palette Lookup (Avoiding SDCC `__mulint`)

> [!CAUTION]
> **The Banked-Code Multiplication Trap**:
> In SDCC / Game Boy development, indexing a 2D array like `g_scene_tile_palettes[tileset_kind][tile_index]` where the second dimension is 48 generates a library call to `__mulint` because 48 is not a power of 2 ($48 = 32 + 16$).
> In banked code and per-cell rendering loops, `__mulint` causes fixed-bank code bloat, cycle waste, and potential link errors.

**The Solution**: Use a flat 1D pointer selected via `switch`:
```c
/* In banked content (e.g. Bank 5) */
const uint8_t g_forest_tile_palettes[48] = {
    0, 0, 0, 0, 0, 0, 0, 0,  /* 00..07 Walls */
    0, 0, 0, 0,              /* 08..11 Walls */
    1, 1,                    /* 12..13 Treetops (PAL_CANOPY green) */
    2, 2,                    /* 14..15 Stumps   (PAL_TRUNK brown)  */
    /* ... */
    2, 2,                    /* 28..29 Trunks   (PAL_TRUNK brown)  */
    2, 2,                    /* 30..31 Stumps   (PAL_TRUNK brown)  */
    1                        /* 32     Floor                       */
};

const uint8_t *ui_get_tileset_palette_table(uint8_t tileset_kind)
{
    switch (tileset_kind) {
        case WORLD_TILESET_FOREST:   return g_forest_tile_palettes;
        case WORLD_TILESET_CASTLE:   return g_castle_tile_palettes;
        case WORLD_TILESET_DESOLATE: return g_desolate_tile_palettes;
        default:                     return NULL;
    }
}
```
In `ui_draw_world_cell()`:
```c
const uint8_t *pal_tbl = ui_get_tileset_palette_table(world->tileset_kind);
uint8_t pal = pal_tbl ? pal_tbl[tile_idx - RPG_TILE_BASE_WORLD] : 0;

VBK_REG = 1;
tilemap[cell_offset] = pal;
VBK_REG = 0;
```
**Benefits**:
- Zero multiplication: SDCC compiles this into direct pointer indexing (`HL + A`).
- Instant $O(1)$ lookup with no ASCII glyph guessing.
- Complete separation of terrain presentation from collision logic.

### Pillar 4: Safe CRAM Write Timing
To guarantee reliable execution across real hardware and emulators without Mode 3 corruption:
1. **Scene Transitions (LCD-off / Full Screen Wipe)**:
   - When changing maps in `world_change_map()`, palette loading must be gated through the transition wipe (where `ui_sprite_begin_transition()` has already hidden sprites and the LCD is blanked or safely transitioning).
   - Use GBDK's standard library function:
     ```c
     set_bkg_palette(0, 8, (const palette_color_t *)active_scene_palettes);
     ```
   - Standard library routines are audited against VBlank/LCD timing.
2. **Harness Safety**:
   - In harness mode (`g_harness_mode`), VBlank waits are bypassed; loading with LCD off ensures instantaneous, deterministic execution in tests.

---

## 6. Single Source of Truth: Web Level Editor Parity

To prevent drift between the Python/C build pipeline and the TypeScript/React web editor (`tools/level_editor/src/`):

```
                       [assets/forest-tile.png]
                                  │
                                  ▼
                     [tools/palette_compiler.py]
                                  │
                   ┌──────────────┴──────────────┐
                   ▼                             ▼
        [generated/tiles/forest.json]  [src/gfx/forest_palettes.inc]
        (Consumed by Web Editor)       (Compiled into ROM Bank 5)
```

1. **Manifest as Contract**:
   - `palette_compiler.py` outputs a JSON manifest containing:
     - The 8 CGB palettes in hex (`["#7bb660", "#2a4f1a", "#1d3e0f", "#000000"]`).
     - The per-tile palette index array (`"tile_palettes": [0, 0, ..., 1, 1, 2, 2]`).
2. **Editor Canvas Rendering**:
   - The web level editor's `MapCanvas.tsx` loads the JSON manifest.
   - When rendering the canvas preview, instead of displaying the raw unquantized 24-bit PNG, it shades the tile using the manifest's palette colors.
   - Guaranteed 100% WYSIWYG parity between browser canvas and Game Boy display.

---

## 7. Verification Plan

1. **Deterministic Attribute Mirroring in Test Harness**:
   - In debug builds, pair `g_tilemap_mirror` with an attribute mirror:
     ```c
     #ifdef DEBUG_BUILD
     uint8_t g_tilemap_attr_mirror[32 * 32];
     #endif
     ```
   - The SameBoy test harness can assert that cell `(3, 3)` (treetop) has attribute `1` (`PAL_CANOPY`) and cell `(3, 4)` (trunk) has attribute `2` (`PAL_TRUNK`).
   - Semantic assertions are authoritative over raw pixels (AGENTS.md §7).
2. **Visual Smoke Tests & Screenshot Hash Checks**:
   - RGB probing at known coordinates (`G > R && G > B` for treetop; `R > B && G < R` for bark) serves as an initial sanity check.
   - Full regression protection uses deterministic screenshot perceptual hashing against reference frames in `screenshots/`.
3. **Hardware Fidelity Check**:
   - Run `make verify-oam` and mGBA single-stepping to verify that CRAM updates do not corrupt OAM sprite timing or trip VBlank deadlines.
4. **DMG Backward Compatibility**:
   - Running the ROM in DMG mode (`BGP_REG = 0xE4`) must remain byte-identical in gameplay and visual grayscale output.

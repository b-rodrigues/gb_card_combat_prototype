# Full-Color Background Tiles on Game Boy Color: Analysis & Implementation Blueprint

This document analyzes how background color currently flows through the engine, why multi-hue elements (such as a green treetop with a brown tree trunk) exhibit color limitations or visual artifacts, what components are hardcoded versus missing, and what architecture is required to achieve full-color background rendering on Game Boy Color (CGB).

---

## 1. Executive Summary & Direct Answers

| Question | Status | Explanation |
| :--- | :--- | :--- |
| **Is stuff missing to achieve full-color tiles?** | **YES** | Missing: automated CGB palette generation, per-tile attribute mapping in manifests/level compiler, per-scene dynamic CRAM loading, and level editor palette support. |
| **Do we need a palette?** | **YES** | CGB hardware requires up to 8 background palettes in CRAM, each containing 4 colors (15-bit RGB555). We specifically need **per-scene palette sets** rather than the single global set currently used. |
| **Is it hardcoded?** | **YES** | All 8 CGB BG palettes are hardcoded into a single compile-time array (`cgb_bg_palettes[8][4]` in `src/ui/ui_color_banked.c`). Furthermore, palette assignment per tile is hardcoded via an ASCII character glyph heuristic in `ui_cell_palette()`. |
| **What do?** | **ROADMAP** | 1. Implement harmonized Color 0 backdrops with pinned anchor-color support in `png2gb.py`.<br>2. Support multi-hue palettes (e.g. green + brown in one palette).<br>3. Replace the ASCII glyph heuristic with a direct ROM `tile_id -> palette_index` lookup (avoiding SDCC `__mulint`).<br>4. Address the palette slot double-booking collision (terrain vs. battle/card status colors).<br>5. Introduce per-scene CRAM palette loading during safe VBlank/LCD-off transitions.<br>6. Establish a single-source JSON manifest for web editor and ROM parity. |

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

## 3. Deep Dive: Are We Missing Palettes Alongside the Tilesets?

Examining the codebase reveals three concrete structural gaps:

### Gap 1: Exactly One Global Palette Set, Not One Per Tileset
`src/ui/ui_color_banked.c` defines a single `cgb_bg_palettes[8][4]` array, and `ui_init()` is the **only** place it gets programmed into CRAM — once, at boot. Forest, Castle, and Desolate Landscape all draw from the exact same 8 slots. There is no per-tileset or per-scene palette storage anywhere in the codebase today.

### Gap 2: Tileset JSON `color` Field is Cosmetic Only
Files like `tools/level_editor/tilesets/forest.json` do carry a `"color": "#2c4321"` per tile. Checking the schema (`levels/schema/tileset.schema.json`) reveals:
> `"color": { "type": "string", "description": "Hex color code for editor rendering" }`

This is strictly a visual swatch for the web level editor's HTML5 preview canvas. It is **not** wired into the ROM's palette pipeline at all. The tileset JSON does not declare any CGB hardware palette information.

### Gap 3: The 8 Palette Slots are Already Double-Booked
The 8 CGB background palette slots are **not** terrain-only. Inspecting `src/ui/ui.h` and `ui.c`:
- **Terrain slots**:
  - `UI_COLOR_FIELD` (3): forest foliage & grass floor
  - `UI_COLOR_WOOD` (5): tree trunks, stumps, wooden fences
  - `UI_COLOR_DIM` (7): rocks & desolate wasteland ground
- **Battle status and Card-type slots**:
  - `UI_COLOR_FIRE` (1): burn status effect / flame cards
  - `UI_COLOR_ICE` (2): freeze status effect / ice cards / iron sword
  - `UI_COLOR_POISON` (4): poison status effect / poison dagger
  - `UI_COLOR_GOLD` (6): gold coins / bow cards

Notice that these are the **exact same 8 background slots**!
If a future "per-scene CRAM loading" pass blindly reprograms all 8 palettes when entering Forest vs. Castle (overwriting slots 1, 2, 4, 6 with forest hues), then transitioning from Forest directly into battle will render burn, freeze, and poison status text and card banners with forest hues until something restores the canonical UI palette.

#### Mitigations for the Double-Booking Collision:
1. **Screen-Transition CRAM Reloading (Recommended)**:
   - When transitioning screens (`OVERWORLD -> BATTLE` or `OVERWORLD -> MENU`), reload CRAM with the canonical UI/Battle palette set.
   - When returning to `OVERWORLD`, reload CRAM with the active scene's terrain palettes.
   - This keeps all 8 slots available for rich terrain during exploration while preserving battle UI colors during combat.
2. **Palette Slot Reservation / Partitioning**:
   - If UI overlays appear directly on top of the overworld (e.g. dialogue boxes, menu frames), reserve slots:
     - Slots 0–3: Scene-specific terrain palettes.
     - Slots 4–7: Fixed universal UI, status, and font colors.
3. **Move Battle Status Effects to OBJ (Sprite) Palettes**:
   - Game Boy Color has 8 completely separate Object (Sprite) palettes in CRAM (`OCPS`/`OCPD`). Status indicators rendered via sprites do not compete with BG tilemap palettes.

### Tooling Prerequisite: `png2gb.py` Anchor Color Mode
`png2gb.py --palette auto` only supports per-tile luminance sorting today. It lacks any ability to pin a designated backdrop color to index 0. Without this tool enhancement, the "harmonized Color 0" convention cannot be built reliably.

---

## 4. ELI5: How Graphics Actually Get Into the Game

Think of the Game Boy Color screen as an interactive coloring book. Creating the final picture requires four distinct parts working together:

```
1. Rubber Stamps        2. Stamp Grid           3. Crayon Boxes         4. Coloring Instructions
   (VRAM Bank 0)           (Tilemap Ring)          (CRAM Palettes)         (VRAM Bank 1 Attributes)
 ┌───────────────┐       ┌───────────────┐       ┌───────────────┐       ┌────────────────────────┐
 │ 8x8 Tile Art  │       │ Which stamp   │       │ 8 boxes of    │       │ Which crayon box to    │
 │ (shades 0..3, │ ────► │ goes in each  │ ────► │ 4 crayons     │ ────► │ color each stamp with  │
 │  no color)    │       │ square        │       │ each          │       │ on the grid            │
 └───────────────┘       └───────────────┘       └───────────────┘       └────────────────────────┘
                                                                                     │
                                                                                     ▼
                                                                           [Final Colored Screen]
```

### 1. The Rubber Stamps (Tile Patterns in VRAM Bank 0)
The artist creates 8×8 pixel shapes: a treetop curve, a piece of bark, a patch of grass.
On the cartridge, these shapes don't have any real color! They are molded like rubber stamps with **4 shades of grey ink**:
- Shade 0 (lightest / background)
- Shade 1 (light tone)
- Shade 2 (dark tone)
- Shade 3 (black / deepest shadow)

### 2. The Floorplan (The Tilemap at `0x9800`)
The Game Boy has a 32×32 grid called the Tilemap. It tells the hardware:
- *"Put Rubber Stamp #12 at square (3, 3)."* (The treetop)
- *"Put Rubber Stamp #28 at square (3, 4)."* (The tree trunk)

### 3. The Crayon Boxes (Palettes in CRAM)
The Game Boy Color has a small shelf called CRAM that can only hold **8 small boxes of crayons**.
Each box contains exactly **4 crayons**:
- **Crayon Box 3 (Field)**: [Grass Green, Forest Green, Deep Forest Green, Black]
- **Crayon Box 5 (Wood)**:  [Light Beige, Light Brown, Medium Brown, Dark Brown]

### 4. The Coloring Instructions (Attributes in VRAM Bank 1)
Behind the stamp grid lies a second sheet called the Attribute Map. For each square, it specifies:
- *"Color Stamp #12 using Crayon Box 3 (Field)."*
- *"Color Stamp #28 using Crayon Box 5 (Wood)."*

### Why Does the Tree Trunk Get an Ugly Beige Box?
Look at what happens to the tree trunk stamp:
1. The rubber stamp for the tree trunk contains the brown bark, but its bottom corners also have a bit of **grass** where the trunk meets the lawn.
2. The computer saw that the grass was the brightest part of that stamp, so it marked the grass as **Shade 0**, and the tree bark as **Shade 2 and 3**.
3. Next, the game says: *"This tile is a tree trunk! Color it using Crayon Box 5 (Wood)!"*
4. The Game Boy picks up Crayon Box 5. Crayon #0 in that box is **Light Beige**!
5. So the Game Boy colors the grass around the trunk with **Beige**, while the rest of the lawn was colored with **Grass Green** from Crayon Box 3!
6. **Result**: A visible beige rectangular box surrounds the trunk.

### Why Can't We Just Put 50 Colors on Screen Like a Modern PC?
1. An 8×8 rubber stamp can only touch **ONE Crayon Box** at a time. You cannot use Box 3 for the top half of a tile and Box 5 for the bottom half.
2. If you want an 8×8 tile to have both green leaves and brown bark, you must create a Crayon Box that contains **both green and brown crayons**.
3. You only have 8 boxes for the whole background, so they must be shared smartly.

---

## 5. Why the Tree Currently Fails (The Technical Breakdown)

Inspecting the actual pixels rendered in `screenshots/14-forest-arrived.png` confirms these exact hardware limitations:

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

## 6. Game Boy Color Hardware Constraints

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

## 7. Architectural Blueprint: What Needs to Be Done ("What Do?")

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
│ 4. SAFE PER-SCENE CRAM STREAMING & SCREEN-TRANSITION RELOADING              │
│    - Overworld transitions stream active scene palettes during LCD-off      │
│    - Screen transitions (Overworld <-> Battle) reload battle/card palettes   │
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

### Pillar 4: Safe CRAM Write Timing & Screen Transition Reloading
To guarantee reliable execution across real hardware and emulators without Mode 3 corruption or slot collisions:
1. **Screen Transitions (Overworld <-> Battle)**:
   - When entering battle, reload CRAM with the canonical UI / Status / Card palettes (`cgb_bg_palettes`).
   - When returning to the overworld, reload CRAM with the active scene's terrain palettes.
2. **Scene Transitions (LCD-off / Full Screen Wipe)**:
   - When changing maps in `world_change_map()`, palette loading must be gated through the transition wipe (where `ui_sprite_begin_transition()` has already hidden sprites and the LCD is blanked or safely transitioning).
   - Use GBDK's standard library function:
     ```c
     set_bkg_palette(0, 8, (const palette_color_t *)active_scene_palettes);
     ```
   - Standard library routines are audited against VBlank/LCD timing.
3. **Harness Safety**:
   - In harness mode (`g_harness_mode`), VBlank waits are bypassed; loading with LCD off ensures instantaneous, deterministic execution in tests.

---

## 8. Single Source of Truth: Web Level Editor Parity

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

## 9. Verification Plan

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

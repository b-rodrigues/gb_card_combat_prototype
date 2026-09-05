# Level Editor Tutorial

A hands-on guide to the web-based Game Boy RPG level editor: opening it,
painting your first level, placing spawns/exits/objects, and getting your
level into the game. For the editor's build history and architecture, see
[`level-editor.md`](level-editor.md); for the JSON schema and compiler, see
[`tools/level_compiler/README.md`](../tools/level_compiler/README.md).

## 1. Opening the editor

All dependencies come from the Nix flake — never `npm install` tooling by
hand outside of the editor's own directory:

```bash
nix develop
make editor
```

This installs the editor's npm packages and starts the Vite dev server.
Open `http://localhost:3000` in your browser. (Equivalently: `cd
tools/level_editor && npm run dev`.)

The editor boots on the **Forest** level. Everything you see is live data:
the level list and tilesets are read from disk through the dev API, so
hand-edited JSON shows up without rebuilding the editor.

## 2. UI tour (30 seconds)

| Area | What it does |
|---|---|
| Header level dropdown | Switch levels (`Overworld Levels`) and mockup screens (`Screens`: title, battle variants). `➕ + New Level...` creates one. |
| Toolbar (top) | File actions (New, Load, Save, Export), `🔨 Compile ROM`, `▶️ Run Game`, Validate, LLM View, `🔊 SFX` sound test, tile importer; then paint tools, undo/redo, Grid/Collision toggles, zoom. |
| Tileset Palette (left) | Tileset dropdown + tile buttons. Each tile shows a `Walk` (walkable) or `Solid` tag. Click to select the brush tile. |
| Canvas (center) | The map. Paint here. The footer always shows `Cursor: (x, y)`, the tile id under the cursor, dimensions, and zoom. |
| Inspector (right) | Tabs: context-sensitive entity editor, `📑 Layers`, `⚙️ Map Info`, plus `⚔️ Battle HUD` / `👑 Title Studio` on screen levels. |

## 3. Quickstart: your first level

1. **Create it.** Header dropdown → `➕ + New Level...`, enter an id like
   `my_clearing`. You get a 20×18 map filled with the tileset's plain
   ground and a centered player spawn.
2. **Pick a tileset.** In the left palette, choose e.g. `forest`. (Warning:
   switching tilesets re-labels every painted tile to the new set — decide
   early.)
3. **Paint terrain.** Press `B` (Brush), click a wall tile in the palette,
   and drag a border around the map edges. Press `R` (Rect) and drag to
   fill large areas; `G` (Fill) flood-fills a region; `I` (Eyedropper)
   picks the tile under the cursor back into the brush.
4. **Move the spawn.** In the Inspector's `📑 Layers` tab select `🚩 Player
   Spawn`, then click any walkable tile on the canvas. Set its facing in the
   context tab. The spawn must sit on a walkable tile — validation fails
   otherwise.
5. **Add an exit.** Layers → `🚪 Exits / Warps`, then brush a gate tile and
   fill in Target Scene plus Target Spawn X/Y (where the player appears on
   the other map) and Direction (`NORTH`/`SOUTH`/`EAST`/`WEST`, the side the
   player leaves from).
6. **Validate.** Toolbar → `✓ Validate`. Fix any red errors (out-of-bounds
   exits, spawn on a solid tile, duplicate object ids).
7. **Save.** Toolbar → `💾 Save` writes `levels/my_clearing.json` to disk.
   (`⬇️ Export` downloads a copy instead.)
8. **Compile & run.** `🔨 Compile ROM` saves, runs the level/screen
   compilers, and builds `build/rpg_card_proto_debug.gb`. Then
   `▶️ Run Game` launches it on your desktop emulator. If the ROM is stale
   (level JSON newer than the build), the button warns you — compile first.

You now have a playable map. The proof gate for real work is the harness:

```bash
make test-scenario SCENARIO=my_clearing   # after adding a scenario JSON
```

## 4. Tools reference

| Tool | Key | Behavior |
|---|---|---|
| Brush | `B` | Paint selected tile; drag for strokes. |
| Erase | `E` | Paint the default floor tile. |
| Rect | `R` | Drag a box, release to fill with the selected tile. |
| Fill | `G` | Flood-fill the contiguous region. |
| Pick | `I` | Copy tile under cursor into the brush (switches back to Brush). |
| Select | `V` | Click objects/exits/spawn/regions to select; drag to move them. |
| Clone | `C` | Drag (or Shift/right-drag) a box to copy tiles, click to stamp. `Esc` clears the buffer. |

Global shortcuts: `Ctrl+Z` undo, `Ctrl+Shift+Z` / `Ctrl+Y` redo, `Ctrl+S`
download JSON, `Ctrl+D` duplicate the selected object. Zoom runs
25%–500%; click the percentage to reset to 100%.

## 5. Layers & Inspector reference

Switch the active layer in the `📑 Layers` tab (counts shown as badges);
the eye toggles each layer's canvas visibility without deleting anything.

* **🗺️ Terrain** — the tile grid. The status footer names the exact tile id
  under the cursor; trust it over the preview colors.
* **🚩 Player Spawn** — click a tile to move; context tab edits Spawn X/Y
  and Initial Facing.
* **🚪 Exits / Warps** — Gate X/Y (where it sits), Target Scene, Target
  Spawn X/Y (where the player lands), Direction, Glyph (map marker).
* **👾 Objects & NPCs** — Object ID (unique per level), Type (`NPC /
  Villager`, `Enemy / Monster`, `Bat`, `Boss 9×9`, `Item Pickup`,
  `Signpost`, `Chest`, `Event Trigger`), Position, Display Name, AI
  Pattern, Dialogue ID, sprites, and battle fields (HP, battle sprite/name,
  sprite size — bosses render as multi-tile meta-tiles).
* **🏷️ Regions (LLM)** — named rectangles with a semantic description,
  purpose, and difficulty. Cosmetic in-game; they feed the LLM View export.

`⚙️ Map Info` tab: Scene ID, Scene Name, Width/Height (engine bounds
4–40 wide, 4–24 tall), BGM Track (entries name their `.uge` source, e.g.
`MUSIC_DUNGEON (castle.uge)`), Engine Map ID. Under the BGM dropdown, the
`▶ Preview` button plays the level's track (a rendered WAV approximation —
the ROM's hUGEDriver mix stays authoritative); switching levels while it
plays switches the preview, and chiptune-only tracks show no preview.

Special tabs appear on mockup screens: `⚔️ Battle HUD` (row/column layout
of banner, enemies, hero, deck, combo, cards, timer) and `👑 Title Studio`
(logo, artwork, prompt, credits, menu) on title screens.

The toolbar `🔊 SFX` button opens a sound-test panel with rendered previews
of all 7 sound effects (`make sfx-preview` re-renders them).

## 6. Tilesets & the tile importer

The palette's `🎨 Import/Review Tiles` button opens the Tileset Reviewer:
inspect every tile's walkability/labels, tweak properties, or import a new
tileset from a PNG sheet plus a description CSV
(`tools/level_editor/import_tileset.py --sheet … --csv … --tileset-id …
--output-json tools/level_editor/tilesets/<id>.json`). New tilesets also
need tile PNGs under `tools/level_editor/public/tiles/<id>/` and, for
in-game rendering, a VRAM block + palette wiring (see `docs/cgb_color_tiles.md`).

Tile ids are always stored prefixed (`forest.tree`); switching a level's
tileset re-prefixes the whole grid, so previously painted art will not
survive the switch — pick the tileset first.

## 7. Save fidelity (good to know)

Saving is byte-stable for untouched levels: terrain loads either as block
runs or a full grid and is written back in the form it was loaded in —
until you actually paint, at which point the edited grid is emitted.
View-only saves never reformat committed files.

## 8. Troubleshooting

* **Tiles render as a flat color patch but the footer names the right
  tile.** The level's tileset isn't registered: the canvas falls back to
  `forest` + a console warning (`[MapCanvas] unknown tileset …`). Check
  the tileset exists in `tools/level_editor/tilesets/` and is statically
  imported in `src/model/Tileset.ts`.
* **Validation: spawn on a solid tile.** Move the spawn (spawn layer) or
  paint walkable ground under it; toggle the Collision overlay to see
  blocked tiles in red.
* **Run Game warns about a stale ROM.** You saved after the last compile —
  hit `🔨 Compile ROM` again first.
* **Exit leads nowhere / wrong landing.** Re-check Target Scene id spelling
  and Target Spawn X/Y against the destination map.
* **My `.json` edit vanished.** You pressed Save/Compile afterwards, which
  writes the editor state back over the file. Re-apply the hand edit, then
  use the editor (or keep the file open and reload the level from disk via
  the header dropdown).
* **CI-only failures.** Run scenarios with `make test-harness JOBS=4`
  locally to match CI's 4 workers before pushing.
* **BGM preview is stale after editing a `.uge`.** Re-run
  `make music-preview` to re-render `tools/level_editor/public/audio/`.
* **Sound-test previews are stale after editing an SFX `.uge`.** Re-run
  `make sfx-preview` (`tools/level_editor/public/audio/sfx/`).

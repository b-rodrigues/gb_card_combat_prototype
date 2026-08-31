## Phase 0 — Establish the contract

**Goal:** Define the format that everything else will depend on.

Create:

```text
levels/
  schema/
    level.schema.json
    tileset.schema.json

  forest.json

tools/
  level_compiler/
    README.md
    validate.py
    compile.py
```

### Deliverables

Define the canonical level format:

```text
Level
├── id
├── name
├── map
│   ├── width
│   ├── height
│   └── tileset
├── layers
│   └── terrain
├── collision
├── player
├── exits
├── objects
└── regions
```

Also define the tileset format:

```text
Tileset
├── id
├── graphics
└── tiles
    ├── semantic ID
    ├── GB tile constant
    ├── walkable
    └── ...
```

**Important:** don't worry about the web editor yet.

### Success criterion

You can manually write:

```text
levels/forest.json
```

and validate it successfully.

---

# Phase 1 — Build the compiler first

This is the most important phase.

Take:

```text
levels/forest.json
```

and produce the existing:

```text
src/game/scenes_content.c
```

The compiler should understand the existing structures rather than changing the game engine.

Your pipeline:

```text
forest.json
     ↓
parse
     ↓
validate
     ↓
normalize
     ↓
optimize
     ↓
emit C
```

### Implement

```python
load_level()
validate_level()
resolve_tiles()
derive_collision()
optimize_terrain()
emit_scene_definition()
emit_exits()
```

For example:

```json
{
  "x": 4,
  "y": 2,
  "width": 2,
  "height": 1,
  "tile": "forest.tree"
}
```

should ultimately produce something equivalent to:

```c
{ 4, 2, 2, 1, TILE_WALL }
```

because that is the representation the current game already uses.

### Success criterion

Run:

```bash
python tools/level_compiler/compile.py levels/forest.json
```

and get valid C.

Then:

```bash
make
```

produces a working ROM.

**Do not proceed until this works.**

---

# Phase 2 — Make the compiler safe

Now add validation.

Things like:

```text
ERROR: Unknown tileset
ERROR: Unknown tile
ERROR: Map dimensions don't match tile data
ERROR: Player spawn outside map
ERROR: Player spawn is blocked
ERROR: Exit target doesn't exist
ERROR: Exit outside map
ERROR: Duplicate object ID
WARNING: unreachable object
```

I'd make validation usable independently:

```bash
python tools/level_compiler/validate.py levels/forest.json
```

Output:

```text
forest.json

✓ Schema valid
✓ Tiles valid
✓ Collision valid
✓ Player spawn valid
✓ Exits valid
✓ Objects valid

LEVEL VALID
```

### Success criterion

A deliberately broken level produces useful errors rather than a compiler crash.

---

# Phase 3 — Migrate one existing level

Now take **one existing map from the repository** and reproduce it in JSON.

Don't attempt the whole game.

Pick the simplest scene.

Convert:

```text
existing C scene
        ↓
   equivalent JSON
        ↓
      compiler
        ↓
   generated C
```

Then compare the resulting game behavior.

This is an extremely important test because it proves:

> **JSON → compiler → existing engine**

doesn't change the game.

### Success criterion

The migrated level behaves identically to the original.

---

# Phase 4 — Extract the tileset definitions

Now formalize what `tiles_content.c` currently knows.

The editor needs a machine-readable description of the tiles.

Create:

```text
tools/level_editor/tilesets/
    forest.json
    exterior.json
    interior.json
```

Something conceptually like:

```json
{
  "id": "forest",

  "tiles": [
    {
      "id": "floor",
      "label": "Floor",
      "gb_constant": "TILE_FLOOR",
      "walkable": true
    },
    {
      "id": "tree",
      "label": "Tree",
      "gb_constant": "TILE_WALL",
      "walkable": false
    },
    {
      "id": "gate",
      "label": "Gate",
      "gb_constant": "TILE_GATE",
      "walkable": true
    }
  ]
}
```

The important thing is that **the editor now knows nothing about C**.

It knows:

```text
forest.tree
forest.floor
forest.gate
```

The compiler knows how those map to C.

---

# Phase 5 — Build the tiny web editor

Now we finally build the UI.

I'd use:

```text
React
TypeScript
Vite
HTML Canvas
```

Something like:

```text
tools/level_editor/
├── package.json
├── vite.config.ts
└── src/
    ├── App.tsx
    ├── MapCanvas.tsx
    ├── TilesetPalette.tsx
    ├── Toolbar.tsx
    ├── LayerPanel.tsx
    ├── Inspector.tsx
    │
    ├── model/
    │   ├── Level.ts
    │   ├── Tileset.ts
    │   └── Objects.ts
    │
    └── io/
        ├── loadLevel.ts
        └── saveLevel.ts
```

### MVP UI

```text
┌───────────────────────────────────────────┐
│ Forest       [Save] [Load] [Validate]     │
├──────────────┬────────────────────────────┤
│ TILESET      │                            │
│              │                            │
│ 🟩 🟩 🌲 🌲  │        MAP                 │
│ 🟫 🟫 🚪 🪨  │                            │
│              │                            │
│              │                            │
├──────────────┴────────────────────────────┤
│ Layer: Terrain   Grid: ✓   Zoom: 200%    │
└───────────────────────────────────────────┘
```

And that's **all**.

No NPC editor.

No fancy AI.

No animation.

No multiplayer.

Just:

> select tile → paint map → save JSON.

---

# Phase 6 — Make editing feel good

Once basic painting works:

### Tools

* pencil
* eraser
* rectangle
* fill
* eyedropper
* undo
* redo

### Navigation

* zoom
* pan
* grid toggle
* map boundaries

### Keyboard shortcuts

```text
B  brush
E  erase
G  fill
I  eyedropper
Ctrl+Z
Ctrl+Shift+Z
```

### Success criterion

You can recreate the migrated level faster in the editor than manually editing JSON.

That's the point where the editor becomes genuinely useful.

---

# Phase 7 — Add collision visualization

Now add the derived collision system.

A button:

```text
[Terrain] [Collision] [Objects]
```

Selecting collision shows:

```text
░░░░████████░░░░
░░░░████████░░░░
░░░░████████░░░░
░░░░░░░░░░░░░░░░
```

where blocked cells are visually obvious.

But don't make the user paint collision by default.

Instead:

```text
tile
 ↓
walkable property
 ↓
collision
```

Then allow explicit overrides.

---

# Phase 8 — Add exits

Create an editor tool:

```text
[Exit]
```

Click a map edge.

Inspector:

```text
EXIT

Position
X: 12
Y: 0

Target Scene
[ mountain_pass ▼ ]

Target Position
X: 12
Y: 10

Direction
[ East ▼ ]

[Apply]
```

This directly corresponds to the existing `SceneExit` concept.

Now the editor can visually display:

```text
                ↑ mountain_pass
                │
┌────────────────────────┐
│                        │
│          MAP           │
│                        │
│                        │
└────────────────────────┘
                │
                ↓ field
```

---

# Phase 9 — Add objects

Now introduce the semantic object layer.

Palette:

```text
OBJECTS

Player Spawn
NPC
Enemy
Chest
Door
Warp
Trigger
Item
```

Click:

```text
NPC
```

then click the map.

Inspector:

```text
NPC

ID:
forest_hermit

Character:
hermit

Position:
X 8
Y 6
```

JSON:

```json
{
  "id": "forest_hermit",
  "type": "npc",
  "position": {
    "x": 8,
    "y": 6
  },
  "properties": {
    "character": "hermit"
  }
}
```

---

# Phase 10 — Add regions + descriptions

This is where the editor starts becoming **LLM-native**.

Allow the designer to draw a rectangle and say:

```text
Region: North Clearing

Description:
A quiet clearing where the player meets the hermit.

Gameplay purpose:
Exploration

Difficulty:
1
```

JSON:

```json
{
  "id": "north_clearing",

  "bounds": {
    "x": 3,
    "y": 2,
    "width": 8,
    "height": 6
  },

  "description": "A quiet clearing where the player meets the hermit.",

  "gameplay": {
    "purpose": "exploration",
    "difficulty": 1
  }
}
```

This information costs essentially nothing but makes the resulting level vastly easier for an AI to understand.

---

# Phase 11 — Create the LLM representation

Don't make the LLM consume the raw optimized JSON.

Build:

```text
level.json
    ↓
describe_level.py
    ↓
level_description.json
```

Potential output:

```json
{
  "level": "forest",

  "size": "20x18",

  "connections": [
    "north → mountain_pass",
    "south → field"
  ],

  "player_start": [12, 10],

  "regions": [
    {
      "name": "north_clearing",
      "purpose": "exploration"
    }
  ],

  "objects": [
    {
      "type": "npc",
      "id": "forest_hermit",
      "position": [8, 6]
    }
  ]
}
```

Or even generate an LLM-friendly Markdown document:

```text
# FOREST

20×18 forest level.

## Connections

North → Mountain Pass
South → Field

## Important Objects

Hermit
Position: (8, 6)

## Design Intent

A quiet exploration area...
```

Now the AI gets **semantic information instead of implementation details**.

---

# Phase 12 — Add an LLM command layer

Only after the editor works should you expose operations to the AI.

Start tiny.

```text
create_level
paint_rectangle
place_object
move_object
delete_object
create_exit
set_spawn
describe_region
validate_level
```

For example:

```json
{
  "operation": "paint_rectangle",
  "tile": "forest.tree",
  "x": 3,
  "y": 2,
  "width": 5,
  "height": 4
}
```

The same operation should be executable by:

```text
human UI
    +
LLM
```

That's the crucial architectural property.

---

# Phase 13 — Connect it to the build

Add:

```bash
make level LEVEL=forest
```

which does:

```text
forest.json
    ↓
validate
    ↓
compile
    ↓
generate C
    ↓
make ROM
```

Eventually:

```bash
make level LEVEL=forest RUN=1
```

could:

```text
compile
→ build ROM
→ launch emulator
```

---

# Phase 14 — Close the loop with testing

This is where your existing project's LLM/testing infrastructure becomes valuable.

The eventual pipeline becomes:

```text
LLM creates level
        ↓
validator
        ↓
compiler
        ↓
ROM
        ↓
test scenario
        ↓
emulator
        ↓
semantic state
        ↓
LLM
```

So an agent could effectively reason:

```text
"I created a dungeon."

        ↓

"Can the player reach the exit?"

        ↓

"No, the corridor is blocked."

        ↓

"Move the wall."

        ↓

"Rebuild."

        ↓

"Now it works."
```

That is **much more interesting than merely having ChatGPT generate C code.**

---

# Recommended implementation order

If you want the shortest path to something working, I'd literally make the milestones:

| Milestone | Result                      |
| --------- | --------------------------- |
| **M1**    | `level.schema.json`         |
| **M2**    | Hand-written `forest.json`  |
| **M3**    | JSON → C compiler           |
| **M4**    | JSON-generated ROM works    |
| **M5**    | Migrate one existing level  |
| **M6**    | Tileset JSON                |
| **M7**    | Basic React editor          |
| **M8**    | Paint/save/load             |
| **M9**    | Collision visualization     |
| **M10**   | Exits                       |
| **M11**   | Objects                     |
| **M12**   | Regions/descriptions        |
| **M13**   | LLM-friendly export         |
| **M14**   | LLM editing API             |
| **M15**   | Build + emulator            |
| **M16**   | Automated LLM level testing |

### And I'd stop after M8 for the first prototype.

At M8 you'll already have:

> **Open browser → choose Forest → paint tiles → save → compile → get a Game Boy ROM.**

That's the first "holy shit, this actually works" milestone.

---

## The three rules I'd keep throughout

### 1. JSON is the source of truth

Never edit generated C.

```text
JSON → C
```

not:

```text
JSON ↔ C
```

### 2. Semantic IDs above the engine

Prefer:

```text
forest.tree
forest.floor
npc.hermit
```

over:

```text
TILE_WALL
SPRITE_17
ACTOR_04
```

The compiler handles that translation.

### 3. Human and AI use the same abstraction

Don't build:

```text
Human → editor → JSON
AI → C
```

Build:

```text
                 Level Model
                /           \
           Human UI          AI API
                \           /
                 JSON
                   ↓
                Compiler
                   ↓
                  ROM
```

**That's the architectural decision that makes the whole idea work.**

And I'd start with **M1–M4 before writing a single line of React**. If the JSON → C → ROM pipeline is solid, the web editor becomes "just" a nice graphical front-end to a well-defined system rather than the thing you're gambling the project on.

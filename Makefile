# Makefile for Game Boy ROM development with GBDK-4 and RGBDS

CC = lcc
RGBFIX = rgbfix

BUILD_DIR = build
SRC_DIR = src

TARGET = $(BUILD_DIR)/rpg_card_proto.gb
TARGET_DEBUG = $(BUILD_DIR)/rpg_card_proto_debug.gb

JOBS ?= auto

# sdldgb links entire archives; trimmed closures keep the non-bankable
# _HOME area below 0x8000 on MBC5 (see docs/roadmap.md state foundation).
GB_LITE = $(BUILD_DIR)/gb_lite.lib
SM83_LITE = $(BUILD_DIR)/sm83_lite.lib

GENERATED_MUSIC_DIR = generated/music
RGBASM_HUGE ?= $(shell command -v rgbasm-huge 2>/dev/null || echo rgbasm)
RGB2SDAS = python3 tools/rgb2sdas.py
UGE2SOURCE = uge2source

INCLUDES = -I$(SRC_DIR) -I$(SRC_DIR)/core -I$(SRC_DIR)/world -I$(SRC_DIR)/battle -I$(SRC_DIR)/input -I$(SRC_DIR)/audio -I$(SRC_DIR)/ui -I$(SRC_DIR)/debug -I$(SRC_DIR)/screens -I$(SRC_DIR)/game -Ilib/hUGEDriver/include -I$(GENERATED_MUSIC_DIR) -I$(GENERATED_SFX_DIR) -I$(GENERATED_TILES_DIR)

ALL_SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/*/*.c)
BANK5_EARLY_SRCS = $(SRC_DIR)/world/scene_load.c
SRCS = $(BANK5_EARLY_SRCS) $(filter-out $(BANK5_EARLY_SRCS),$(ALL_SRCS))

# Debug-harness-only sources excluded from the release ROM.
# telemetry.c IS needed by gameplay (game.c/world.c emit events);
# telemetry_snap.c is the banked extended-snapshot builder (debug-only).
DEBUG_ONLY_SRCS = $(SRC_DIR)/debug/scenarios.c $(SRC_DIR)/debug/assertions.c $(SRC_DIR)/debug/telemetry_snap.c $(SRC_DIR)/debug/snapshot_banked.c
RELEASE_SRCS = $(filter-out $(DEBUG_ONLY_SRCS),$(SRCS))

MUSIC_SRCS = $(GENERATED_MUSIC_DIR)/battle.c $(GENERATED_MUSIC_DIR)/desolate_landscape.c $(GENERATED_MUSIC_DIR)/forest.c $(GENERATED_MUSIC_DIR)/boss_fight.c
GENERATED_SFX_DIR = generated/sfx
# Explicit list (not wildcard): asset names contain spaces, which make
# would split. Escaped following the assets/music rules' convention.
SFX_UGE = assets/sfx/sfx\ accept.uge assets/sfx/sfx\ back.uge \
          assets/sfx/sfx\ block.uge assets/sfx/sfx\ cursor.uge \
          assets/sfx/sfx\ hit.uge assets/sfx/sfx\ hit2.uge
# Both C files come from one transcriber run (plus sfx_tables.h); the
# recipe is deterministic, so a double invocation is a harmless no-op.
SFX_TABLES = $(GENERATED_SFX_DIR)/sfx_tables.c $(GENERATED_SFX_DIR)/sfx_index.c
MUSIC_OBJS = $(patsubst $(GENERATED_MUSIC_DIR)/%.c,$(BUILD_DIR)/music/%.o,$(MUSIC_SRCS))
MUSIC_OBJS_DEBUG = $(patsubst $(GENERATED_MUSIC_DIR)/%.c,$(BUILD_DIR)/debug/music/%.o,$(MUSIC_SRCS))

HUGEDRIVER_OBJ = $(BUILD_DIR)/lib/hUGEDriver.o
HUGEDRIVER_OBJ_DEBUG = $(BUILD_DIR)/debug/lib/hUGEDriver.o

OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(RELEASE_SRCS)) $(MUSIC_OBJS) $(SFX_OBJS) $(HUGEDRIVER_OBJ)
OBJS_DEBUG = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/debug/%.o,$(SRCS)) $(MUSIC_OBJS_DEBUG) $(SFX_OBJS_DEBUG) $(HUGEDRIVER_OBJ_DEBUG)

# Emulator detection
EMULATOR ?= $(shell command -v pyboy 2>/dev/null || command -v sameboy 2>/dev/null || command -v mgba-sdl 2>/dev/null || command -v mgba-qt 2>/dev/null || command -v mgba 2>/dev/null || echo "")

.PHONY: all release debug run run-debug test test-harness test-scenario state roundtrip screenshot screenshots parity lint memmap verify-oam verify-vram verify-scroll verify-music verify-endurance vram-check vram-text vram-dialogue gfx atlas atlas-check tiles tiles-check doctor music sfx level levels levels-check screens screens-check editor clean

all: $(TARGET)

release: $(TARGET)

debug: $(TARGET_DEBUG)

# Compile-to-assembly warning pass.  -Wall cannot be part of the normal
# build: sdcc's --use-stdout pipeline corrupts the .asm stream when warnings
# are enabled (they leak into stdout).  Compiling with -S surfaces the same
# warnings without invoking the assembler.
lint: gfx tiles $(SRCS)
	@ok=1; \
	for f in $(SRCS); do \
		out=$$($(CC) -S -Wf-Wall $(INCLUDES) -o /dev/null "$$f" 2>&1 || true); \
		if echo "$$out" | grep -q "warning"; then \
			echo "=== $$f ==="; echo "$$out" | grep "warning"; ok=0; \
		fi; \
	done; \
	if [ "$$ok" = "1" ]; then echo "lint: no warnings"; else exit 1; fi

# Regenerate GB tile data headers from PNG assets (docs/graphics.md pipeline).
# Deterministic: rerunning produces byte-identical output.  Requires Pillow,
# which the Nix dev shell provides.
GFX_OUT_DIR = $(SRC_DIR)/gfx

gfx:
	@mkdir -p $(GFX_OUT_DIR)
	# ── Intrepid font ─────────────────────────────────────────────────────
	@python3 tools/png2gb.py assets/intrepid.png --name intrepid_font_tiles \
		--raw -o $(GFX_OUT_DIR)/intrepid_font_tiles.inc
	# ── Forest tileset (assets/forest-tile.png, 16 cols × 3 rows) ────────
	# Full 48-tile world sheet (g_tileset_forest)
	@python3 tools/png2gb.py assets/forest-tile.png --name rpg_forest_world_tiles \
		--palette auto --anchor-color "#7bb660" \
		--raw -o $(GFX_OUT_DIR)/rpg_forest_world_tiles.inc
	# Floor tile: col 0, row 2
	@python3 tools/png2gb.py assets/forest-tile.png --name rpg_forest_floor \
		--palette auto --anchor-color "#7bb660" --tile-coords "0,2" \
		--raw -o $(GFX_OUT_DIR)/rpg_forest_floor.inc
	# Treetop tile: col 12, row 0
	@python3 tools/png2gb.py assets/forest-tile.png --name rpg_forest_tree \
		--palette auto --anchor-color "#7bb660" --tile-coords "12,0" \
		--raw -o $(GFX_OUT_DIR)/rpg_forest_tree.inc
	# Exit tile: col 8, row 2
	@python3 tools/png2gb.py assets/forest-tile.png --name rpg_forest_exit \
		--palette auto --anchor-color "#7bb660" --tile-coords "8,2" \
		--raw -o $(GFX_OUT_DIR)/rpg_forest_exit.inc
	# Stump tiles TL,TR,BL,BR + mini (BR repeated): cols 14-15, rows 0-1
	@python3 tools/png2gb.py assets/forest-tile.png --name rpg_forest_stumps \
		--palette auto --anchor-color "#7bb660" --tile-coords "14,0 15,0 14,1 15,1 15,1" \
		--raw -o $(GFX_OUT_DIR)/rpg_forest_stumps.inc
	# Sprite tiles from forest-tile.png
	@python3 tools/png2gb.py assets/forest-tile.png --name forest_hero_sprite_tile \
		--palette auto --tile-coords "1,2 2,2" \
		-o $(GFX_OUT_DIR)/forest_hero_sprite_tile.h
	@python3 tools/png2gb.py assets/forest-tile.png --name forest_kobold_sprite_tile \
		--palette auto --tile-coords "3,2 4,2" \
		-o $(GFX_OUT_DIR)/forest_kobold_sprite_tile.h
	@python3 tools/png2gb.py assets/forest-tile.png --name forest_bat_sprite_tile \
		--palette auto --tile-coords "9,2 10,2" \
		-o $(GFX_OUT_DIR)/forest_bat_sprite_tile.h
	@python3 tools/png2gb.py assets/forest-tile.png --name forest_chest_sprite_tile \
		--palette auto --tile-coords "11,2" \
		-o $(GFX_OUT_DIR)/forest_chest_sprite_tile.h
	# ── Battle enemy art (assets/battle_sprites.png, 3 cols × 8 rows) ────
	# 36 tiles: 3 art sets × 12 cells (2 frames × 3x2).  Order MUST match
	# ART_ORDER in tools/screen_compiler/battle_compile.py (slime, bat,
	# boss); art set N lives at tile offset N*12 in battle_enemy_art.h.
	# Sheet layout: see tools/compose_battle_sprites.py.
	@python3 tools/png2gb.py assets/battle_sprites.png --name battle_enemy_art \
		--palette auto --tile-coords "0,0 1,0 2,0 0,1 1,1 2,1 0,2 1,2 2,2 0,1 1,1 2,1 0,3 1,3 2,3 0,7 0,7 0,7 0,4 1,4 2,4 0,7 0,7 0,7 0,5 1,5 2,5 0,6 1,6 2,6 0,5 1,5 2,5 0,6 1,6 2,6" \
		-o $(GFX_OUT_DIR)/battle_enemy_art.h
	# ── Desolate landscape (assets/desolate_landscape.png, 16 cols × 3 rows) ──
	# Full 48-tile world sheet (g_tileset_desolate)
	@python3 tools/png2gb.py assets/desolate_landscape.png --name rpg_desolate_world_tiles \
		--palette auto --anchor-color "#938da1" \
		--raw -o $(GFX_OUT_DIR)/rpg_desolate_world_tiles.inc
	# 41-tile subset for scene terrain lookup (rows 0–2, cols 0–8 on row 2)
	@python3 tools/png2gb.py assets/desolate_landscape.png --name rpg_desolate_tiles \
		--palette auto --anchor-color "#938da1" --tile-coords "0,0 1,0 2,0 3,0 4,0 5,0 6,0 7,0 8,0 9,0 10,0 11,0 12,0 13,0 14,0 15,0 0,1 1,1 2,1 3,1 4,1 5,1 6,1 7,1 8,1 9,1 10,1 11,1 12,1 13,1 14,1 15,1 0,2 1,2 2,2 3,2 4,2 5,2 6,2 7,2 8,2" \
		--raw -o $(GFX_OUT_DIR)/rpg_desolate_tiles.inc
	# Sprite tiles from desolate_landscape.png
	@python3 tools/png2gb.py assets/desolate_landscape.png --name hero_desolate_sprite_tile \
		--palette auto --tile-coords "1,2 2,2" \
		-o $(GFX_OUT_DIR)/hero_desolate_sprite_tile.h
	@python3 tools/png2gb.py assets/desolate_landscape.png --name kobold_sprite_tile \
		--palette auto --tile-coords "3,2 4,2" \
		-o $(GFX_OUT_DIR)/kobold_sprite_tile.h
	@python3 tools/png2gb.py assets/desolate_landscape.png --name desolate_bat_sprite_tile \
		--palette auto --tile-coords "9,2 10,2" \
		-o $(GFX_OUT_DIR)/desolate_bat_sprite_tile.h
	# ── Castle tileset (assets/castle-tile.png, 9 cols × 3 rows) ─────────
	# Full 27-tile world sheet (g_tileset_castle)
	@python3 tools/png2gb.py assets/castle-tile.png --name rpg_castle_tiles \
		--palette auto --anchor-color "#d7d7d7" --raw -o $(GFX_OUT_DIR)/rpg_castle_tiles.inc
	# Sprite tiles from castle-tile.png
	@python3 tools/png2gb.py assets/castle-tile.png --name castle_bat_sprite_tile \
		--palette auto --tile-coords "5,2 6,2" \
		-o $(GFX_OUT_DIR)/castle_bat_sprite_tile.h



# Regenerate the asset atlas (docs/assets_atlas.md + src/gfx/asset_atlas.h
# + the banked .inc data).  Deterministic: rerunning produces byte-identical
# output.  See docs/assets_atlas.md and tools/asset_atlas.py.
atlas:
	@python3 tools/asset_atlas.py

# Drift check: Makefile gfx --tile-coords must match the atlas registry, and
# the generated artifacts must be in sync with the current source assets.
# Level compiler targets (docs/level-editor.md)
LEVEL ?= forest
level:
	@python3 tools/level_compiler/validate.py levels/$(LEVEL).json
	@python3 tools/level_compiler/compile.py --all -o src/game/scenes_content.c
	@echo "Compiled level: $(LEVEL)"

levels:
	@python3 tools/level_compiler/validate.py levels/*.json
	@python3 tools/level_compiler/compile.py --all -o src/game/scenes_content.c
	@echo "All levels compiled to src/game/scenes_content.c"

# JSON is the source of truth: committed C must equal fresh compile (no
# hand edits to generated files). decompile.py is a recovery/forensics
# tool, not a gate (see docs/level-editor.md Phase 15).
levels-check:
	@python3 tools/level_compiler/validate.py levels/*.json
	@python3 tools/level_compiler/compile.py --all -o src/game/scenes_content.c --check

src/game/scenes_content.c: $(wildcard levels/*.json)
	@python3 tools/level_compiler/compile.py --all -o src/game/scenes_content.c

# Screen content compiler (docs/level-editor.md Phase 17): screens/*.json is
# the source of truth for title + battle mockup data, just as levels/*.json
# is for scenes.  Committed C must equal fresh compile (no hand edits).
screens:
	@python3 tools/screen_compiler/title_compile.py -o src/game/title_data.c screens/title.json
	@python3 tools/screen_compiler/battle_compile.py --all -o src/game/
	@echo "All screens compiled to src/game/{title_data,battle_screens,battle_types}.c"

screens-check:
	@python3 tools/screen_compiler/title_compile.py --check
	@python3 tools/screen_compiler/battle_compile.py --all --check

src/game/title_data.c: screens/title.json
	@python3 tools/screen_compiler/title_compile.py -o src/game/title_data.c screens/title.json

src/game/battle_screens.c src/game/battle_types.c: $(wildcard screens/battle/*.json) $(wildcard screens/enemy_types/*.json)
	@python3 tools/screen_compiler/battle_compile.py --all -o src/game/

# Extract tile images from source PNGs for the web editor (import_tileset.py)
extract-tiles:
	@python3 tools/level_editor/import_tileset.py \
		--sheet assets/forest-tile.png --csv assets/forest-tileset-description.csv \
		--tileset-id forest --label "Whispering Forest" \
		--gb-tileset-kind WORLD_TILESET_FOREST \
		--output-dir tools/level_editor/public/tiles/forest \
		--output-json tools/level_editor/tilesets/forest.json
	@python3 tools/level_editor/import_tileset.py \
		--sheet assets/desolate_landscape.png --csv assets/desolate_landscape-description.csv \
		--tileset-id desolate_landscape --label "Desolate Landscape" \
		--gb-tileset-kind WORLD_TILESET_DESOLATE \
		--output-dir tools/level_editor/public/tiles/desolate_landscape \
		--output-json tools/level_editor/tilesets/desolate_landscape.json
	@python3 tools/level_editor/import_tileset.py \
		--sheet assets/castle-tile.png --csv assets/castle-tileset-description.csv \
		--tileset-id castle --label "Castle & Bastion" \
		--gb-tileset-kind WORLD_TILESET_CASTLE \
		--output-dir tools/level_editor/public/tiles/castle \
		--output-json tools/level_editor/tilesets/castle.json

# NOTE: extract-tiles is intentionally NOT a dependency here.
# The tileset JSON files (tools/level_editor/tilesets/*.json) are the sole
# source of truth for tile definitions, vram_block layouts, and gb_constant
# values.  Run 'make extract-tiles' once manually when importing a new PNG
# sheet, then commit the resulting JSON.  Re-running it on every editor
# launch would overwrite hand-crafted fields (gb_constant, vram_block).
editor:
	@echo "Starting Game Boy RPG Level Editor..."
	@cd tools/level_editor && npm install --no-audit --no-fund && npm run dev

atlas-check:
	@python3 tools/asset_atlas.py --check

# Tileset manifest check: vram_block composition, exit marking, sheet bounds.
tiles-check:
	@python3 tools/level_editor/validate_tilesets.py tools/level_editor/tilesets/*.json

# Tile-trait generation: manifests -> generated/tiles/tile_traits.h
# (walk/glyph ranges + exit indices consumed by world.c, patrol_banked.c,
# ui.c). Deterministic: rerunning reproduces the header byte-identically.
GENERATED_TILES_DIR = generated/tiles
GENERATED_TILE_WALK = $(GENERATED_TILES_DIR)/tile_walk.h
GENERATED_TILE_GLYPH = $(GENERATED_TILES_DIR)/tile_glyph.h
GENERATED_TILE_PALETTE = $(GENERATED_TILES_DIR)/tile_palette.h
tiles: $(GENERATED_TILE_WALK) $(GENERATED_TILE_GLYPH) $(GENERATED_TILE_PALETTE)

$(GENERATED_TILE_WALK) $(GENERATED_TILE_GLYPH) $(GENERATED_TILE_PALETTE): tools/level_editor/tilesets/desolate_landscape.json tools/level_editor/tilesets/forest.json tools/level_editor/tilesets/castle.json tools/level_compiler/generate_tiles.py | $(GENERATED_TILES_DIR)
	python3 tools/level_compiler/generate_tiles.py --out "$(GENERATED_TILES_DIR)"

$(GENERATED_TILES_DIR):
	mkdir -p $(GENERATED_TILES_DIR)

# Consumers rebuild when the generated headers change (they are untracked).
$(BUILD_DIR)/world/world.o $(BUILD_DIR)/world/patrol_banked.o $(BUILD_DIR)/debug/world/world.o $(BUILD_DIR)/debug/world/patrol_banked.o: $(GENERATED_TILE_WALK)
$(BUILD_DIR)/ui/ui.o $(BUILD_DIR)/debug/ui/ui.o: $(GENERATED_TILE_GLYPH)
$(BUILD_DIR)/game/tiles_content.o $(BUILD_DIR)/debug/game/tiles_content.o: $(GENERATED_TILE_PALETTE)

# Toolchain self-check: every required native binary must not only resolve
# but EXECUTE (a broken file shadowing the real one fails at exec time with
# a cryptic OSError deep inside a build rule). Order-only prerequisite of
# the music/SFX conversion rules, so a broken toolchain fails here first
# with the fix command instead of mid-build.
doctor:
	@python3 tools/doctor.py

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# NOTE: deliberately NO global --max-allocs-per-node override.  SDCC 4.4.1
# (sm83) has a pointer-cache miscompile family across branch joins; the
# per-function volatile guards (battle_update vb, battle_screen vg,
# const-volatile Battle* render params, patrol_banked byte-pointer reads)
# are the fix -- verified by clean-tree A/B (commit history Aug 2026).
# Empirically, BOTH a lowered budget (12000) AND making the g_bk_* banked
# staging globals volatile change codegen in ways that break patrol stepping
# under otherwise-identical sources; default flags with the local guards is
# the only validated configuration.  If you touch optimization flags, run
# patrol_slime_cross / patrol_enemy_bumps_player / battle_multi_enemy_cycle_kill
# plus the full harness before trusting the build.

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) -c $(INCLUDES) -o $@ $<

# Per-file alloc caps (see docs/roadmap.md post-mortem): the bank-3 patrol
# path needs a hard-capped budget in these units to keep its commit-path
# stores intact under SDCC 4.4.1; battle/UI keep their volatile guards with
# default flags (caps there re-expose the transition-render HP corruption).
build/debug/world/world.o: src/world/world.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) -c -DDEBUG_BUILD -Wf--max-allocs-per-node500 $(INCLUDES) -o $@ $<

build/debug/world/patrol_banked.o: src/world/patrol_banked.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) -c -DDEBUG_BUILD -Wf--max-allocs-per-node500 $(INCLUDES) -o $@ $<

build/world/world.o: src/world/world.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) -c -Wf--max-allocs-per-node500 $(INCLUDES) -o $@ $<

build/world/patrol_banked.o: src/world/patrol_banked.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) -c -Wf--max-allocs-per-node500 $(INCLUDES) -o $@ $<

$(BUILD_DIR)/debug/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) -c -DDEBUG_BUILD $(INCLUDES) -o $@ $<

music: $(MUSIC_SRCS) sfx

# Tracker SFX -> synth step tables (Path C transcription). Deterministic:
# rerunning reproduces generated/sfx/sfx_tables.c byte-identically.
sfx: $(SFX_TABLES)

$(SFX_TABLES): $(SFX_UGE) tools/transcribe_sfx.py | $(GENERATED_SFX_DIR) doctor
	python3 tools/transcribe_sfx.py --out "$@" $(SFX_UGE)

$(GENERATED_SFX_DIR):
	mkdir -p $(GENERATED_SFX_DIR)

SFX_OBJS = $(patsubst $(GENERATED_SFX_DIR)/%.c,$(BUILD_DIR)/sfx/%.o,$(SFX_TABLES))
SFX_OBJS_DEBUG = $(patsubst $(GENERATED_SFX_DIR)/%.c,$(BUILD_DIR)/debug/sfx/%.o,$(SFX_TABLES))

$(BUILD_DIR)/sfx/%.o: $(GENERATED_SFX_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) -c $(INCLUDES) -o $@ $<

$(BUILD_DIR)/debug/sfx/%.o: $(GENERATED_SFX_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) -c -DDEBUG_BUILD $(INCLUDES) -o $@ $<

$(GENERATED_MUSIC_DIR)/battle.c: assets/music/Battle\ BGM.uge tools/compile_music.py | $(GENERATED_MUSIC_DIR) doctor
	python3 tools/compile_music.py "$<" 6 song_battle "$@"

$(GENERATED_MUSIC_DIR)/desolate_landscape.c: assets/music/desolate_landscape.uge tools/compile_music.py | $(GENERATED_MUSIC_DIR) doctor
	python3 tools/compile_music.py "$<" 6 song_desolate_landscape "$@"

$(GENERATED_MUSIC_DIR)/forest.c: assets/music/Forest.uge tools/compile_music.py | $(GENERATED_MUSIC_DIR) doctor
	python3 tools/compile_music.py "$<" 6 song_forest "$@"

$(GENERATED_MUSIC_DIR)/boss_fight.c: assets/music/Boss\ fight.uge tools/compile_music.py | $(GENERATED_MUSIC_DIR) doctor
	python3 tools/compile_music.py "$<" 6 song_boss_fight "$@"

$(GENERATED_MUSIC_DIR):
	mkdir -p $(GENERATED_MUSIC_DIR)

$(BUILD_DIR)/music/%.o: $(GENERATED_MUSIC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) -c $(INCLUDES) -o $@ $<

$(BUILD_DIR)/debug/music/%.o: $(GENERATED_MUSIC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) -c -DDEBUG_BUILD $(INCLUDES) -o $@ $<

$(HUGEDRIVER_OBJ): lib/hUGEDriver/src/hUGEDriver.asm tools/rgb2sdas.py | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(RGBASM_HUGE) -I lib/hUGEDriver/ -DGBDK -o $(BUILD_DIR)/lib/hUGEDriver.obj $<
	$(RGB2SDAS) -b 6 -o $@ $(BUILD_DIR)/lib/hUGEDriver.obj

$(HUGEDRIVER_OBJ_DEBUG): lib/hUGEDriver/src/hUGEDriver.asm tools/rgb2sdas.py | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(RGBASM_HUGE) -I lib/hUGEDriver/ -DGBDK -o $(BUILD_DIR)/debug/lib/hUGEDriver.obj $<
	$(RGB2SDAS) -b 6 -o $@ $(BUILD_DIR)/debug/lib/hUGEDriver.obj

$(GB_LITE) $(SM83_LITE): $(OBJS) $(OBJS_DEBUG) | $(BUILD_DIR)
	python3 tools/make_lite_libs.py $(BUILD_DIR)

# The VBlank ISR is copied to WRAM 0xC900 by crt0.s.  sdldgb auto-places
# _DATA at 0xC0A0 (after shadow OAM) and ignores ABS .org reservations, so
# _DATA is pinned at 0xC940 to keep every C symbol above the reserved
# 0xC900-0xC93F ISR region.  Without this, g_boot_phase/g_harness_mode land
# at 0xC89A-C89B and get corrupted by the fixed-layout WRAM (blank screen).
LDFLAGS = -Wl-b_DATA=0xC940

$(TARGET): gfx tiles levels screens music $(OBJS) build/crt0.o $(GB_LITE) $(SM83_LITE) | $(BUILD_DIR)
	$(CC) -no-crt -Wm-yc -Wl-yt0x19 -Wl-yo8 $(LDFLAGS) -Wl-m -Wl-j -o $@ build/crt0.o $(OBJS) $(GB_LITE) $(SM83_LITE)
	@python3 tools/make_sym.py $(BUILD_DIR)/rpg_card_proto.noi $(BUILD_DIR)/rpg_card_proto.sym
	@$(RGBFIX) -v -C -m 0x1b -r 2 -t "GBCARDRPG" $@

$(TARGET_DEBUG): gfx tiles levels screens music $(OBJS_DEBUG) build/crt0.o $(GB_LITE) $(SM83_LITE) | $(BUILD_DIR)
	$(CC) -no-crt -Wm-yc -Wl-yt0x19 -Wl-yo8 $(LDFLAGS) -Wl-m -Wl-j -Wl-y -o $@ build/crt0.o $(OBJS_DEBUG) $(GB_LITE) $(SM83_LITE)
	@python3 tools/make_sym.py $(BUILD_DIR)/rpg_card_proto_debug.noi $(BUILD_DIR)/rpg_card_proto_debug.sym
	@$(RGBFIX) -v -C -m 0x1b -r 2 -t "GBCARDRPG" $@

build/crt0.o: src/crt0.s | $(BUILD_DIR)
	sdasgb -o $@ $<

run: $(TARGET)
	@if [ -z "$(EMULATOR)" ]; then \
		echo "Error: No suitable Game Boy emulator found in PATH." >&2; \
		exit 1; \
	fi; \
	echo "Launching ROM in emulator ($(EMULATOR))..."; \
	$(EMULATOR) $(TARGET)

run-debug: $(TARGET_DEBUG)
	@if [ -z "$(EMULATOR)" ]; then \
		echo "Error: No suitable Game Boy emulator found in PATH." >&2; \
		exit 1; \
	fi; \
	echo "Launching Debug ROM in emulator ($(EMULATOR))..."; \
	$(EMULATOR) $(TARGET_DEBUG)

test: $(TARGET)
	@echo "Validating Game Boy ROM header..."
	@if command -v $(RGBFIX) >/dev/null 2>&1; then \
		$(RGBFIX) -v -C -t "GBCARDRPG" $(TARGET); \
	else \
		test -s $(TARGET); \
	fi
	@echo "ROM validation successful: $(TARGET)"

test-harness: debug
	python3 tools/dev.py test --jobs $(JOBS)

test-scenario: debug
	@python3 tools/dev.py scenario $(SCENARIO)

state: debug
	@python3 tools/dev.py state $(SCENARIO)

roundtrip: debug
	@python3 tools/dev.py roundtrip $(SCENARIO)

screenshot: $(TARGET)
	@bash tools/screenshot.sh $(BUILD_DIR)/screenshot.png $(TARGET)

# Headless gameplay walkthrough screenshots for visual review without booting
# the ROM (see tools/capture_walkthrough.py and AGENTS.md §56).  Boots the
# real release ROM in headless PyBoy, walks deterministically to each
# milestone, and saves raw 160x144 PNGs into screenshots/.  Visual review
# only -- semantic telemetry stays authoritative.  Manual only; not part of
# the CI chain.
# WYSIWYG gate: the ROM must show what the level editor shows for the
# same levels/*.json (whole-map VRAM parity per map + animation frame-set
# parity).  Needs the debug ROM (runs the SameBoy harness like
# test-harness).  Manual only for now; not part of the CI chain.
parity: debug
	@python3 tools/parity_check.py

screenshots: $(TARGET)
	@python3 tools/capture_walkthrough.py

# Verify the player sprite's real-OAM transition-hide across screen changes
# and scene (map) changes via the mGBA debugger (see tools/verify_oam.py).
verify-oam: debug
	@python3 tools/verify_oam.py

# Verify real-boot VRAM writes land (vsync-before-render + LCD-off boot
# redraw): boots the debug ROM WITHOUT harness mode and compares the real
# background ring against the WRAM mirror (see tools/verify_vram.py).
verify-vram: debug
	@python3 tools/verify_vram.py

# Verify camera scrolling invariants (0 VRAM writes during scroll, 2 writes on
# tile commit, no blank floor cells, hero anchored, no LCD-off frames).
verify-scroll: debug
	@python3 tools/verify_scroll.py

# Verify autonomous enemy patrol AI patterns (Slimes cross, Bats circle).
verify-patrol: debug
	@python3 tools/verify_patrol.py

# Verify the music clock never stalls across screen/map transitions: boots the
# debug ROM WITHOUT harness mode (real interrupts) and walks FIELD -> TOWN and
# a guard dialogue round-trip, asserting g_audio_ticks advances every sampled
# frame (timer-driven clock; VBlank stalls 1-2 frames per LCD-off redraw).
verify-music: debug
	@python3 tools/verify_music.py

# Endurance stress test: 60 seconds (3600 frames) of continuous gameplay with
# real timer-driven audio and OAM DMA to assert long-running stability.
verify-endurance: release debug
	@python3 tools/verify_endurance.py

# Font/VRAM pixel ground truth via PyBoy (see tools/vram_check.py).  Boots the
# real release ROM headlessly and reads VRAM directly -- the one place a
# pixel-level check is the correct tool (the char->tile mapping has no
# semantic representation).  Manual only; not part of the CI chain.
vram-check: release
	@python3 tools/vram_check.py

# Text-layer ground truth via PyBoy (see tools/vram_text_check.py): asserts
# generic text lands on the always-displayed BACKGROUND (0x9800), not the
# overworld-only WINDOW (0x9C00), by opening the ITEM menu with START and
# reading real VRAM.  Manual only; not part of the CI chain.
vram-text: release
	@python3 tools/vram_text_check.py

# Dialogue-box placement ground truth via PyBoy (see
# tools/vram_dialogue_check.py): asserts the dialogue box renders in the
# BACKGROUND tilemap (0x9800) at screen rows 12-17, written into the
# scrolled ring at (12 + scroll_y)..(17 + scroll_y) so it cannot shift with
# the camera or pollute the map ("text from other scenes").  Manual only;
# not part of the CI chain.
vram-dialogue: release
	@python3 tools/vram_dialogue_check.py

# Print a reproducible memory budget (code/WRAM usage, _HOME headroom vs the
# 0x8000 ceiling).  Exits non-zero if a documented invariant is violated.
memmap: debug
	@python3 tools/memmap.py $(BUILD_DIR)/rpg_card_proto_debug.map

clean:
	rm -rf $(BUILD_DIR) $(GENERATED_MUSIC_DIR)

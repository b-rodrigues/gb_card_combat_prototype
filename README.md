# Kaartenheld — Game Boy Demo

**Kaartenheld** (Luxembourgish for "Card Hero") is an upcoming top-down adventure game for the Nintendo Game Boy by **FroBlo Studios**, a two-person indie team of inexperienced-but-relentless developers.

This repository contains the playable **demo** for Kaartenheld: a top-down adventure with a deck-building card combat system in which you build **poker hands to trigger combos**, explore the world, and grow your deck — with **no traditional RPG mechanics**: no equipment, no levels, no XP. Your power is entirely determined by the cards you carry.

Every combat victory rewards you with a **new random card**: add it to your deck, or sell it.

## How It Is Made

Kaartenheld is built with a deliberately split creative process:

- **Art and music are entirely human-made** — every sprite, tileset, and track is crafted by hand by FroBlo Studios.
- **The code is written by AI coding agents** — an LLM-first development workflow in which agents build, test, and debug the game through a deterministic, machine-readable harness (see the technical documentation below).
- **Levels are authored in a bespoke level editor** built specifically for this game, which compiles directly into the game's build pipeline.

## The Game

- **Top-down adventure**: explore tile-based maps, talk to characters, follow the story.
- **Card combat**: battles are fought with a 20-card deck; you play cards from a 5-card hand.
- **Poker combos**: the values of the cards in your hand form poker hands — PAIR, TWO PAIR, THREE KIND, STRAIGHT, FLUSH, FULL HOUSE, FOUR KIND, STRAIGHT FLUSH, FIVE KIND — and stronger hands mean stronger effects. Ring cards act as jokers, substituting any value.
- **No RPG grind**: there is no equipment and no leveling. Deck composition is your entire progression.
- **Loot cards**: every victory drops a procedurally generated card (names, values, riders, rarity tiers). Keep it, build it into your deck, or sell it to a merchant for ECUs, the in-game currency.
- **Status effects**: POISON, BURN, and FREEZE ride on card hits with stacking rules and per-round ticks.
- **Persistence**: battery-backed SRAM save/load with slots.

## About FroBlo Studios

FroBlo Studios is two inexperienced indie developers making a Game Boy game the hard way: authentic hardware constraints, a fully reproducible Nix toolchain, and an AI-agent-first development workflow (more on that below).

---

## Development & Technical Documentation

The following sections cover how this repository is engineered and tested.

### LLM-First Development Architecture

Developing Game Boy games with AI agents requires solving retro hardware opacity: LLMs cannot reliably play real-time games with a joypad or interpret low-resolution pixels.

This repository solves that by making **semantic observability a first-class subsystem**:

```text
Host-Side AI Agent / Test Runner (Python)
                   │
                   ▼  PTY / Debug Protocol
      mGBA / SameBoy Emulator Session
                   │
                   ▼  Memory & Registers
      Game Boy Debug ROM (rpg_card_proto_debug.gb)
    ┌──────────────────────────────────────────────┐
    │ • Declarative State Injection (Scene/Player) │
    │ • Bounded Telemetry Ring Buffer (Events)     │
    │ • Semantic State Snapshots (Game/World/Party)│
    │ • Hardware Timer Sound Clock (TIMA ISR)      │
    │ • Deterministic RNG Seeding                  │
    └──────────────────────────────────────────────┘
```

#### Key LLM-First Capabilities

- **Declarative Scenario Fixtures**: 159 deterministic scenarios in `tools/scenarios/tests/*.json` that configure map coordinates, story flags, party stats, decks, hands, inventory, and enemy states before running scripted inputs.
- **Parallel Test Harness**: Runs test scenarios concurrently across host CPU cores (`make test-harness JOBS=16`), achieving over 7x speedup compared to serial execution.
- **Structured Telemetry**: Every state transition, map crossing, collision, dialogue event, quest update, card action, combo resolution, loot drop, and battle action emits structured telemetry into a bounded ring buffer.
- **Semantic State Snapshots**: Agents inspect exact world coordinates, active screens, dialogue trees, story flags, deck/collection contents, and combat stats rather than parsing pixel frames.
- **Deterministic Randomness**: RNG is fully seeded and reproducible across test runs (card draws, deck shuffles, loot rolls, status chances).
- **OAM Fidelity Checks**: `make verify-oam` drives mGBA's debugger to assert sprite positions across battle/scene/dialogue transitions — catching VBlank-timed rendering bugs the SameBoy harness cannot observe.

See [`docs/DEBUG_PROTOCOL.md`](docs/DEBUG_PROTOCOL.md) and [`AGENTS.md`](AGENTS.md) for full protocol and operational contracts.

### Engine Features

The codebase includes a complete, playable vertical slice proving all core systems:

- **World & Overworld**:
  - Tile-based maps with smooth sub-tile hardware scrolling;
  - Collision detection, walkability allowlists, and warp gates;
  - Persistent actors, NPC patrolling, and proximity interaction.
- **Card Combat** (see [`docs/card-battle.md`](docs/card-battle.md)):
  - Attack/defend phases with a 5-card hand drawn from a 20-card deck;
  - Number-based **poker-style combos**: the hand you make determines the tier — PAIR, TWO PAIR, THREE KIND, STRAIGHT, FLUSH, FULL HOUSE, FOUR KIND, STRAIGHT FLUSH, FIVE KIND;
  - **Ring cards act as jokers**, substituting any value;
  - Card types (sword, shield, bow, heal, dagger) with deterministic effect resolution and a visible turn timer;
  - Live combo preview on the battle HUD (mirrors resolution, including ring trials).
- **Deck & Collection** (see [`docs/deck.md`](docs/deck.md)):
  - 20-card combat deck with draw/discard/reshuffle;
  - Collection management through the quick screen (add/remove deck copies, filter/sort);
  - Deck state is persistent and saveable.
- **Procedural Card Loot** (Borderlands-inspired, see [`docs/loot.md`](docs/loot.md)):
  - Enemies drop **cards**, not generic items — every drop is either deck material or sellable loot;
  - Procedural card synthesis (names, values, riders) with rarity tiers;
  - Cards can be sold back to the merchant for **ECUs**, the game's currency.
- **Status Effects** (see [`docs/combo-system.md`](docs/combo-system.md)):
  - POISON, BURN, and FREEZE as on-hit riders with deterministic chance rolls, stacking rules, and per-round ticks.
- **Story & Events**:
  - Dialogue player with multi-step choices and branch conditions;
  - Scripted quest system with multi-stage state tracking;
  - Global story flags and variables.
- **Persistence**:
  - Battery-backed SRAM save/load with versioned formats and slot management.
- **Audio Architecture**:
  - Hardware Timer-driven music clock (`TIMA` overflow ISR) running at fixed 256 Hz tempo regardless of main loop load or LCD redraws.

### Engine vs. Game Content Separation

The codebase strictly enforces the separation of generic engine primitives from game-specific data:

```text
src/
├── core/       Generic boot glue, event engine, dialogue runner, story flags, quests
├── rpg/        Generic GameState, cards, deck, currency, party, progression, status, save
├── world/      Generic scene loader, actors, movement, collision, interactions
├── battle/     Generic battle lifecycle, hand/combo evaluation, combatant stats
├── ui/         Generic screen renderer, menu frame layouts, font rendering
├── screens/    Overworld, battle, dialogue, quick screen, shop, save/load screens
├── input/      Joypad abstraction and programmatic debug input injection
├── audio/      Timer-driven audio ISR and sound registers
├── debug/      Harness telemetry, scenario loader, assertions, RNG control
│
└── game/       GAME CONTENT (All game-specific tables live here)
    ├── game_ids.h          Named story flags, variables, currencies, and actors
    ├── content.c           New game initialization and victory hooks
    ├── events_content.c    Scripted events and quest triggers
    ├── dialogue_content.c  Dialogue scripts and NPC lines
    ├── actors_content.c    Per-map actor spawn tables and behaviors
    ├── cards_content.c     Card catalog and effect definitions
    ├── quests_content.c    Quest registry rows and objectives
    ├── scenes_content.c    Scene definitions and map layouts
    ├── shops_content.c     Shop inventories and pricing
    ├── loot_drop_banked.c  Per-enemy drop tables and loot synthesis
    └── tiles_content.c     World tilesets and graphical definitions
```

When creating a new game from this template, developers and agents modify `src/game/` while leaving the engine subsystems in `src/core/`, `src/world/`, `src/rpg/`, and `src/ui/` untouched.

### Hardware & Toolchain

- **Target Hardware**: Nintendo Game Boy (DMG) / Game Boy Color (CGB)
- **C Toolchain**: GBDK-4 (`lcc` / SDCC)
- **Assembly Toolchain**: RGBDS (`rgbasm`, `rgblink`, `rgbfix`)
- **Environment**: Nix flakes (100% reproducible, zero host dependencies)
- **Development Emulator**: SameBoy & mGBA

### Quick Start

#### 1. Enter the Nix Development Shell

```bash
nix develop
```

All build tools, compilers, emulators, and test runners are automatically provided.

#### 2. Primary Make Targets

| Target | Description | Output |
| :--- | :--- | :--- |
| `make release` | Build optimized release ROM | `build/rpg_card_proto.gb` |
| `make debug` | Build debug ROM with harness & telemetry | `build/rpg_card_proto_debug.gb` |
| `make test-harness` | Run all 159 scenarios in parallel | Parallel test results (PASS/FAIL) |
| `make test-scenario SCENARIO=<name>` | Run one scenario with full diagnostics | PASS/FAIL + state + telemetry |
| `make test` | Validate ROM header and checksums | ROM verification |
| `make verify-oam` | mGBA debugger OAM fidelity checks across transitions | OAM verification |
| `make memmap` | Check ROM and WRAM memory budget | Invariant check (`_HOME < 0x8000`) |
| `make lint` | Compile-to-assembly `-Wall` pass over all sources | Warning report |
| `make screenshots` | Headless PyBoy walkthrough capture | `screenshots/*.png` |
| `make run` | Launch release ROM in emulator | Game window |
| `make run-debug` | Launch debug ROM in emulator | Debug game window |
| `make screenshot` | Capture headless emulator screenshot | `build/screenshot.png` |
| `make clean` | Remove all generated build artifacts | Clean directory |

#### 3. Running Scenario Tests

Run all scenarios in parallel (defaults to all host cores, or specify `JOBS`):

```bash
make test-harness JOBS=16
```

Run a single scenario with full diagnostic trace:

```bash
make test-scenario SCENARIO=town_arrival
```

### Documentation

#### Protocol & Agent Contracts

- [`AGENTS.md`](AGENTS.md) — Operational contract and rules for AI coding agents.
- [`docs/DEBUG_PROTOCOL.md`](docs/DEBUG_PROTOCOL.md) — Authoritative debug protocol and LLM state inspection contract.
- [`docs/dev-harness.md`](docs/dev-harness.md) — Deterministic scenario design and emulator bridge.
- [`docs/LLM_AGENT_GUIDE.md`](docs/LLM_AGENT_GUIDE.md) — Practical guide for agents working in this repo.

#### Architecture & Engineering

- [`docs/architecture.md`](docs/architecture.md) — Subsystem architecture and dependency direction.
- [`docs/game-vs-engine.md`](docs/game-vs-engine.md) — Engine/game layer boundary rules.
- [`docs/FOUNDATION_CONTRACT.md`](docs/FOUNDATION_CONTRACT.md) — Foundation boundaries and golden rules.
- [`docs/rpg-foundation.md`](docs/rpg-foundation.md) — Foundation design notes.
- [`docs/save-format.md`](docs/save-format.md) — SRAM save format and versioning specification.
- [`docs/memory-budget.md`](docs/memory-budget.md) — ROM and WRAM memory budgets.
- [`docs/graphics.md`](docs/graphics.md) — Graphics conversion pipeline (`png2gb.py`) and tile layout.
- [`docs/testing.md`](docs/testing.md) — Testing strategy and harness usage.
- [`docs/roadmap.md`](docs/roadmap.md) — Known gaps and future work.

#### Gameplay Design

- [`docs/card-battle.md`](docs/card-battle.md) — Card combat design: attack/defend phases, card roles, timer.
- [`docs/combo-system.md`](docs/combo-system.md) — Combo tiers, ring jokers, effect and status architecture.
- [`docs/deck.md`](docs/deck.md) — Deck system: 20-card deck, draw/discard, collection.
- [`docs/deck-management.md`](docs/deck-management.md) — Quick-screen deck management UX.
- [`docs/loot.md`](docs/loot.md) — Procedural card loot, rarity, and ECU economy.
- [`docs/implementing-card-phases.md`](docs/implementing-card-phases.md) — Implementation status of the card phases.
- [`docs/dialogue-boxes.md`](docs/dialogue-boxes.md) — Dialogue box rendering and camera alignment.

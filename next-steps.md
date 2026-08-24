# Next Steps

## Current State

Branch: `loot-instances`
All gates green: 134/134 scenarios, lint clean, memmap +441 B fixed headroom,
verify-oam OK.

The poker combat system is fully operational: material × effect × weapon
identity tables, derived CardId synthesis, live COMBO row preview,
energy 6/phase with rejection feedback, and enemy trios with doubled HP.

## Priority 1 — Fix the victory drop hook hang

The victory loot drop hook is fully coded but **deferred**: invoking it
from `world_on_battle_end` causes the guest to spin (mGBA ~300% CPU,
debugger pipe dead, game_render breakpoint never re-hit).

### What we know
- The hook calls two banked dispatches sequentially:
  `game_loot_pool_for_battle()` then `loot_roll_drop()`
- Disabling the entire hook makes all scenarios pass
- Disabling just the collection add still hangs → the issue is in the
  pool trampoline or the rng consumption shifting subsequent outcomes
- The nix-pinned GBDK was rebuilt from source during this session
  (`building '...gbdk-4.3.0.drv'`), which may have changed codegen
- Identical scripts flip between passing and failing across sessions

### Debugging approach
1. Run with `videoSync=true audioSync=true` (throttled) to see if the
   spin is an unthrottled-run-only artifact
2. Bisect: test pool trampoline alone vs roll trampoline alone
3. If nested-trampoline depth is the issue, restructure as a single
   banked entry that does pool-copy + roll internally
4. Check whether the freshly rebuilt GBDK produces different codegen
   than the previously cached build

## Priority 2 — Card naming + display (Phase 3 lite)

Generated cards currently show only their archetype's catalog name.
Adding a material prefix ("WD SW", "BRN SW", "MYT SW") in the CARDS tab
and detail view makes drops feel distinct.

Small scope, high player-visible impact.

## Priority 3 — Sell/trade UI at the Merchant

A "SELL" mode in shop_screen that lists owned loot instances and converts
them to gold via the centralized sell-value formula (§16).  Medium scope.

## Priority 4 — Fire/ice status effects (Phase 4)

Burn and freeze/slow as new StatusId values with battle resolution rules.
Only worth doing after drops are flowing.

## Completed This Session

- Poker hand tiers (PAIR..FIVE KIND) with strict sizing + suited bonus
- Derived-id card_get_def synthesis (loot-range CardIds)
- DA1 Poison Dagger starter card + BOW 10 Merchant stock
- Energy pool 6/phase + NO ENERGY!/OUT OF USES! rejection feedback
- Live COMBO row preview (real-time pending-hand classification)
- Enemy trios + doubled enemy HP + solo final boss
- Save system fixes (slot stride 0x400, runtime capacity guard)
- SRAM description blob fix, NULL-speaker guard, reshuffle rider fix
- dialogue-boxes.md constraints doc

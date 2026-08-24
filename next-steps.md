# Next Steps

## Current State

Branch: `loot-instances`
All gates green: 139/139 scenarios, lint clean, memmap +72 B fixed headroom,
verify-oam OK.

The poker combat system is fully operational: material × effect × weapon
identity tables, derived CardId synthesis, live COMBO row preview,
energy 6/phase with rejection feedback, and enemy trios with doubled HP.

## Priority 1 — ~~Fix the victory drop hook hang~~ RESOLVED

**Root cause (found 2026-08-24): fixed-bank overflow, not the trampolines.**
Wiring the drop hook grew `_CODE`/`_HOME` ~30 B past `0x8000`; rgblink
silently overwrote the start of bank 2 (`Warning: Possible overflow ... bank
1 -> 2`) and any execution reaching the corrupted region spun the guest.
Whether it hung flipped with unrelated code-size changes — hence the
"identical scripts flip between passing and failing" behavior.

Fix: restructured the drop path so it costs almost no fixed code — the whole
decision (50% gate on the isolated loot RNG, weapon pick from the enemy
profile, material+effect roll, derived-id encode) is ONE bank-2 body
(`src/game/loot_drop_banked.c`) behind a thin wrapper (`game_loot_drop()`,
`src/game/content.c`); `world_on_battle_end` only records the result
(telemetry + collection add).  Regression scenarios: `loot_drop_victory`
(deterministic wood plain dagger from seed 5002 + telemetry payload asserts)
and `loot_drop_slime`.  See AGENTS.md §52.18 for the general rule.

## Priority 2 — ~~Card naming + display (Phase 3 lite)~~ DONE

Loot cards now synthesize their identity name ("WD SW", "MYT PSN DA") in
`loot_synth_banked` (`CardDefinition.name` widened to 12); the CARDS tab
shows the name instead of the generic type/power code for loot-range ids,
and the detail page renders the full 11-char name.  Covered end-to-end by
`loot_drop_victory` (telemetry payload + `screen_row contains "WD DA"`).

## Priority 3 — ~~Sell/trade UI at the Merchant~~ DONE

Selling lives in the CARDS-tab detail page (the §24 canonical flow:
collection -> select card -> sell), not a separate shop mode: engaging a
buying shop (`ShopDefinition.buys`) sets `g->shop_id` (cleared on scene
change), and loot-range cards then show `[A]SELL` on their detail page.
A sells one copy for its synthesized `price` (= the centralized §16
formula, computed in `loot_synth_banked`) and emits CARD_SOLD.  Fully
decked cards are unsellable (ownership must stay backed).  Regression:
`merchant_sell_loot`.  Note: this cost ~370 fixed-bank bytes; paid for
by removing all `%`/`/` from harness-exercised paths so the SDCC divmod
library no longer links (`ui_ones_digit`, battle wrap-arounds).

## Priority 4 — ~~Fire/ice status effects (Phase 4)~~ DONE

STATUS_BURN (2 dmg/round, max 3 stacks, duration 2) and STATUS_FREEZE
(no tick, duration 1 -- afflicted combatant skips its next attack) added
to the generic status system; freeze is exposed to battle as a flat WRAM
bitmask (`g_status_frozen_mask`) maintained by apply + the banked tick
body, so the fixed-bank cost is one load/and in the telegraph path plus
the TURN_SKIPPED event emit.  Loot wiring: fire swords roll BURN riders,
ice swords FREEZE riders, daggers POISON -- all plain-heavy (25% rider
chance), legality per §34.2; synthesized names carry the effect infix
("WD PSN DA", "IRN FR SW").  Scenarios: `status_burn_apply`,
`status_freeze_skip` (both payload-locked, negative-tested).

## Completed This Session

- Victory drop hook un-hung and enabled (root cause: fixed-bank overflow
  corrupting bank 2 — see AGENTS.md §52.18; drop decision now one bank-2
  body behind `game_loot_drop()`)
- Isolated loot RNG (`g_loot_rng_state`) seeded from the main seed via
  `rng_set_seed`; drops never shift shared-RNG outcomes
- `loot_drop_victory` regression scenario (drop telemetry payloads +
  CARDS-tab name render) + restored `loot_drop_slime`
- Loot card identity names ("WD SW", "MYT PSN DA") in CARDS tab + detail
- Merchant sell flow (CARDS-tab detail `[A]SELL`, CARD_SOLD telemetry,
  `merchant_sell_loot` scenario)
- Fire/ice statuses (Phase D): BURN + FREEZE with battle resolution
  (frozen bitmask + TURN_SKIPPED), loot rider wiring, scenarios
  `status_burn_apply` / `status_freeze_skip`
- Fixed-bank diet: SDCC divmod library unlinked (`ui_ones_digit`,
  battle wrap-arounds, add/sub rider scaling)
- All gates green: 139/139 harness, release ROM, lint, memmap (+72 B
  fixed headroom), verify-oam

## Completed Previous Session

- Poker hand tiers (PAIR..FIVE KIND) with strict sizing + suited bonus
- Derived-id card_get_def synthesis (loot-range CardIds)
- DA1 Poison Dagger starter card + BOW 10 Merchant stock
- Energy pool 6/phase + NO ENERGY!/OUT OF USES! rejection feedback
- Live COMBO row preview (real-time pending-hand classification)
- Enemy trios + doubled enemy HP + solo final boss
- Save system fixes (slot stride 0x400, runtime capacity guard)
- SRAM description blob fix, NULL-speaker guard, reshuffle rider fix
- dialogue-boxes.md constraints doc

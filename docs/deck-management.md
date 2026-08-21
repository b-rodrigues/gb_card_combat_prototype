# Deck Management — CARDS Menu & Starter Deck

Authoritative spec for the deck-management feature: the starter deck grant,
the unified CARDS menu, and the input contract around it. Semantics of the
underlying collection/deck engine live in `docs/deck.md`; this document owns
the *player-facing* management layer.

---

## 1. Starter deck

A new game grants a real, owned 5-card deck (collection **and** deck state in
`GameState.cards`, written by `game_new_game()` via the silent mutators
`deck_collection_add` / `deck_add_card` — no boot telemetry):

```text
IRON_SWORD   ×2   → SW3   (max_copies 2)
WOODEN_SHIELD ×2  → SH2   (max_copies 2)
FIRE_TOME    ×1   → FI4   (3 uses per battle)
```

Consequences:

- Battles always draw from the player's real deck. The packed fallback deck
  (`src/battle/deck_init.c`) remains only as a safety net for empty/legacy
  state and mirrors this same 5-card set.
- Healing is **not** free at the start. The only heal source is the shop's
  HEALING_HERB card — **HE5** (power 5, `uses_per_battle` 3, price 20g).
  Buy it, then add it to the deck from the CARDS menu.
- Shop rows render the battle code derived from the card definition
  (`ui_card_code_str`: type code + power digit, e.g. `HE5`, `SW3`) plus its
  `price` — no display strings are duplicated in the shop layer.
- The old 20-card starter table is gone; the deck grows toward the 20-card
  cap through play.

## 2. The CARDS menu

START opens the quick screen. It has two tabs: **CARDS** and **QUEST**
(LEFT/RIGHT switches). The former separate DECK tab is merged into CARDS:
every owned, deckable card is one list entry showing its deck membership.

### Layout

Each card occupies a **pair of rows**:

```text
  * FILTER/SORT *            ← top row (see §4)
> SW3                     T      ← name row: cursor + short code + membership
  Sword: physical               ← description row (combat description)
  SH2                     F
  Shield: block dmg
  ...
```

- **Short code**: `<battle_type><power>` — SW/SH/BO/FI/HE + value digit,
  identical to the combat hand display.
- **Description**: `card_get_description(battle_type)` — the same string the
  battle HUD shows.
- **Membership column** (rightmost): the decked-copy count as a digit
  (`0` = none, `1`..`n` = copies in the deck), so partial stacks (starter
  2x SW3/SH2, herb up to 3) are visible at a glance.
- SPECIAL cards (e.g. AMULET) are not deckable and are hidden from the list.
- ~6 pairs visible; scrolling moves by pair.

### Actions

| Input | Context | Effect |
|---|---|---|
| START | overworld / menu | open / close the menu |
| LEFT / RIGHT | menu | switch tab |
| UP / DOWN | card rows | move cursor by entry (UP from first card reaches the top row) |
| A | card row | add one copy; clear all copies when fully decked (§3) |
| SELECT | CARDS row | detail submenu (§5) |
| SELECT | QUEST row | placeholder quest detail text |
| B | main list, not on top row | jump to first card row (**step 1**) |
| B | on top FILTER/SORT row | close the menu (**step 2**) |
| B | any submenu | back out one level |

The two-step B is deliberate: B never closes the menu directly from the card
list, so an accidental press cannot lose your place.

## 3. Deck membership: count-up-then-clear (A)

A routes through the real mechanics — no UI-side shortcuts:

- A adds **one** copy via `deck_add_card()`. Emits `CARD_ADDED_TO_DECK`.
- Once the card is fully decked (the add is rejected for per-card reasons —
  no owned copies left beyond `max_copies`), A **clears every decked copy**
  of it via repeated `deck_remove_card()`. Emits `CARD_REMOVED_FROM_DECK`
  per copy.
- Rejection (deck full) shows a transient message line instead of a silent
  no-op:
  - `DECK FULL` — hard limit of **20** cards (`MAX_DECK_CARDS`).

(The earlier per-press boolean toggle could never rebuild a multi-copy stack
— pressing A on a 2-copy row silently dropped it to 1 with no path back —
so the semantics were redesigned around the visible count digit.)

## 4. Filter/sort top row

The first list entry is `* FILTER/SORT *`. Reach it with UP from the first
card (or B's step-1 jump followed by UP). A opens a small picker (hint line
`LR CYCLE  A:OK B:NO`) where LEFT/RIGHT cycle the existing filter
(ALL/ATK/DEF/HEL/STS/UTL) and sort (OFF/TYPE/PWR+/PWR-/CST+/CST-)
values; A validates and applies them to the list; B backs out unchanged.

## 5. Detail submenu (SELECT)

SELECT on a highlighted entry opens a full-property view; B returns.

CARDS:

```text
NAME   HERB
TYPE   HEAL
PWR    5        COST   1
USES   3/battle (- = unlimited)
MAX CP 3        PRICE  20
OWNED  x1       IN DECK x0
```

QUEST: a one-line placeholder (`no details yet`); per-quest detail is future
work.

## 6. Reshuffle semantics (combat)

Already implemented by the deck engine (`deck_draw`, `src/battle/deck.c`);
restated here because it constrains the menu's promises:

- Playing a card discards it and refills that hand slot immediately.
- When the draw pile is exhausted, the discard pile is shuffled back in —
  the player cycles through all their cards again ("fresh deck").
- `uses_remaining` travels inside the `Card` copy through discard/draw/
  reshuffle, so **limited cards stay depleted across reshuffles** while
  unlimited cards cycle freely. A depleted HE5 returns to the hand but is
  unplayable (skipped by cursor movement and auto-select).

## 7. Telemetry contract

No new events. The menu reuses:

- `CARD_ADDED_TO_DECK` / `CARD_REMOVED_FROM_DECK` (add / clear)
- `CARD_PURCHASED` / `CARD_PURCHASE_FAILED` (shop)
- `RENDER_SCREEN` (menu redraws)

State assertions use the extended snapshot's collection/deck sections; byte
offsets are a transport contract (AGENTS.md §53.7), scenarios must use the
semantic assertion names.

## 8. Scenarios

| Scenario | Proves |
|---|---|
| `cards_menu_toggle` | grant exists at new game; A on a maxed card clears it, adds rebuild the count, events emitted |
| `fallback_deck_starter` | emptying the persistent deck routes battles onto the packed starter-deck fallback (hand renders SW3 SW3 SH2 SH2 FI4) |
| `reshuffle_preserves_limits` | FI4 depletes after 3 plays and stays dead across reshuffle; SW keeps cycling |
| `herb_purchase_flow` | buy herb → add to deck → battle heal 5 |

Existing scenarios bound to the old tab/filter/sort keybindings are updated
or retired alongside this change.

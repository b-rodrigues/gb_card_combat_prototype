I’d phase it so that **every phase produces something playable**, rather than building the whole combat engine and only testing it at the end.

### Phase 0 — Lock down the prototype rules

Before coding, define the smallest possible ruleset:

* 1 player
* 1 enemy
* 5-card hand
* `SW`, `SH`, `BO`, `FI`, `HE`
* numbers 1–9
* ascending number straights
* A = select
* B = undo
* SELECT = execute
* START = inventory
* bottom description line
* bottom-most timer bar
* no card artwork

**Deliverable:** a tiny written combat spec that the implementation agent can treat as the source of truth.

---

### Phase 1 — Build the battle screen

Ignore actual combat.

Get this rendered on the Game Boy:

```text
SLIME              HP 30/30

        [SLIME]

HERO               HP 40/40

COMBO:

SW2  SH5  BO3  SW4  HE7
 ^
Sword attack. Deals physical damage.
████████████████████████████████
```

Implement:

* player/enemy placeholder sprites
* HP display
* card hand
* cursor
* combo area
* description line
* timer bar

**Goal:** prove the UI fits comfortably on the Game Boy screen.

---

### Phase 2 — Nail the controls

Make the UI completely interactive, but don't resolve attacks yet.

Implement:

* D-pad left/right → move cursor
* A → select card
* B → remove last card
* SELECT → attempt to execute
* START → existing inventory
* timer pauses in inventory

Make sure the cursor and selected cards feel responsive.

**Goal:** the controls should already feel good before combat exists.

---

### Phase 3 — Build the card/hand system

Introduce the actual card data structures.

Implement:

* card IDs
* card types
* card numbers
* five-card hand
* combo array
* card selection
* card removal
* drawing replacement cards

Initially use a **fixed, deterministic deck**.

For example:

```text
SW2 SW3 SW4 SH3 SH5
BO2 BO4 FI3 HE5 ...
```

Don't randomize yet.

**Goal:** the agent can reliably manipulate cards without worrying about combat.

---

### Phase 4 — Build the combo evaluator

This should be its own isolated system.

Given:

```text
SW2 BO3 FI4
```

it should return something like:

```text
cards:       3
numbers:     2,3,4
straight:    true
multiplier:  150
```

Test it with lots of combinations:

```text
2 3 4       → straight
2 4 5       → no straight
5 4 3       → no straight initially
2 3         → normal
2 3 4 5     → 4-card straight
```

**Goal:** combat math can be tested independently of the UI.

---

### Phase 5 — First complete attack

Now connect everything.

The player should be able to:

```text
select cards
      ↓
SELECT
      ↓
evaluate combo
      ↓
calculate damage
      ↓
display result
      ↓
subtract enemy HP
```

Example:

```text
COMBO: SW2 BO3 SW4

2 → 3 → 4
STRAIGHT!
DAMAGE: 13
```

Don't worry about enemy AI yet.

**Goal:** you can defeat the enemy through card selection.

---

### Phase 6 — Add the turn timer properly

Now make the timer part of the actual game.

Start it when the player receives control:

```text
████████████████████████████████
```

Have it shrink:

```text
████████████████████████
```

then:

```text
████████████
```

then:

```text
████
```

then empty.

When it expires:

* execute the current combo if one exists
* otherwise end the turn

Pause it during:

* animations
* result messages
* inventory
* enemy turns

**Goal:** the player feels pressure, but never feels cheated.

---

### Phase 7 — Enemy turn

Give the enemy one extremely simple attack.

For example:

```text
SLIME ATTACK!
DAMAGE: 10
```

Then transition into the player's defense phase.

Don't build sophisticated AI yet.

**Goal:** establish the complete loop:

**Player → Enemy → Player → Enemy**

---

### Phase 8 — Defense

Introduce `SH` cards.

Enemy attacks:

```text
DAMAGE: 15
```

Player gets their hand:

```text
SW2 SH5 BO3 SH4 HE7
```

Player selects:

```text
SH4 SH5
```

SELECT:

```text
BLOCK: 9
DAMAGE TAKEN: 6
```

Then return to the player's next turn.

**Goal:** the player now has meaningful decisions on both offense and defense.

---

### Phase 9 — Healing and card effects

Add `HE`.

For the first version:

```text
HE5
```

simply means:

```text
+5 HP
```

Then add `FI` as a simple elemental attack.

Don't introduce elemental weaknesses yet.

**Goal:** prove that different card types can share the same underlying card framework.

---

### Phase 10 — Proper deck cycling

Once the basic battle is fun:

* shuffle deck
* draw hand
* discard played cards
* draw replacements
* handle empty deck
* prevent impossible hand states

This is where the system starts feeling like an actual card game rather than a combat UI with predefined cards.

---

### Phase 11 — Improve the feel

Only now spend time on juice:

* card-selection sounds
* combo sound
* straight sound
* attack sound
* shield/block sound
* low-timer beep
* damage numbers
* flashing enemy sprite
* tiny attack animations
* screen shake, if practical
* blinking timer at low time

Because the underlying system already works, these improvements should be relatively safe.

---

### Phase 12 — Expand the combat rules

Once the basic version is fun, start adding the things that make it more authentically Baten Kaitos-inspired:

* descending straights
* longer combos
* matching-number combinations
* elemental interactions
* stronger card effects
* defensive combos
* special cards
* character-specific decks
* status effects
* critical/special combinations

**Important:** add these one at a time. Don't dump all of the original game's complexity into the prototype at once.

---

### Phase 13 — Multiple enemies and targeting

Add:

* multiple enemies
* target cursor
* enemy-specific weaknesses
* attacks that hit one or multiple targets

This is where D-pad Up/Down/Left/Right can acquire more meaning.

---

### Phase 14 — Replace the text placeholders

Only once the mechanics are solid should you move from:

```text
SW3  SH5  BO2
```

to something like:

```text
⚔3  🛡5  🏹2
```

And eventually to tiny pixel-art icons.

The important thing is that the underlying card representation **doesn't change**.

---

## The really important milestone

I'd consider the first **vertical slice** to be Phase 8:

```text
       PLAYER TURN
            ↓
      Select cards
            ↓
        Build combo
            ↓
         SELECT
            ↓
      Attack enemy
            ↓
       Enemy attacks
            ↓
      Select defense
            ↓
         SELECT
            ↓
       Block damage
            ↓
       PLAYER TURN
```

With:

```text
A       Select
B       Undo
SELECT  Execute
START   Inventory
```

and:

```text
Card description
────────────────────────
Turn timer
```

at the bottom.

If **that** feels good on the actual Game Boy, I'd stop and playtest it extensively before implementing anything more complicated.

### In short

**0. Rules → 1. UI → 2. Controls → 3. Cards → 4. Combos → 5. Attack → 6. Timer → 7. Enemy → 8. Defense → 9. Effects → 10. Deck → 11. Polish → 12. Advanced combos → 13. Multiple enemies → 14. Graphics**

That ordering minimizes wasted work and gives you a playable build at almost every step.

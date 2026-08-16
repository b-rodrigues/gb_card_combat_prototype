# Game Boy Baten Kaitos–Inspired Combat Prototype

## 1. Objective

Build a very simple, highly readable Game Boy combat prototype inspired by the card-based combat of the first **Baten Kaitos**.

The prototype should prioritize:

* fast card selection
* number-based combos
* simple attack and defense
* responsive Game Boy controls
* extremely simple graphics
* a visible turn timer
* minimal menus
* reusable tile-based UI

Do **not** attempt to reproduce the original game's graphics.

For the first prototype, cards are represented entirely by **two-letter abbreviations plus a number**.

Examples:

```text
SW3
SH5
BO2
FI4
HE7
```

The goal is to prove that the card-combat loop is fun and understandable before investing in proper card artwork or complex effects.

---

# 2. Card Representation

Every card has:

```text
TYPE + NUMBER
```

Examples:

```text
SW3
SH5
BO2
FI4
HE7
```

The two-letter code communicates the card type.

The number is the card's numerical value and is used for combo construction.

### Initial card types

| Code | Meaning | Function         |
| ---- | ------- | ---------------- |
| `SW` | Sword   | Physical attack  |
| `SH` | Shield  | Defense          |
| `BO` | Bow     | Ranged attack    |
| `FI` | Fire    | Elemental attack |
| `HE` | Heal    | Restore HP       |

Keep the initial prototype to these five card types.

More can be added later.

---

# 3. No Card Graphics Initially

Do not create individual card sprites.

The card:

```text
SW3
```

should literally be rendered as text/tiles.

Likewise:

```text
SH5
BO2
FI4
HE7
```

The Game Boy already has everything needed to display letters and numbers.

This dramatically reduces the graphical requirements and makes it possible to concentrate on the combat system.

Later, `SW3` could become:

```text
[small sword icon] 3
```

without changing the underlying card system.

---

# 4. Card Data

Internally, do not store cards as strings.

Use compact IDs.

For example:

```text
CARD_SWORD = 0
CARD_SHIELD = 1
CARD_BOW = 2
CARD_FIRE = 3
CARD_HEAL = 4
```

A card instance contains approximately:

```text
type
number
power
effect
```

For example:

```text
type   = CARD_SWORD
number = 3
power  = 3
effect = DAMAGE
```

The renderer converts this into:

```text
SW3
```

This separation is important because the graphics can later be replaced without rewriting the combat engine.

---

# 5. Game Boy UI Philosophy

The Game Boy screen is only 160×144 pixels.

Do not try to reproduce a modern card-game interface.

The UI should be mostly:

* background tiles
* text
* numbers
* a small cursor
* player/enemy sprites
* extremely simple effects

Cards should be rendered into the **background tilemap**, not as hardware sprites.

Reserve sprites for:

* player
* enemy
* cursor if necessary
* simple attack effects

This keeps the prototype within the Game Boy's sprite and memory limitations.

---

# 6. Battle Screen

The basic layout should be approximately:

```text
┌──────────────────────────────┐
│ SLIME              HP 30/30  │
│                              │
│          [SLIME]             │
│                              │
│ HERO               HP 40/40  │
│                              │
│ COMBO: SW2 BO3 SW4           │
│                              │
│ SW2  SH5  BO3  SW4  HE7      │
│              ^               │
│ Sword attack. Deals damage.  │
│██████████████████████████████│
└──────────────────────────────┘
```

The screen has four important areas:

1. **Enemy information**
2. **Current combo**
3. **Player's hand**
4. **Card description + turn timer**

The timer always occupies the bottom line.

---

# 7. Hand

The player initially has a five-card hand.

Example:

```text
SW2  SH5  BO3  SW4  HE7
```

The cards are arranged horizontally.

The cursor moves between them.

Example:

```text
SW2  SH5  BO3  SW4  HE7
 ^
```

Move right:

```text
SW2  SH5  BO3  SW4  HE7
      ^
```

Move right again:

```text
SW2  SH5  BO3  SW4  HE7
           ^
```

The cursor should be extremely obvious.

A simple arrow underneath the selected card is sufficient.

---

# 8. Card Description

There is **no card information menu**.

The description of the currently highlighted card is **always visible** immediately above the timer.

For example:

```text
SW2  SH5  BO3  SW4  HE7
 ^
Sword attack. Deals physical damage.
████████████████████████████████
```

Move to Shield:

```text
SW2  SH5  BO3  SW4  HE7
      ^
Blocks incoming damage.
████████████████████████████████
```

Move to Bow:

```text
SW2  SH5  BO3  SW4  HE7
           ^
Ranged physical attack.
████████████████████████████████
```

Descriptions should be **one very short sentence**.

Do not display lengthy card explanations.

---

# 9. Turn Timer

The entire bottom row is a visual turn timer.

It starts full:

```text
████████████████████████████████
```

and decreases **from right to left**.

For example:

```text
████████████████████████████████
```

then:

```text
██████████████████████████
```

then:

```text
██████████████
```

then:

```text
██████
```

then:

```text
██
```

then:

```text
```

The player should be able to understand the remaining decision time without reading a number.

### Timer rules

* Timer starts when player input becomes available.
* Timer runs only during decision-making.
* Timer pauses during attack/defense animations.
* Timer pauses while inventory is open.
* Timer should not run during battle transitions.
* The bar should flash when it becomes critically low.
* Do not depend on color because the target is the original Game Boy.
* Use blinking or animation for the low-time warning.

---

# 10. Timer Expiration

When the timer reaches zero:

### If cards have been selected

Automatically execute the current combo.

Example:

```text
COMBO: SW2 BO3 SW4
```

Timer reaches zero.

The game automatically performs:

```text
SW2 BO3 SW4
```

This prevents the player from simply losing their turn because they failed to press a button.

### If no cards have been selected

End the action without an attack.

Optionally play a short warning sound.

This can be changed later if testing shows that automatically selecting the first available card feels better.

---

# 11. Button Mapping

The control scheme should respect the existing prototype.

### D-Pad

**LEFT**

Move to previous card.

**RIGHT**

Move to next card.

**UP / DOWN**

Used for context-dependent navigation, primarily target selection when multiple targets exist.

For a single-enemy battle, UP/DOWN can simply do nothing.

### A

**Select card.**

A adds the currently highlighted card to the current combo.

After selecting the card, automatically advance the cursor to the next card.

### B

**Undo / cancel.**

Remove the most recently selected card.

If no cards have been selected, cancel the current action.

### SELECT

**Execute the current action.**

This replaces START as the combat confirmation button.

There is no secondary confirmation screen.

### START

**Open inventory.**

START retains its existing prototype behavior.

Opening inventory pauses combat and pauses the timer.

When the inventory closes, return to exactly the same battle state.

---

# 12. Basic Interaction

The primary interaction should be:

```text
D-PAD
  ↓
Highlight card
  ↓
Read description
  ↓
A
  ↓
Add card
  ↓
Cursor advances
  ↓
A
  ↓
Add another card
  ↓
...
  ↓
SELECT
  ↓
Execute combo
```

Undo:

```text
B
↓
Remove last selected card
```

Inventory:

```text
START
↓
Inventory
```

This keeps the control scheme extremely simple.

---

# 13. Combo Display

Selected cards appear in a dedicated area above the hand.

For example:

```text
COMBO: SW2 BO3 SW4
```

The hand remains below:

```text
SW2  SH5  BO3  SW4  HE7
```

Selected cards should have some simple visual indication in the hand, such as:

* inverted tiles
* an underline
* a small marker
* blinking

No special graphics are required.

---

# 14. Selecting Cards

Suppose the hand is:

```text
SW2  SH5  BO3  SW4  HE7
 ^
```

Press A.

The game displays:

```text
COMBO: SW2

SW2  SH5  BO3  SW4  HE7
      ^
```

The cursor automatically advances.

Press A again:

```text
COMBO: SW2 SH5
```

Continue until the desired sequence has been created.

---

# 15. Undo

Suppose:

```text
COMBO: SW2 BO3 SW4
```

Press B.

The result becomes:

```text
COMBO: SW2 BO3
```

The cursor should return to the removed card.

This allows the player to correct mistakes quickly without restarting the turn.

---

# 16. Number-Based Combos

The central prototype mechanic is the number sequence.

The first version should implement only **ascending straights**.

Example:

```text
SW2 → SW3 → SW4
```

produces:

```text
2 → 3 → 4

STRAIGHT!
```

A non-sequence:

```text
SW2 → BO4 → FI5
```

does not produce a straight because `3` is missing.

The card types do not have to match.

This is important.

A combo can be:

```text
SW2 → BO3 → FI4
```

and still be a valid straight.

The numbers are what matter.

However, if the types match, then the combo gets a multiplier bonus:

```text
SW2 → SW3 → SW4
```

---

# 17. Combo Length

Initially support:

### 2 cards

Normal combination.

### 3 cards

Straight bonus.

### 4 cards

Larger straight bonus.

### 5 cards

Maximum initial combo.

Example:

```text
2 → 3
```

Normal.

```text
2 → 3 → 4
```

3-card straight.

```text
2 → 3 → 4 → 5
```

4-card straight.

```text
1 → 2 → 3 → 4 → 5
```

5-card straight.

The exact multipliers should remain simple.

---

# 18. Simple Damage Formula

Do not use floating-point arithmetic.

Use integer percentages.

For example:

```text
Base damage = sum of card values
```

Then apply a combo multiplier.

Example:

```text
SW2 + BO3 + SW4

Base = 9

3-card straight = ×150%

Final damage = 13
```

Use integer arithmetic:

```text
damage = base * multiplier / 100
```

Possible initial multipliers:

```text
Normal combo: 100
3-card straight: 150
4-card straight: 175
5-card straight: 200
```

These numbers can be tuned later.

---

# 19. Card Types

## SW — Sword

Physical attack.

Example:

```text
SW3
```

Description:

```text
Sword attack. Deals physical damage.
```

Damage contribution:

```text
3
```

---

## SH — Shield

Defense.

Example:

```text
SH5
```

Description:

```text
Blocks incoming damage.
```

When used defensively:

```text
Defense = 5
```

Multiple Shield cards can be combined.

---

## BO — Bow

Ranged physical attack.

Example:

```text
BO4
```

Description:

```text
Ranged physical attack.
```

Initially, Bow can behave almost identically to Sword.

Its distinction can be expanded later.

---

## FI — Fire

Elemental attack.

Example:

```text
FI4
```

Description:

```text
Fire elemental attack.
```

Initially, simply treat it as an attack with a Fire type.

Elemental weaknesses can be added later.

---

## HE — Heal

Restores HP.

Example:

```text
HE5
```

Description:

```text
Restores 5 HP.
```

For the first version, simply restore the number shown on the card.

---

# 20. Attack Resolution

When SELECT is pressed:

1. Stop player input.
2. Stop the timer.
3. Evaluate the selected cards.
4. Determine whether a straight exists.
5. Calculate base damage/effect.
6. Apply combo bonus.
7. Display the result.
8. Play a short animation.
9. Apply the result to the enemy.
10. Check for victory.
11. Begin enemy turn.

Example:

```text
COMBO: SW2 BO3 SW4

2 → 3 → 4

STRAIGHT!

DAMAGE: 13
```

Then:

```text
ENEMY HP
30 → 17
```

---

# 21. Simple Attack Animation

Do not require sophisticated graphics.

For the first version:

```text
COMBO!
```

then:

```text
ATTACK!
```

then:

```text
-13
```

Then update HP.

If a placeholder projectile is desired, a simple sprite moving across the screen is sufficient.

The combat system must work perfectly without the animation.

---

# 22. Enemy Turn

Use a single enemy initially.

Example:

```text
SLIME
HP 30/30
```

The enemy can have extremely simple AI.

For example:

```text
Normal attack
Normal attack
Heavy attack
Normal attack
```

or a small weighted random table.

The enemy does not need to use the card system yet, but will in the future.

---

# 23. Defense

When the enemy attacks:

```text
SLIME ATTACK!
DAMAGE: 15
```

Switch the player into defense mode.

The hand is shown:

```text
SW2  SH5  BO3  SH4  HE7
      ^
```

The description changes automatically:

```text
Blocks incoming damage.
```

The player selects Shield cards.

Example:

```text
COMBO: SH4 SH5
```

SELECT executes the defense.

Calculate:

```text
Defense = 4 + 5
Enemy damage = 15
Damage taken = 6
```

Display:

```text
BLOCK: 9

TAKEN: 6
```

Then continue the battle.

---

# 24. Healing

If the player selects:

```text
HE5
```

and presses SELECT:

```text
+5 HP
```

The player's HP updates immediately.

For the initial prototype, Heal does not need complicated combo rules.

---

# 25. Multiple Targets

Do not implement complicated target selection initially.

With one enemy:

**Target automatically.**

When multiple enemies are eventually added:

* LEFT/RIGHT changes target.
* A can confirm the target if necessary.
* A small arrow identifies the selected enemy.

Example:

```text
        ↓
     [SLIME]

     [BAT]
```

This can be added after the one-enemy prototype works.

---

# 26. Inventory

START should continue opening the existing inventory.

While inventory is open:

* combat is paused
* timer is paused
* current hand remains unchanged
* current combo remains unchanged
* cursor position remains unchanged

Example:

Before inventory:

```text
COMBO: SW2 BO3

SW2 SH5 BO3 SW4 HE7
           ^
```

Open inventory with START.

Close inventory.

Return to:

```text
COMBO: SW2 BO3

SW2 SH5 BO3 SW4 HE7
           ^
```

Nothing should reset.

---

# 27. Sound Design

Because the graphics are extremely simple, sound is important.

Use short Game Boy sound effects.

### Card selection

Short click.

### Undo

Lower click.

### Combo detected

Short rising tone.

### Straight

Distinct positive sound.

### Attack

Short impact.

### Defense

Block sound.

### Heal

Positive ascending tone.

### Low timer

Periodic warning beep.

### Victory

Simple victory jingle.

These sounds can provide much of the feedback that graphics cannot.

---

# 28. Timer Audio

When the timer becomes low, the game should provide additional feedback.

For example:

```text
████████████████████
```

normal.

Then:

```text
██████
```

timer begins flashing.

Play a short warning beep at intervals.

This allows the player to recognize that they need to commit their action without constantly watching the bar.

Do not make the warning too aggressive initially.

---

# 29. Game State Machine

The combat engine should be implemented as an explicit state machine.

Suggested states:

```text
BATTLE_START
    ↓
PLAYER_DRAW
    ↓
PLAYER_SELECT
    ↓
PLAYER_COMBO
    ↓
PLAYER_RESOLVE
    ↓
CHECK_BATTLE_END
    ↓
ENEMY_TURN
    ↓
ENEMY_ATTACK
    ↓
PLAYER_DEFENSE
    ↓
DEFENSE_RESOLVE
    ↓
CHECK_BATTLE_END
    ↓
NEXT_TURN
```

Additional temporary states:

```text
ATTACK_ANIMATION
DAMAGE_DISPLAY
HEAL_DISPLAY
COMBO_DISPLAY
VICTORY
DEFEAT
INVENTORY
```

---

# 30. Timer State

The timer should be tied specifically to the player decision state.

For example:

```text
PLAYER_COMBO
    ↓
timer running
    ↓
SELECT
    ↓
timer stopped
    ↓
PLAYER_RESOLVE
```

Or:

```text
PLAYER_COMBO
    ↓
timer reaches zero
    ↓
automatically resolve
```

Never let the timer run during:

* animations
* dialogue
* inventory
* damage display
* victory/defeat
* transitions

---

# 31. Deck

Initially use a small fixed deck.

Example:

```text
SW2
SW3
SW4
SW5

SH2
SH4
SH5

BO2
BO3
BO5

FI3
FI4

HE2
HE5
```

Start with a deterministic deck order for debugging.

Once the system works, implement shuffling.

The player should draw replacement cards after cards are played according to the eventual deck rules.

---

# 32. Randomness

Do not introduce lots of randomness in the first version.

The first prototype should be deterministic enough to test combos.

Randomization can later control:

* deck shuffle
* enemy actions
* damage variation
* special effects

The core card-combination system should remain predictable.

---

# 33. Memory and Hardware Considerations

The prototype should avoid:

* large card sprites
* individual sprite objects for every card
* floating-point calculations
* dynamic memory allocation
* large strings stored repeatedly
* complicated particle systems
* unnecessary animation frames

Prefer:

* card IDs
* small fixed arrays
* integer arithmetic
* lookup tables
* tilemap rendering
* reusable icons/tiles
* fixed-size battle structures

The hand can simply be an array of five card IDs.

The current combo can be another small fixed array.

---

# 34. Recommended Initial Data Structures

Conceptually:

```text
Card
    type
    number
    power
```

Player:

```text
Player
    hp
    max_hp
    deck[]
    hand[5]
    combo[]
    combo_length
    cursor_position
```

Battle:

```text
Battle
    state
    enemy_hp
    enemy_max_hp
    timer
    selected_target
```

There is no need for anything more complicated in the first prototype.

---

# 35. Rendering Strategy

Use the Game Boy background tilemap for:

* HP text
* enemy text
* combo text
* card text
* descriptions
* timer bar
* cursor

Use sprites only for:

* player character
* enemy
* optional attack effect

This makes the combat UI cheap to render and easy to update.

---

# 36. Timer Rendering

The timer can be represented by a row of repeated tiles.

For example, a 20-tile bar:

```text
████████████████████
```

Each timer tick removes one or more tiles from the **right side**.

This is visually simple and cheap.

Conceptually:

```text
timer_tiles = 20
```

As time decreases:

```text
20 → 19 → 18 → ... → 1 → 0
```

Only redraw the affected section rather than rebuilding the entire screen.

---

# 37. First Prototype Battle

The first playable battle should contain:

### Player

```text
HP: 40
```

### Enemy

```text
SLIME
HP: 30
```

### Hand

```text
SW2 SH5 BO3 SW4 HE7
```

### Turn

Player gets a fixed decision period.

The timer starts:

```text
████████████████████████████████
```

Player selects:

```text
SW2 → BO3 → SW4
```

The screen shows:

```text
COMBO: SW2 BO3 SW4
```

SELECT executes.

The game detects:

```text
2 → 3 → 4
STRAIGHT!
```

Calculates damage.

Displays:

```text
DAMAGE: 13
```

Enemy HP becomes:

```text
17/30
```

Enemy attacks.

Player receives a defense opportunity.

Player selects Shield cards.

SELECT resolves the defense.

The loop continues until:

```text
VICTORY
```

or:

```text
DEFEAT
```

---

# 38. Development Order

Implement the system in this exact order.

### Phase 1 — Card Rendering

Make the Game Boy display:

```text
SW2 SH5 BO3 FI4 HE7
```

with a movable cursor.

### Phase 2 — Input

Implement:

```text
LEFT / RIGHT
A
B
SELECT
START
```

and verify:

* A selects
* B undoes
* SELECT executes
* START opens inventory

### Phase 3 — Combo

Implement:

```text
2 → 3 → 4
```

detection.

### Phase 4 — Timer

Add the full-width bottom bar.

Make it shrink from right to left.

### Phase 5 — Attack

Add damage calculation and enemy HP.

### Phase 6 — Defense

Add Shield cards and enemy attacks.

### Phase 7 — Heal

Add HE cards.

### Phase 8 — Enemy AI

Add simple enemy behavior.

### Phase 9 — Deck

Add card drawing and shuffling.

### Phase 10 — Presentation

Only after the combat works:

* improve sprites
* add attack animations
* add more card types
* add elemental interactions
* add better sound
* add more sophisticated combos

---

# 39. Final Prototype UI

The target experience should look roughly like this:

```text
┌──────────────────────────────┐
│ SLIME              HP 17/30  │
│                              │
│          [SLIME]             │
│                              │
│ HERO               HP 40/40  │
│                              │
│ COMBO: SW2 BO3 SW4           │
│                              │
│ SW2  SH5  BO3  SW4  HE7      │
│              ^               │
│ Sword attack. Deals damage.  │
│██████████████████████████████│
└──────────────────────────────┘
```

The player's mental process should be:

> "I'm highlighting SW4. It says what it does at the bottom. I've already selected SW2 and BO3. I have 2-3-4, so that's a straight. I press SELECT."

That is the entire core loop.

The prototype does **not** need beautiful cards, elaborate menus, or sophisticated animations.

It needs to make this interaction feel **fast, obvious, and satisfying**.

## Core rules to preserve

**A = select card**

**B = undo**

**SELECT = execute**

**START = inventory**

**D-Pad = navigate**

**Bottom line = always-visible card description**

**Very bottom line = right-to-left turn timer**

**Cards = two letters + number**

**Combos = primarily based on card numbers**

**Graphics = tile/text based wherever possible**

That should be the foundation of the first playable Game Boy implementation.

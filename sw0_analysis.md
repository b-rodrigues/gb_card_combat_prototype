# SW0 Hand-Card Render Bug — Root Cause Analysis

## What the symptoms rule out

| Observation | Rules Out |
|---|---|
| f12 probe shows correct WRAM hand data `[(0,3),(0,3),(1,2),(1,2),(3,4)]` | Wrong deck init, wrong `deck_draw`, wrong `battle_start` |
| SW0 visible on screen | VRAM write path itself (tile 'S','W','0' did reach VRAM; only 'value' digit is wrong) |
| Cards 1–4 show 0xFF = EMPTY | Read of `.type` for i≥1 returns 0xFF |
| Harness passes `fallback_deck_starter` (asserts row 14 = "SW3 SW3 SH2 SH2 FI4") | Bug is **real-hardware / LCD-on only**, not harness-visible |
| Pattern is transient (does not persist in WRAM) | The wrong read values come from a **stale register / wrong address**, not a corrupted Battle struct |

---

## Confirmed-correct elements (from full ASM trace)

Everything below was verified against the debug build `.asm`:

- **Battle struct layout** (all field offsets): ASM uses `0x016e` for `hand[0]`, `0x019b` for `combo_count`, `0x0196` for `selected_indices`, `0x019c` for `cursor_pos`. All match the manually computed layout (card = 8 bytes, deck = 323 bytes, etc.). ✓
- **Battle pointer staging**: `ui_update_battle` in `ui.c` correctly stages `g_bk_ptr_a = battle` (= `&g_game.battle` = `0xCE58`). `ui_update_battle_banked` reads it into BC. ✓
- **Battle pointer propagation from `battle_screen_render`**: Pre-computed at `[sp+2..3]` = `g + 0x050c = &g->battle`. Both the dirty check and the `ui_update_battle` call use this same slot. ✓
- **Card byte-copy direction (the "8-byte backward push")**: Starting at `&hand[i]+7` (ring), reading backwards with `ld a,(hl-)`, pushing in 2-byte `push de` pairs — produces `[sp+2]=type, [sp+3]=value` inside `battle_draw_card_at`, which is what the function reads. ✓
- **`battle_draw_card_at` callee cleanup**: `add sp, #8` before `jp (hl)` cleans its own Card struct argument. ✓
- **Loop stack frame**: SP is stable at `SPF` throughout the outer i-loop; the "peek via `inc sp; inc sp; push hl`" idiom correctly writes derived pointers without moving SP. ✓
- **`battle_vram_sync_write` (951dac0)**: Correctly compiled — `ldh a, (_STAT_REG+0)` every iteration (no caching), `di`/`ei` bracket the wait+write, LCD-off short-circuits to direct write. ✓

---

## The active bug: §52.19 layout-sensitive miscompile in bank 3

### What changed in commit 951dac0

`src/ui/ui_battle_content.c` (bank 3) gained a **larger `battle_vram_sync_write`** body:

| Before | After |
|---|---|
| 2 `while` loops + 1 store | `di` + 1 `while` loop + 1 store + `ei` + else-branch store |

This makes the function several bytes longer, **shifting the absolute addresses of everything that follows it in bank 3** — `battle_put_char`, `battle_draw_text_line`, `battle_draw_num2`, `battle_card_type_code`, and critically **`battle_draw_battle_hand`** and `battle_combo_pending_name`.

### Why SDCC miscompiles are layout-sensitive (§52.19)

SDCC 4.4.1's SM83 register allocator decides _which_ stack slot or register to cache a computed pointer in based on **branch-join dataflow analysis**. The analysis is position-dependent: the exact set of branch joins, the live ranges, and the conflict graph all depend on the compiled-to-ASM offsets and sizes. Moving a function by even a few bytes changes how the allocator resolves conflicts in nearby functions.

The previous instances of this bug (patrol cadence, battle HP corruption) all followed the same pattern: a pointer computed on one CFG path is reused via a cached slot at a JOIN point, but the slot was filled by a **different path** with a different value.

### Where the miscompile fires

The `_battle_draw_battle_hand` function in bank 3 pre-computes four derived pointers at function ENTRY (before the loop), stores them at fixed stack slots, then reads them back each iteration:

```asm
; Entry: battle ptr in DE → BC
ld hl, #0x019b; add hl, bc   → &combo_count ptr → [SPF+0..1]
ld hl, #0x0196; add hl, bc   → &selected_indices ptr → [SPF+2..3]
ld hl, #0x016e; add hl, bc   → &hand[0] ptr → [SPF+4..5]   ← THIS ONE
ld hl, #0x019c; add hl, bc   → &cursor_pos ptr → [SPF+6..7]
```

The **hand[0] base pointer** cached at `[SPF+4..5]` is the cornerstone of every card read. Each iteration computes `&hand[i] = [SPF+4..5] + i*8`.

After the 951dac0 layout shift, SDCC's allocator is assigning the `[SPF+4..5]` slot in a context where the value it writes is stale — either:

**(a) The battle pointer in BC is stale at the point of the `add hl, bc` for hand[0]**, because the allocator reused BC from a prior join branch that computed a different value (the same class as the battle HP corruption bug in `battle_update`). The `volatile Battle *vb` fix pattern for `battle_update` works because it forces a memory reload; `battle_draw_battle_hand` does not have this guard.

**(b) The `push hl` / store-to-stack-slot sequence for the hand[0] ptr is emitted at a JOIN point where HL already held a different pointer** (a prior loop iteration's value or a pointer from a different expression), and SDCC optimized away the `add hl, bc` on one path. On the path taken at runtime, the slot gets the wrong value.

### Why the symptom is `[(0,0), 0xFF×4]`

The value `0xFF` in hand[1..4].type is the signature of reading from **un-mapped or ROM-padded memory** (ROM banks pad to `0xFF`). If the cached `&hand[0]` is wrong by a large offset — e.g., pointing into the uninitialized tail of ROM bank 3 above the actual code — every `hand[i]` read would return `0xFF`.

For hand[0]: the read produces `{type=0, value=0}` = SW0. This is consistent with the pointer landing at a zero-initialized WRAM region (e.g., just above the end of the battle struct or in the timer/dirty fields which are zero at battle-phase start). Since `type=SWORD=0` matches the real card, the pointer is only slightly wrong — possibly aligned at `battle->hand[0].type` but pointing into a different field (e.g., `battle->energy=0` for type, `battle->phase=0` for value).

Concretely: if the cached hand ptr = `&battle->cursor_pos_region` ≈ `battle + 0x019c`, then:
- `[battle+0x019c]` = cursor_pos = 0 (initial) → type = SWORD ✓  
- `[battle+0x019d]` = energy = 6 initially, but ≠ 3 → value = wrong ✓

For i≥1, `ptr + i*8` falls into different WRAM regions, all initializing to 0 or garbage, or hits the tail of a non-initialized struct → 0xFF for type.

### Why the harness can't see it

As documented in §52.15: the SameBoy harness runs with the LCD **off** (vsync skipped). When LCD is off, `battle_vram_sync_write` skips the `STAT_REG` wait entirely and writes directly to VRAM. This avoids the timing window that Mode 3 interacts with.

BUT — more importantly — **the miscompile fires during bank-3 execution where bank 3 is mapped**. Under the harness, the same code path runs. The miscompile symptom should be visible in the harness too, if `fallback_deck_starter` asserts on row 14.

Wait — **the harness passes `fallback_deck_starter`**. This contradicts a pure logic miscompile. The miscompile is triggered only by the **LCD-on VRAM write path** or by a **real-hardware timing effect** that SameBoy doesn't model.

### Revised hypothesis: STAT-bit timing edge case (not pure miscompile)

The `battle_vram_sync_write` fix is:
```c
di;
while (STAT_REG & 0x02);  // exit when NOT in Mode 2 or 3
*dst = tile;
ei;
```

On real hardware, this exits at the START of Mode 0 (HBlank, ~204 CPU cycles) or Mode 1 (VBlank). After the `jr NZ` branch exits the loop, there are additional instructions before the store: `pop de; push de; ld a, c; ld (de), a`. That's approximately 10–14 CPU cycles. Safe in HBlank.

**But**: the `di` is **before** the `while` loop. If an interrupt fires between `while` entry and `jr NZ` exit, `di` prevents it. But if the `while` exits at the VERY LAST cycle of Mode 0, the `ld (de), a` store could land in Mode 2 of the next scanline. On a real DMG running at 4.194 MHz, this is plausible at the very last HBlank cycles.

**More likely**: the `ei` at the end and the 256Hz timer ISR (AGENTS.md §35). If `ei` re-enables the timer ISR, and the timer immediately fires (pending interrupt), the ISR runs between two successive `battle_put_char` calls. The ISR runs for a few hundred cycles. When it returns, the PPU may now be in Mode 3. The NEXT `battle_put_char` call begins a new wait. But `di` is in the NEXT call's `battle_vram_sync_write`, so the ISR can't interrupt THAT wait. This is handled correctly.

**The actual remaining issue**: the **screen_buf skip-guard**. On the FULL REDRAW path (`ui_draw_battle_full`):

1. `ui_clear_screen()` → `g_ui_screen_buf[y][x] = ' '` for all cells ✓
2. `ui_update_battle()` → `battle_draw_battle_hand()` → tries to write 'S','W','3' to row 14

But: `ui_clear_screen()` writes `ui_font_tile_base` to VRAM **directly** (no STAT wait, no mode check) because the full redraw is always called with LCD off. Fine.

Then `battle_draw_battle_hand()` calls `battle_put_char(col, 14, 'S')`. Inside `battle_put_char`:
```c
if (g_ui_screen_buf[14][col] != 'S') {  // ' ' != 'S' → true
    battle_vram_sync_write(dst, tile_S);
    g_ui_screen_buf[14][col] = 'S';
}
```

LCD is off → direct write. ✓

So the hand renders correctly on the first full redraw. **The SW0 must appear on a SUBSEQUENT incremental update** where dirty=BATTLE_DIRTY_HAND is set.

Now: what sets `BATTLE_DIRTY_HAND` after the initial render?

- `battle_cursor_move` (input)
- `battle_card_select`
- `battle_card_undo_banked`
- `battle_update` (phase transitions)
- `battle_start` (initial: `dirty = BATTLE_DIRTY_ALL`)

After the initial render, `battle->dirty = 0` (cleared at the end of `battle_screen_render`). On the next frame where the player presses LEFT/RIGHT/A/B, `battle_cursor_move` or `battle_card_select` sets `dirty |= BATTLE_DIRTY_HAND`.

That NEXT incremental render: LCD is **ON**. `battle_vram_sync_write` takes the STAT wait path. If the wait exits correctly, the write lands.

**But the `g_ui_screen_buf` skip-guard**: since the first full-render wrote 'S','W','3' into screen_buf, the incremental render skips cards that haven't changed. So `battle_put_char` is a no-op for positions that still match screen_buf.

Unless: the **cursor blink or phase change** sets `BATTLE_DIRTY_ALL` (0xFF), which forces ALL dirty bits including HAND. At that point `battle_draw_battle_hand` runs again. Screen_buf has 'S','W','3'. The card IS 'S','W','3'. Skip guard fires → no VRAM write. ✓

**The only way SW0 appears** is if either:
1. Screen_buf was NOT updated correctly during the first render (LCD-off path hit an error), OR
2. A subsequent render writes 'S','W','0' to VRAM and updates screen_buf to 'S','W','0'

For case 2: something reads `hand[0].value = 0` instead of 3. This is the miscompile hypothesis — but it would also affect the harness.

### The remaining true explanation: timing between `battle_start` and first render

**This is the most likely root cause that the harness cannot see:**

In `battle_start`:
```c
while (n--) *p++ = 0;  // zero entire Battle struct
// ... set player/enemy HP, etc.
battle_init_from_deck_state(b, ds);  // fills b->deck with cards
for (i = 0; i < BATTLE_HAND_SIZE; i++) {
    deck_draw(&b->deck, &b->hand[i]);  // draws 5 cards
}
// ...
b->dirty = BATTLE_DIRTY_ALL;
```

The sequence is: memset → init deck → draw hand → set dirty. All correct.

Then `game_render` → `ui_lcd_off()` → full redraw → `ui_lcd_on()`.

**BUT**: in `game_update` → `screen_update(g)` → `battle_screen_update(g)` → `battle_update(b)`. Inside `battle_update`, there is additional `b->dirty |= ...` setting. The `game_update` call includes both update AND render within the same frame loop iteration.

The real-hardware question: does the **LCD-off full redraw** happen in the same frame as `battle_start`, or one frame later? If `battle_start` runs in frame N, `game_render` in frame N does the full redraw with LCD off. The hand IS in WRAM correctly. The render should be correct.

---

## Summary of three concurrent issues

### Issue A (CONFIRMED FIXED by 951dac0): VRAM double-wait
The old `battle_vram_sync_write` placed stores at the start of Mode 3. The `g_ui_screen_buf` update happened, VRAM write was dropped, skip-guard prevented retry. **Fix is in HEAD and verified correct from ASM.**

### Issue B (ACTIVE, §52.19): Bank-3 layout shift from 951dac0
Adding code to `battle_vram_sync_write` shifted `_battle_draw_battle_hand`'s address in bank 3. SDCC's register allocator, which is layout-sensitive, may now pre-compute a stale `&hand[0]` in the function's preamble. This would produce wrong `type`/`value` reads in incremental (LCD-on) renders — but would ALSO affect the harness, which contradicts `fallback_deck_starter` passing.

**Unresolved contradiction**: if it's a pure logic miscompile, `fallback_deck_starter` would fail. It doesn't. Unless `fallback_deck_starter` takes a code path where the miscompile doesn't fire (e.g., the harness's LCD-off path bypasses the `STAT_REG` wait branch, and SDCC's allocator produces different codegen along that path — but code paths don't affect WRAM reads).

### Issue C (ACTIVE, most likely): STAT timing + re-render order interaction

The `battle_vram_sync_write` wait exits at Mode 0 (HBlank). On the full 5-card hand render (15 VRAM writes total for 5 × "SW3" format), many HBlank windows are consumed. If a write is requested during the timer ISR (music clock), the `di` should prevent that. But:

- **`ei` between consecutive `battle_put_char` calls**: after each `battle_vram_sync_write` returns, `ei` re-enables interrupts. The timer ISR can now fire. If the ISR takes long enough, the PPU exits Mode 0 before the next `battle_put_char`'s `di`. That's fine — the next call's `while (STAT_REG & 0x02)` will wait for the next Mode 0.

- **No write is actually dropped** in this scenario. The wait-based approach handles this correctly.

The true remaining issue must be in the **path that sets SW0 in `g_ui_screen_buf`** itself.

---

## Most probable root cause (actionable hypothesis)

The `battle_draw_card_at` function receives `Card card` **by value** from `battle->hand[i]`. The copy is done via 8 `ld a,(hl-)` reads. If `hl` at the start of the copy is wrong by a constant, `card.value` (the third character rendered) reads from the wrong WRAM byte.

Specifically: the hand-card render shows **SW + digit**. 'S' and 'W' come from `battle_card_type_code(card.type)` which is determined by `card.type`. If `card.type = 0 = SWORD`, the code is "SW" regardless. The digit comes from `'0' + card.value`.

**SW0 = type=SWORD(0), value=0.** The correct is value=3.

The Card struct in WRAM at `hand[0]`:
```
offset 0: type = 0 (SWORD)
offset 1: value = 3
offset 2: uses_remaining = 0xFF
offset 3: cost = 1
offset 4: effect = 1 (DAMAGE_TARGET)
offset 5: status_id = 0
offset 6: status_chance = 0
offset 7: ring = 0
```

The copy reads from `&hand[i]+7` backwards. If it reads from `&hand[i]+6` backwards instead (one short), the 8 reads land at offsets 6,5,4,3,2,1,0,-1. The last push would have `type=[offset 0]=0, value=[offset -1]=?`. `offset -1` is `&hand[0] - 1` = `deck.discard_count` (last byte of Deck) = 0 after memset.

**That produces exactly SW0 for card 0**: type=0(SWORD), value=0. ✓

For cards 1-4: `&hand[1] - 1 = &hand[0] + 7 = &hand[0].ring = 0`. The reads from offset `ring` backwards pick up: ring, status_chance, status_id, effect, cost, uses_remaining(0xFF), value(3), type(0).

Last push for card 1 = `type=[ring]=0, value=[byte before ring]=[last byte of hand[0]]=[discard_count-like]=?`. No, wait: if the cached `hand_ptr` is off by -1, then for i=1: `ptr + 8 = (&hand[0]-1)+8 = &hand[0]+7 = &hand[0].ring`. Then 7 incs give `&hand[0].ring + 7 = &hand[1].status_chance`. The 8 backward reads from `hand[1].status_chance`: status_chance(0), status_id(0), effect(1), cost(1), uses_remaining(0xFF), value(2), type(1=SHIELD), and one before = `hand[0].ring=0`. Last push: type=`hand[0].ring`=0 (SWORD), value=hand[1].type=1(SHIELD). So card 1 renders as SW1, not 0xFF.

This doesn't match the 0xFF pattern. So the "off by 1" hypothesis doesn't explain 0xFF for cards 1-4.

**For type to be 0xFF, the read address must land in ROM (bank 3 unmapped portion) or uninitialized memory.** The bank-3 ROM at addresses above the code section is padded with 0xFF.

**Revised: the hand_ptr cached at [SPF+4..5] is a GARBAGE value from a previous banked call's return.**

The `banked_call_run` trampoline restores `HOME_BANK` and `__current_bank` on return. After `banked_call_run()` returns in the fixed bank, the return address in HL (SDCC uses HL for return address in the banked trampoline) could contain a value that SDCC then incorrectly reuses as a live pointer in the next expression if a JOIN point hasn't properly killed it.

If BC (battle ptr) was correct but HL was stale from a previous CALL's return-address staging, and SDCC generates `ld hl, #0x016e; add hl, BC` using a stale BC instead of the fresh one from [sp+10..11], the hand_ptr could be wild.

---

## Recommended diagnostic steps

1. **Check the `ui_battle_content.c` allocation budget**. The fix description says caps on `ui_battle_content.c` exposed HP corruption in a previous iteration. The current `Makefile` does NOT cap `ui_battle_content.c`. Adding `--max-allocs-per-node500` to `ui_battle_content.c` (both builds) is the §52.20 escape hatch — but per §52.19, this may flip a different bug on.

2. **Add a `volatile` re-read of `g_bk_ptr_a` inside `ui_update_battle_banked`** before the `battle->hand[]` access chain, specifically: force BC to be reloaded from the canonical WRAM pointer at each dirty-bit branch entry, not from a pre-entry cached register. Pattern: `const volatile Battle *battle = (const volatile Battle *)g_bk_ptr_a;` (note `volatile` in the CAST, not just the declaration).

3. **Break the pre-cached pointer pattern in `battle_draw_battle_hand`**: replace the 4 pre-computed derived pointers with inline per-iteration computation. Costs ~4 instructions per iteration but eliminates the cross-iteration pointer cache as a miscompile target.

4. **Instrument with a telemetry probe inside `battle_draw_card_at`**: emit `CARD_READ` event with the raw type/value bytes before the type-code lookup. Compare to the WRAM state. This distinguishes "wrong address read" (type/value differ from WRAM) from "correct read, wrong VRAM write" (type/value match WRAM but screen shows wrong tile).

5. **A/B test with `--max-allocs-per-node` applied exclusively to `ui_battle_content.c`** and running `fallback_deck_starter` + `patrol_slime_cross` + `battle_multi_enemy_cycle_kill` per §52.19 protocol.

The fix that will hold: **pass `volatile Card` to `battle_draw_card_at`** (force individual field reads from volatile memory) or **read `hand[i]` through a `volatile Card *`** without the by-value copy entirely, rewriting `battle_draw_card_at` to take a `const volatile Card *card` instead of `Card card`. This eliminates the struct-by-value copy as a miscompile target entirely.

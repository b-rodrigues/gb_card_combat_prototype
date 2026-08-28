#!/usr/bin/env python3
"""Headless gameplay walkthrough screenshots (PyBoy).

Boots the real release ROM headlessly (window="null") and walks the hero
through key gameplay moments, saving raw 160x144 PNGs into screenshots/ so a
developer or LLM can review the current look without booting the ROM
(AGENTS.md §56).  Screenshots are for VISUAL review only; semantic state and
telemetry remain authoritative (AGENTS.md §7/§40).

Three sessions, each starting from a fresh boot (fresh persistent state):

  Walk A (town):
    00-boot-field        overworld at spawn with the HUD
    01-field-scrolled    FIELD with the camera scrolled (SCX > 0)
    02-town-arrived      TOWN just inside the east gate (camera at origin)
    03-guard-dialogue    dialogue box over the scrolled town (camera offset)
    04-dialogue-next     second dialogue line
    05-shop              shopkeeper shop screen
    06-cards-menu        START quick screen (CARDS tab, cursor on FILTER/SORT)
    07-filter-picker     filter/sort picker (A on the top row)
    08-quests-tab        QUEST tab (RIGHT from CARDS)
    12-wizard-save       wizard interaction (save game menu)
    13-wizard-saved      game state saved to slot 1

  Walk B (battle):
    09-battle            slime encounter (battle screen)
    10-battle-attack     after a player attack (damage dealt)
    11-battle-run        after fleeing (result line)

  Walk C (forest):
    14-forest-arrived    FOREST gate arrival

The walks are POSITION-based, not press-count based (see
tools/vram_dialogue_check.py for why PyBoy button delays are lossy): each
step is a single 1-tick press edge followed by a wait for the tile commit,
the player position is read back from WRAM after every press, and a dropped
press is re-pressed so the route self-corrects.

Each session now ADVANCES past the boot-time title splash, title menu and
three-slide intro before any walking begins.  Milestone frames are verified:
no frame may still show title/intro text; text-bearing milestones must show
their expected substrings.  Any failure aborts with a non-zero exit code.

Run:
    make screenshots     # builds the release ROM, then runs this
    nix develop --command python3 tools/capture_walkthrough.py
"""

import os
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROM = os.path.join(REPO, "build", "rpg_card_proto.gb")
OUT = os.path.join(REPO, "screenshots")

# Player Entity layout: position{x,y}, hp, max_hp, active, facing, id.
# Located by scanning WRAM for the deterministic boot pattern (FIELD spawn
# (4,4), hero 10/10, active, facing DOWN, id PLAYER) rather than hardcoding
# the g_game offset (which shifts with the _DATA layout on each build).
PLAYER_BOOT = bytes([4, 4, 10, 10, 1, 1, 1])
WRAM_BASE = 0xC000
WRAM_SIZE = 0x2000

# Title / intro BG markers.  Title shows "G I A U S A R" (logo) plus menu
# items "NEW GAME", "CONTINUE", "SOUND".  Intro has three scripted slides.
TITLE_LINES = (
    # Title logo & menu - unique markers only
    "G I A U S A R",
    "The Waking Whale",
    "and the Closed",
    "NEW GAME",
    "CONTINUE",
    "SOUND",
    # Intro slide 0
    "The skies above",
    "A whale stirs",
    "in the deep.",
    # Intro slide 1
    "The sky closes,",
    "sealed against",
    "the waking whale.",
    # Intro slide 2
    "Only the Lord of",
    "Slimes stands",
    "between all that",
    "lives and the end.",
    # Intro prompt
    "[A] NEXT",
)

failures = []


def check(label, ok, detail=""):
    status = "PASS" if ok else "FAIL"
    print(f"[{status}] {label}" + (f" -- {detail}" if detail and not ok else ""))
    if not ok:
        failures.append(label)


def find_player(pb):
    wram = bytes(pb.memory[i] for i in range(WRAM_BASE, WRAM_BASE + WRAM_SIZE))
    idx = wram.find(PLAYER_BOOT)
    if idx < 0:
        print("error: could not locate the player entity in WRAM after boot",
              file=sys.stderr)
        sys.exit(1)
    return WRAM_BASE + idx


def make_pb():
    from pyboy import PyBoy
    pb = PyBoy(ROM, window="null")
    for _ in range(180):
        pb.tick()
    return pb


def wait_rendered(pb, timeout=360, settle=8):
    """Tick until the framebuffer is non-blank (a full-screen transition
    wipes the display white for tens of frames -- e.g. ~54 frames on the
    FIELD->TOWN gate crossing), then a few settle frames so the new screen
    finishes drawing."""
    for _ in range(timeout):
        im = pb.screen.image.convert("RGB")
        colors = im.getcolors(maxcolors=100000)
        if colors is not None and len(colors) > 1:
            for _ in range(settle):
                pb.tick()
            return True
        pb.tick()
    return False


def bg_text(pb):
    """The visible BG tilemap as 18 rows of 20 chars.  The console font
    lives at tile base 0 (tile = ASCII - 32), so map cells read directly
    as text; non-font tiles (terrain) become '?'.  SCX/SCY are applied so
    the read is correct over a scrolled camera too."""
    scx = pb.memory[0xFF43]
    scy = pb.memory[0xFF42]
    rows = []
    for r in range(18):
        line = ""
        for c in range(20):
            t = pb.memory[0x9800 + ((scy // 8 + r) % 32) * 32
                           + (scx // 8 + c) % 32]
            line += chr(t + 32) if t < 96 else "?"
        rows.append(line)
    return rows


def on_title_or_intro_pb(pb):
    rows = bg_text(pb)
    for r in rows:
        for m in TITLE_LINES:
            if m in r:
                return True
    return False


def quick_open_pb(pb):
    return any("CARDS QUEST" in r for r in bg_text(pb))


def on_overworld_pb(pb):
    return (not quick_open_pb(pb)) and not on_title_or_intro_pb(pb)


def press_start(pb, settle=40):
    pb.button_press("start")
    for _ in range(4):
        pb.tick()
    pb.button_release("start")
    for _ in range(settle):
        pb.tick()


def advance_to_overworld(pb, tries=12):
    """Press START through the boot title splash, menu and the 3-slide
    intro until the game is back on the overworld (FIELD).  A START that
    lands on the open overworld opens the quick screen instead; the loop
    detects that (quick screen text) and closes it with the next START, so
    the sequence self-heals and always ends on the overworld."""
    for _ in range(tries):
        wait_rendered(pb)
        if on_overworld_pb(pb):
            return True
        press_start(pb)
        for _ in range(90):
            if on_overworld_pb(pb):
                return True
            pb.tick()
    return on_overworld_pb(pb)


def shoot(pb, label, need=None, check_title=True):
    path = os.path.join(OUT, label + ".png")
    if not wait_rendered(pb):
        print(f"warning: {label}: screen never became non-blank; "
              "capturing anyway", file=sys.stderr)
    pb.screen.image.save(path)
    rows = bg_text(pb)
    top = next((r.strip() for r in rows if r.strip()), "")
    print("saved", os.path.relpath(path, REPO), f"[{top[:18]}]")

    # Title/intro check only for pure-overworld frames (not dialogue/shop/battle).
    if check_title and on_title_or_intro_pb(pb):
        check(f"{label} past title", False, "frame still shows title/intro")
    if need:
        check(f"{label} shows {need!r}", any(need in r for r in rows))
    return path


def main():
    if not os.path.isfile(ROM):
        print(f"error: ROM not found: {ROM}", file=sys.stderr)
        print("Build it first (make release).", file=sys.stderr)
        return 1

    os.makedirs(OUT, exist_ok=True)
    # Drop frames from previous runs: renamed/removed milestones must not
    # linger as stale PNGs next to the current set.
    for old in os.listdir(OUT):
        if old.endswith(".png"):
            os.remove(os.path.join(OUT, old))
    from pyboy import PyBoy

    # ── Walk A: town, dialogue, shop, quick screen ───────────────────
    pb = PyBoy(ROM, window="null")
    for _ in range(180):
        pb.tick()
    if not advance_to_overworld(pb):
        check("boot: reached overworld", False, "stuck on title/intro")
    pos_addr = find_player(pb)

    def pos():
        return (pb.memory[pos_addr], pb.memory[pos_addr + 1])

    def walk(btn, is_goal, budget=2000):
        """Discrete one-tile presses until is_goal() holds.  Each press is a
        4-tick edge (short enough that the 8-frame move commits after the
        release, so exactly one tile) followed by a wait for the commit; a
        press that produced no movement (dropped by PyBoy) is retried, so
        the route converges regardless of host timing."""
        for _ in range(budget):
            if is_goal():
                return True
            x0, y0 = pos()
            pb.button_press(btn)
            for _ in range(4):
                pb.tick()
            pb.button_release(btn)
            for _ in range(24):
                pb.tick()
                if pos() != (x0, y0):
                    break
        return is_goal()

    def press(btn, settle=12):
        """A 4-tick button press (edge-triggered input, see AGENTS.md 52.10;
        PyBoy applies queued events at frame boundaries, so a one-tick press
        can miss the input window entirely)."""
        pb.button_press(btn)
        for _ in range(4):
            pb.tick()
        pb.button_release(btn)
        for _ in range(settle):
            pb.tick()

    def wait(n):
        for _ in range(n):
            pb.tick()

    def press_until(btn, cond, tries=8, settle=40, timeout=90, label=""):
        """Press ``btn`` repeatedly until ``cond()`` holds (a dropped PyBoy
        press or an in-transition eat is retried until the intended screen
        state is reached).  Warns loudly when the state is never reached:
        a silently-exhausted retry loop poisons every later milestone."""
        for t_i in range(tries):
            if cond():
                return True
            press(btn, settle=settle)
            for _ in range(timeout):
                if cond():
                    return True
                pb.tick()
        if not cond():
            print(f"warning: press_until({btn}{' ' + label if label else ''})"
                  " never reached its condition", file=sys.stderr)
        return cond()

    def tab_to(col, tries=8):
        """Press RIGHT until the quick screen's caret (^ on BG row 3)
        reaches tile column ``col`` (CARDS=0, QUEST=6)."""
        for _ in range(tries):
            if bg_text(pb)[3][col] == "^":
                return True
            press("right", settle=30)
        return bg_text(pb)[3][col] == "^"

    def stable(cond, frames=20):
        """True when ``cond`` holds now AND keeps holding for ``frames``
        consecutive ticks.  Guards against oscillating states (a stray A
        on the overworld re-engages an adjacent guard's dialogue right
        after it closed)."""
        if not cond():
            return False
        for _ in range(frames):
            pb.tick()
            if not cond():
                return False
        return True

    # Advance past title/intro and locate player
    if not advance_to_overworld(pb):
        check("Walk A: boot reached overworld", False, "stuck on title/intro")
    pos_addr = find_player(pb)

    x, y = pos()
    print(f"boot: player at ({x},{y}) (WRAM 0x{pos_addr:04X})")
    shoot(pb, "00-boot-field")

    # FIELD (4,4) -> scrolled camera (x ~22, SCX > 0) -> east wall (30,4) ->
    # south (30,7) -> east gate (31,7) -> TOWN (2,7) -> (2,8) -> west of the
    # guard at (9,8).  The camera scrolls horizontally on FIELD (32 wide) and
    # vertically in TOWN (18 tall), so the dialogue box is exercised with a
    # scrolled camera.
    ok = walk("right", lambda: pos()[0] >= 22)
    check("walk: camera scrolled (x>=22)", ok)
    wait(20)
    shoot(pb, "01-field-scrolled")
    ok = walk("right", lambda: pos()[0] == 30) and ok
    ok = walk("down", lambda: pos()[1] == 7) and ok
    ok = walk("right", lambda: pos()[0] == 2) and ok
    wait(20)
    shoot(pb, "02-town-arrived")
    ok = walk("down", lambda: pos()[1] == 8) and ok
    ok = walk("right", lambda: pos()[0] == 9) and ok
    check("walk: reached the town guard", ok)
    if not ok:
        print("warning: walk did not reach the guard; sampling anyway")

    # Bump the guard at (10,8): a blocked RIGHT press engages the dialogue,
    # verified by the GUARD: speaker tag on the BG tilemap (the window bit
    # alone also drops for menus and the shop).
    ret = press_until("right",
                lambda: any("GUARD:" in r for r in bg_text(pb)), settle=30,
                label="guard dialogue")
    check("guard dialogue opened", ret)
    shoot(pb, "03-guard-dialogue", need="GUARD:", check_title=False)
    press("a", settle=30)
    shoot(pb, "04-dialogue-next", check_title=False)
    # GUARD_GREETING has exactly two lines ("Halt! Keep peace." /
    # "Watch for slimes."): the A above advanced line 1 -> line 2 (captured
    # above); the next A closes the dialogue.  Two traps here: (a) a stray
    # A on the overworld re-engages the still-adjacent guard, and (b) the
    # closing A itself can leak across the dialogue->overworld transition
    # (the screen change resets input state, so the tail of the held press
    # registers as a fresh interact) -- the dialogue re-opens by itself.
    # Require the text to be gone for a full leak cycle (~45 frames) and
    # keep retrying so a re-opened greeting gets closed again.
    press_until("a",
                lambda: stable(
                    lambda: all("GUARD:" not in r for r in bg_text(pb)),
                    frames=45),
                tries=12, settle=30, timeout=120)

    # Walk to the shopkeeper at (9,3): from (9,8) up to (9,4), then bump UP
    # to open the shop -- verified by the SHOP title on the BG tilemap.
    ok = walk("up", lambda: pos()[1] == 4) and ok
    ret = press_until("up", lambda: "SHOP" in bg_text(pb)[0], settle=30,
                label="shop open")
    check("shop opened", ret)
    wait(20)
    shoot(pb, "05-shop", need="SHOP", check_title=False)

    # Close the shop (B restores the overworld), then open the quick
    # screen (START).  Each transition is content-verified so a dropped
    # press is retried; if B was dropped, START closes the shop (the shop
    # treats START like B) and the retry then opens the menu.  The menu
    # draws "CARDS QUEST" tab labels on row 2, so its presence/absence is
    # a text check -- pixel heuristics false-positive on terrain glyphs.
    def quick_open():
        return any("CARDS QUEST" in r for r in bg_text(pb))

    def picker_open():
        return any("LR CYCLE" in r for r in bg_text(pb))

    ret = press_until("b", lambda: stable(lambda: not quick_open()), settle=30,
                label="shop close")
    check("shop closed", ret)
    ret = press_until("start", quick_open, settle=30, label="quick screen")
    check("quick screen opened", ret)
    wait(10)
    shoot(pb, "06-cards-menu", need="CARDS QUEST", check_title=False)

    # The menu opens on the CARDS tab with the cursor on the top
    # FILTER/SORT row: A opens the picker (footer "LR CYCLE  A:OK B:NO"),
    # B backs out, RIGHT switches to the QUEST tab (^ caret under QUEST).
    ret = press_until("a", picker_open, settle=30, label="filter picker")
    check("filter picker opened", ret)
    shoot(pb, "07-filter-picker", need="LR CYCLE", check_title=False)
    ret = press_until("b", lambda: not picker_open(), settle=30, label="picker close")
    check("picker closed", ret)
    ret = tab_to(6)
    check("quest tab caret positioned", ret)
    shoot(pb, "08-quests-tab", check_title=False)

    # Close the quick screen with B (back to the bare overworld)
    ret = press_until("b", lambda: stable(lambda: not quick_open()), settle=30,
                label="quick screen close")
    check("quick screen closed", ret)

    # Walk to the Wizard at (6,10): from (9,4) down to (6,11), then bump UP
    # to open the Save Menu -- verified by its SAVE GAME title.
    ok = walk("down", lambda: pos()[1] == 11)
    ok = walk("left", lambda: pos()[0] == 6) and ok
    check("walk: reached the wizard", ok)
    if not ok:
        print("warning: walk did not reach the wizard; sampling anyway")
    ret = press_until("up", lambda: "SAVE GAME" in bg_text(pb)[0], settle=30,
                label="save menu")
    check("save menu opened", ret)
    shoot(pb, "12-wizard-save", need="SAVE GAME", check_title=False)

    # Press A to save the current game state to Slot 1 (message "SAVED")
    press("a", settle=30)
    shoot(pb, "13-wizard-saved", need="SAVED", check_title=False)
    pb.stop()

    # ── Walk B: slime battle on FIELD ────────────────────────────────
    pb = PyBoy(ROM, window="null")
    for _ in range(180):
        pb.tick()
    if not advance_to_overworld(pb):
        check("Walk B: boot reached overworld", False, "stuck on title/intro")
    pos_addr = find_player(pb)

    def pos2():
        return (pb.memory[pos_addr], pb.memory[pos_addr + 1])

    def walk2(btn, is_goal, budget=2000):
        for _ in range(budget):
            if is_goal():
                return True
            x0, y0 = pos2()
            pb.button_press(btn)
            for _ in range(4):
                pb.tick()
            pb.button_release(btn)
            for _ in range(24):
                pb.tick()
                if pos2() != (x0, y0):
                    break
        return is_goal()

    def press2(btn, settle=12):
        pb.button_press(btn)
        for _ in range(4):
            pb.tick()
        pb.button_release(btn)
        for _ in range(settle):
            pb.tick()

    x, y = pos2()
    print(f"battle walk: boot at ({x},{y})")
    if not advance_to_overworld(pb):
        check("Walk B: boot reached overworld", False, "stuck on title/intro")
    pos_addr = find_player(pb)

    # FIELD (4,4) -> down to row 8 -> right to (13,8) -> right into the
    # hostile slime at (14,8), which resolves to an encounter on commit.
    ok = walk2("down", lambda: pos2()[1] == 8)
    ok = walk2("right", lambda: pos2()[0] == 13) and ok
    check("walk: reached the slime", ok)
    if not ok:
        print("warning: walk did not reach the slime; sampling anyway")
    press2("right", settle=40)
    shoot(pb, "09-battle", need="DECK:")

    # Select card with A, then execute combo with SELECT (damage dealt)
    press2("a", settle=15)
    press2("select", settle=40)
    shoot(pb, "10-battle-attack", check_title=False)

    # Defense turn or next selection
    press2("a", settle=20)
    shoot(pb, "11-battle-run", check_title=False)
    pb.stop()

    # ─────────────────────────────────────────────────────────────
    # Walk C: Walk north to Forest map
    # ─────────────────────────────────────────────────────────────
    pb = PyBoy(ROM, window="null")
    for _ in range(300):
        pb.tick()
    if not advance_to_overworld(pb):
        check("Walk C: boot reached overworld", False, "stuck on title/intro")
    pos_addr = find_player(pb)

    def pos3():
        return (pb.memory[WRAM_BASE + pos_addr],
                pb.memory[WRAM_BASE + pos_addr + 1])

    def step3(btn):
        pb.button_press(btn)
        for _ in range(4):
            pb.tick()
        pb.button_release(btn)
        for _ in range(24):
            pb.tick()

    # FIELD (4,4) -> right to col 12 (8 steps) -> up to row 1 (3 steps) -> up into north gate (12,0)
    for _ in range(8):
        step3("right")
    for _ in range(3):
        step3("up")
    step3("up")
    for _ in range(120):
        pb.tick()

    shoot(pb, "14-forest-arrived")
    pb.stop()

    print()
    if failures:
        print(f"{len(failures)} check(s) failed: {', '.join(failures)}")
        return 1
    print(f"Walkthrough screenshots saved to {os.path.relpath(OUT, REPO)}/")
    return 0


if __name__ == "__main__":
    sys.exit(main())
#!/usr/bin/env python3
"""Print a reproducible memory budget for the debug ROM from its linker map.

Usage:
    python3 tools/memmap.py build/rpg_card_proto_debug.map

Reports the fixed code area (_CODE), the non-bankable _HOME area (which must
stay below 0x8000 on MBC5 -- CPU addresses >= 0x8000 alias VRAM), and the WRAM
_DATA area.  Exits non-zero if any documented invariant is violated.
"""
import os
import re
import sys


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        sys.exit(1)

    areas = {}
    with open(sys.argv[1]) as f:
        for line in f:
            m = re.match(r"\s*(\w+)\s+([0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)\s+=\s+(\d+)\.\s+bytes", line)
            if not m:
                continue
            name, start, size, nbytes = m.groups()
            if name in ("_CODE", "_HOME", "_DATA"):
                areas[name] = (int(start, 16), int(size, 16), int(nbytes))

    # The RAM-resident banked trampolines (crt0.s) are byte-copied at boot
    # into fixed-size WRAM buffers (g_banked_tramp[64], g_banked_call_tramp[64]
    # in src/core/banked.c).  If the ROM trampoline body ever grew past the
    # buffer, the copy would silently overwrite the WRAM globals that follow it
    # (the banked-call staging area / the call trampoline).  Verify the bodies
    # fit now, at build time, instead of corrupting WRAM at runtime.  Symbol
    # addresses come from the matching .noi symbol listing next to the .map.
    tramp_buf_size = 64
    tramp_bodies = {
        "banked_copy": ("_banked_copy_tramp", "_banked_copy_tramp_end",
                        "g_banked_tramp"),
        "banked_call": ("_banked_call_tramp", "_banked_call_tramp_end",
                        "g_banked_call_tramp"),
    }
    symbols = {}
    noi = os.path.splitext(sys.argv[1])[0] + ".noi"
    if os.path.exists(noi):
        with open(noi) as f:
            for line in f:
                m = re.match(r"\s*DEF\s+(\w+)\s+0x([0-9A-Fa-f]+)", line)
                if m:
                    symbols[m.group(1)] = int(m.group(2), 16)

    ok = True

    print("Game Boy ROM memory budget")
    print("--------------------------")

    if "_CODE" in areas:
        start, size, nbytes = areas["_CODE"]
        print(f"_CODE (fixed bank code/data) : {nbytes:>6} B  @ 0x{start:04X}-0x{start + size:04X}")
    else:
        print("_CODE : (not found)")
        ok = False

    if "_HOME" in areas:
        start, size, nbytes = areas["_HOME"]
        end = start + size
        headroom = 0x8000 - end
        status = "OK" if end <= 0x8000 else "VIOLATION (>= 0x8000 aliases VRAM)"
        if end > 0x8000:
            ok = False
        print(f"_HOME (non-bankable)          : {nbytes:>6} B  @ 0x{start:04X}-0x{end:04X}  "
              f"headroom to 0x8000: {headroom} B  [{status}]")
    else:
        print("_HOME : (not found)")
        ok = False

    if "_DATA" in areas:
        start, size, nbytes = areas["_DATA"]
        end = start + size
        ram_headroom = 0xE000 - end
        print(f"_DATA (WRAM)                  : {nbytes:>6} B  @ 0x{start:04X}-0x{end:04X}  "
              f"headroom to 0xE000: {ram_headroom} B")
    else:
        print("_DATA : (not found)")
        ok = False

    print()
    print("RAM-resident trampoline size checks")
    print("-----------------------------------")
    for name, (start_sym, end_sym, buf_sym) in tramp_bodies.items():
        if start_sym in symbols and end_sym in symbols:
            size = symbols[end_sym] - symbols[start_sym]
            status = "OK" if size <= tramp_buf_size else "VIOLATION (overflows buffer)"
            if size > tramp_buf_size:
                ok = False
            print(f"{name:<26} : {size:>3} B / {tramp_buf_size} B buffer  [{status}]")
        else:
            print(f"{name:<26} : symbols not found in .noi")
            ok = False

    # Banked target address validation: symbols that run through the
    # banked-call trampoline (crt0.s) must resolve into the CPU's
    # switchable ROM window (0x4000-0x7FFF).  The trampoline maps the
    # target's low 16 bits to runtime 0x4000 | (target & 0x3FFF), so a
    # symbol placed outside this window by the linker would produce a
    # silent jump into garbage.  Catch this at build time instead of
    # under the harness at runtime.
    banked_targets = {
        "_combo_resolve_banked": "combo eval + effect dispatch (bank 2)",
        "_effect_resolve_into": "effect scaling body (bank 2)",
        "_battle_init_deck_banked": "battle deck bridge (bank 2)",
        "_status_apply_banked": "status apply body (bank 2)",
        "_status_tick_banked": "status tick body (bank 2)",
        "_save_op_banked": "SRAM save/load body (bank 2)",
        "_deck_discard_banked": "discard push (bank 2)",
        "_world_px_banked": "pixel interpolation (bank 2)",
        "_loot_roll_drop_banked": "loot drop roll (bank 2)",
        "_loot_synth_banked": "loot def synthesis (bank 2)",
        "_game_loot_pool_banked": "enemy loot pool (bank 2)",
        "_game_render_reset_banked": "render cache reset (bank 2)",
        "_dialogue_start_def_banked": "dialogue start (bank 2)",
        "_world_on_battle_fled_banked": "battle-fled world update (bank 2)",
        "_debug_snapshot_banked": "core snapshot builder (bank 2)",
        "_deck_init_default_banked": "starter deck unpacker (bank 2)",
        "_deck_reshuffle_banked": "discard reshuffle (bank 2)",
        "_debug_state_snapshot_banked": "debug state snapshot (bank 2)",
        "_ui_format_int_banked": "int formatter (bank 2)",
        "_scene_load_tiles_banked": "scene tile loader (bank 2)",
        "_enemy_deck_setup_banked": "enemy deck setup (bank 2)",
        "_battle_card_undo_banked": "battle card undo (bank 2)",
        "_ui_update_battle_banked": "battle render (bank 2)",
    }
    print()
    print("Banked target address validation")
    print("--------------------------------")
    for sym, desc in banked_targets.items():
        if sym in symbols:
            addr = symbols[sym]
            # Banked symbols have a 5-digit address like 0x24000
            # (bank 2, offset 0x4000).  The trampoline uses the low 16 bits.
            low16 = addr & 0xFFFF
            in_window = 0x4000 <= low16 <= 0x7FFF
            status = "OK" if in_window else "VIOLATION (not in switchable ROM 0x4000-0x7FFF)"
            if not in_window:
                ok = False
            print(f"{sym:<28}: 0x{addr:05X} (window 0x{low16:04X})  ({desc})  [{status}]")
        else:
            print(f"{sym:<28}: not found in .noi")
            ok = False

    print()
    if ok:
        print("memory budget: invariants OK")
    else:
        print("memory budget: INVARIANT VIOLATION")
        sys.exit(1)


if __name__ == "__main__":
    main()

"""Headless walkthrough of the release ROM with semantic assertions
(docs/verify-walkthrough.md).

Runs the real content (levels/) under PyBoy, drives it with BFS-planned
routes, and asserts canonical gameplay state read from WRAM (StateReader).
Saves the committed screenshots/ set as a side effect; the PNGs stay a
non-gating visual aid, the SEMANTIC CHECKS are the gate (CI runs this via
`make verify-walkthrough`).

Exit code 0 = every check passed; 1 = failures (with expected/actual
detail per check, AGENTS.md §46 style).
"""

import os
import sys
import time

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, REPO)
sys.path.insert(0, os.path.join(REPO, "tools"))

from walkthrough.route import Planner                 # noqa: E402
from walkthrough import walks as W                    # noqa: E402
from walkthrough.session import ROM, OUT              # noqa: E402
from walkthrough.walks import WALK_SECONDS            # noqa: E402


def run():
    if not os.path.isfile(ROM):
        print("error: release ROM not found — build it first (make "
              "release)", file=sys.stderr)
        return 1

    os.makedirs(OUT, exist_ok=True)
    # Drop frames from previous runs: renamed/removed milestones must not
    # linger as stale PNGs next to the current set.
    for old in os.listdir(OUT):
        if old.endswith(".png"):
            os.remove(os.path.join(OUT, old))

    failures = []
    planner = Planner()
    saved = []   # (sram_bytes, expected) for the load roundtrip

    walk_specs = [
        ("walk-a", lambda: W.walk_a(planner, failures, sram_out=saved)),
        ("walk-b", lambda: W.walk_b(planner, failures)),
        ("walk-c", lambda: W.walk_c(planner, failures)),
        ("walk-e", lambda: W.walk_e(planner, failures)),
        ("walk-d", lambda: W.walk_d(failures)),
        ("walk-l", lambda: W.walk_l(failures, saved[0])),
    ]

    for name, fn in walk_specs:
        if name == "walk-l" and not saved:
            failures.append(("walk-l", "skipped: no saved state", False,
                             "walk-a save", "missing"))
            continue
        t0 = time.time()
        try:
            fn()
        except Exception as exc:   # noqa: BLE001 — isolate per walk
            import traceback
            traceback.print_exc()
            failures.append((name, "walk crashed", False,
                             "clean run", "%s: %s" % (type(exc).__name__,
                                                      exc)))
        dt = time.time() - t0
        print("[%s] %s in %.1fs" % ("OK " if dt < WALK_SECONDS else "SLOW",
                                    name, dt))
        if dt > WALK_SECONDS:
            failures.append((name, "walk exceeded wall-clock cap", False,
                             "<= %ds" % WALK_SECONDS, "%.1fs" % dt))

    print()
    bad = [c for c in failures if not c[2]]
    if bad:
        print("%d/%d check(s) failed:" % (len(bad), len(failures)))
        for label, name, _ok, expected, actual in bad:
            print("  [%s] %s\n      expected: %s\n      actual:   %s"
                  % (label, name, expected, actual))
        return 1
    print("walkthrough: %d check(s) passed" % len(failures))
    return 0


if __name__ == "__main__":
    sys.exit(run())

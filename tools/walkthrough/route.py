"""BFS route planner over the real working content (levels/) — docs/
verify-walkthrough.md §3 Phase 3.

Reuses the level compiler's own collision derivation (derive_collision +
load_tilesets) so the walkthrough and the ROM agree on walkability with
zero duplicated logic.  Routes adapt automatically to content edits: actor
positions, exits, and patrol boxes are read from levels/*.json at runtime
(no hardcoded coordinates).

Multi-map graph: walkable cells per scene, portal edges from the exits
(entering an exit tile transitions to the target scene).
"""

import json
import os
import sys
from collections import deque

from walkthrough.session import LEVELS_DIR, REPO

# Import the level compiler's derivation (single source of truth).
sys.path.insert(0, os.path.join(REPO, "tools", "level_compiler"))
from validate import load_tilesets          # noqa: E402
from compile import derive_collision, SCENE_ORDER   # noqa: E402

# Patrol boxes (src/world/actor.h): blocked cells for routing — the
# walkthrough must not steer through a hostile's patrol path.
AI_NONE = 0
AI_PATROL_CIRCLE = 1
AI_PATROL_CROSS = 2
AI_CHASE = 3              # steps toward the player every AI tick
AI_PATROL_VERT = 4
AI_PATROL_VERT_TILES = 3

# levels/*.json carries the enum NAMES as strings.
AI_NAMES = {
    "AI_NONE": AI_NONE,
    "AI_PATROL_CIRCLE": AI_PATROL_CIRCLE,
    "AI_PATROL_CROSS": AI_PATROL_CROSS,
    "AI_CHASE": AI_CHASE,
    "AI_PATROL_VERT": AI_PATROL_VERT,
}


def patrol_cells(ai, x, y):
    """Cells the hostile may occupy (conservative inflation)."""
    if ai == AI_PATROL_VERT:
        return {(x, dy) for dy in range(y - AI_PATROL_VERT_TILES,
                                        y + AI_PATROL_VERT_TILES + 1)}
    if ai == AI_PATROL_CROSS:
        return {(x, y), (x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)}
    if ai == AI_PATROL_CIRCLE:
        return {(dx, dy) for dx in (x - 1, x) for dy in (y - 1, y)}
    if ai == AI_CHASE:
        # Chasers close on the player from anywhere: block a generous
        # halo so routes keep their distance.
        return {(dx, dy) for dx in range(x - 2, x + 3)
                for dy in range(y - 2, y + 3)}
    return {(x, y)}


class Scene:
    def __init__(self, name, scene_id, level, tileset):
        self.name = name
        self.scene_id = scene_id
        self.level = level
        self.width = level["map"]["width"]
        self.height = level["map"]["height"]
        self.grid = derive_collision(level, tileset)
        self.exits = level.get("exits", [])
        # Exit tiles sit on the map perimeter, which derive_collision
        # marks as wall — but the ROM triggers them when the player
        # moves into them, so they are portal nodes for routing.
        self.exit_cells = {(e["x"], e["y"]) for e in self.exits}
        self.hostiles = []      # (x, y, ai)
        self.blocked_actors = set()
        for obj in level.get("objects", []):
            props = obj.get("properties", {}) or {}
            flags = props.get("flags", []) or []
            x = obj["position"]["x"]
            y = obj["position"]["y"]
            if "HOSTILE" in flags:
                ai_name = props.get("ai", "AI_NONE")
                self.hostiles.append((x, y, AI_NAMES.get(ai_name, AI_NONE)))
            elif "BLOCKING" in flags:
                self.blocked_actors.add((x, y))

    def walkable(self, x, y, avoid=None):
        if not (0 <= x < self.width and 0 <= y < self.height):
            return False
        if (x, y) in self.exit_cells:
            return True
        if not self.grid[y][x]:
            return False
        if avoid and (x, y) in avoid:
            return False
        return True

    def patrol_blocked(self):
        """Union of all hostiles' patrol cells."""
        cells = set()
        for x, y, ai in self.hostiles:
            cells |= patrol_cells(ai, x, y)
        return cells


class Planner:
    def __init__(self, levels_dir=LEVELS_DIR):
        """Mirror the compiler's content registry: every levels/*.json
        becomes a scene; ids are assigned SCENE_ORDER first, then the
        remaining files alphabetically — exactly the order compile.py
        appends unknown levels to its table, so planner scene ids match
        the ROM's compiled ids.  A NEW level added by the editor gets a
        scene id automatically and is swept by walk_sweep."""
        self.scenes = {}
        tilesets = load_tilesets()
        names = sorted(os.path.splitext(f)[0]
                       for f in os.listdir(levels_dir)
                       if f.endswith(".json"))
        ordered = [n for n in SCENE_ORDER if n in names]
        ordered += [n for n in names if n not in SCENE_ORDER]
        for scene_id, name in enumerate(ordered):
            level = json.load(open(os.path.join(levels_dir,
                                                name + ".json")))
            ts = tilesets.get(level["map"]["tileset"], {})
            self.scenes[name] = Scene(name, scene_id, level, ts)

    def arrival_pos(self, name):
        """The tile the player lands on when entering scene `name`
        (any parent exit's target).  None if no exit leads there."""
        for scene in self.scenes.values():
            for e in scene.exits:
                if e["target_scene"] == name:
                    return (e["target_x"], e["target_y"])
        return None

    def scene_of(self, scene_id):
        for s in self.scenes.values():
            if s.scene_id == scene_id:
                return s
        raise KeyError("no scene with id %d" % scene_id)

    # ── BFS ──────────────────────────────────────────────────────────
    def _bfs(self, scene, start, goal, avoid=None):
        """Shortest walkable path start->goal as a list of (dx, dy)
        moves.  Exit tiles are ordinary cells here; the caller decides
        whether the goal is an exit (portal crossing)."""
        if start == goal:
            return []
        prev = {start: None}
        q = deque([start])
        while q:
            cur = q.popleft()
            for dx, dy in ((0, -1), (0, 1), (-1, 0), (1, 0)):
                nxt = (cur[0] + dx, cur[1] + dy)
                if nxt in prev:
                    continue
                if not scene.walkable(nxt[0], nxt[1], avoid):
                    continue
                prev[nxt] = cur
                if nxt == goal:
                    path = []
                    node = nxt
                    while prev[node] is not None:
                        p = prev[node]
                        path.append((node[0] - p[0], node[1] - p[1]))
                        node = p
                    return list(reversed(path))
                q.append(nxt)
        return None

    def path(self, scene_name, start, goal):
        """Path within one scene.  `avoid` = patrol cells + blocking
        actors, EXCEPT cells that are the start/goal themselves (the
        start may sit inside a patrol box right after boot, and the
        goal may be a bump-adjacent cell next to a blocking actor)."""
        scene = self.scenes[scene_name]
        avoid = scene.patrol_blocked() | scene.blocked_actors
        avoid.discard(start)
        avoid.discard(goal)
        return self._bfs(scene, start, goal, avoid)

    def path_to_exit(self, scene_name, start, target_scene_name):
        """Path to the exit tile whose target_scene is target_scene_name.
        Returns (path_to_exit_cell, exit)."""
        scene = self.scenes[scene_name]
        for e in scene.exits:
            goal = (e["x"], e["y"])
            path = self.path(scene_name, start, goal)
            if path is not None and e.get("target_scene") == target_scene_name:
                return path, e
        return None, None

    def _scene_next(self, from_name, to_name):
        """Next hop on the shortest scene-graph path from -> to (BFS over
        exit adjacency); None if unreachable."""
        prev = {from_name: None}
        q = deque([from_name])
        while q:
            cur = q.popleft()
            for e in self.scenes[cur].exits:
                nxt = e["target_scene"]
                if nxt in prev:
                    continue
                prev[nxt] = cur
                if nxt == to_name:
                    node = nxt
                    while prev[node] != from_name:
                        node = prev[node]
                    return node
                q.append(nxt)
        return None

    def route(self, from_scene, start, to_scene, goal):
        """Cross-scene route: list of steps.
        ("move", dx, dy) | ("exit", direction, exit)."""
        steps = []
        scene_name = from_scene
        cur = start
        guard = 0
        while scene_name != to_scene:
            guard += 1
            if guard > 8:
                raise ValueError("route: too many transitions "
                                 "(%s -> %s)" % (from_scene, to_scene))
            next_scene = self._scene_next(scene_name, to_scene)
            if next_scene is None:
                raise ValueError("route: %s cannot reach %s"
                                 % (from_scene, to_scene))
            found = self.path_to_exit(scene_name, cur, next_scene)
            if not found or found[0] is None:
                raise ValueError("route: no exit path %s:%s -> %s"
                                 % (scene_name, cur, next_scene))
            path, e = found
            # The BFS path's final move enters the exit tile — that move
            # IS the portal crossing, so convert it to an exit step
            # (press toward the exit, ride the wipe).
            moves = path[:-1]
            ldx, ldy = path[-1]
            steps += [("move", dx, dy) for dx, dy in moves]
            steps.append(("exit",
                          {(0, -1): "up", (0, 1): "down",
                           (-1, 0): "left", (1, 0): "right"}[(ldx, ldy)],
                          e))
            cur = (e["target_x"], e["target_y"])
            scene_name = to_scene if e.get("target_scene") == to_scene \
                else e["target_scene"]
        path = self.path(scene_name, cur, goal)
        if path is None:
            raise ValueError("route: no path %s:%s -> %s"
                             % (scene_name, cur, goal))
        steps += [("move", dx, dy) for dx, dy in path]
        return steps

    # ── encounter helpers ────────────────────────────────────────────
    def edge_of(self, scene_name, start, hostile_xy):
        """Nearest walkable cell adjacent to a hostile's patrol box (the
        sweep start for an engagement walk)."""
        scene = self.scenes[scene_name]
        hx, hy = hostile_xy
        best, best_cost = None, None
        for dy in range(-AI_PATROL_VERT_TILES - 1,
                        AI_PATROL_VERT_TILES + 2):
            for dx in (-2, -1, 0, 1, 2):
                cand = (hx + dx, hy + dy)
                path = self.path(scene_name, start, cand)
                if path is not None:
                    cost = len(path)
                    if best_cost is None or cost < best_cost:
                        best, best_cost = cand, cost
        return best

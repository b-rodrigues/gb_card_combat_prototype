import React, { useState, useEffect, useCallback } from 'react';
import './App.css';
import { EditorLevel, LevelExit, LevelRegion, createEmptyEditorLevel, levelDataToEditor } from './model/Level';
import { LevelObject } from './model/Objects';
import { Toolbar, ToolType } from './Toolbar';
import { LayerPanel, EditLayer } from './LayerPanel';
import { TilesetPalette } from './TilesetPalette';
import { Inspector } from './Inspector';
import { MapCanvas } from './MapCanvas';
import { downloadLevelJson } from './io/saveLevel';
import { promptLoadLevelFile } from './io/loadLevel';
import { BUILTIN_TILESETS } from './model/Tileset';

// Built-in levels from repository
import forestData from '../../../levels/forest.json';
import fieldData from '../../../levels/field.json';
import southFieldData from '../../../levels/south_field.json';
import townData from '../../../levels/town.json';
import mountainPassData from '../../../levels/mountain_pass.json';
import castleData from '../../../levels/castle.json';
import titleData from '../../../screens/title.json';
import battleData from '../../../screens/battle.json';

interface ExistingLevelItem {
  id: string;
  name: string;
  data: any;
  category?: 'levels' | 'screens';
}

const EXISTING_LEVELS: ExistingLevelItem[] = [
  { id: 'forest', name: forestData.name || 'Forest', data: forestData, category: 'levels' },
  { id: 'field', name: fieldData.name || 'Field', data: fieldData, category: 'levels' },
  { id: 'south_field', name: southFieldData.name || 'Desolate South Field', data: southFieldData, category: 'levels' },
  { id: 'town', name: townData.name || 'Town', data: townData, category: 'levels' },
  { id: 'mountain_pass', name: mountainPassData.name || 'Mountain Pass', data: mountainPassData, category: 'levels' },
  { id: 'castle', name: castleData.name || 'Castle', data: castleData, category: 'levels' },
  { id: 'title', name: 'Title Screen', data: titleData, category: 'screens' },
  { id: 'battle', name: 'Battle Screen', data: battleData, category: 'screens' },
];

export const App: React.FC = () => {
  const [level, setLevel] = useState<EditorLevel>(() => levelDataToEditor(forestData as any));
  const [currentLevelId, setCurrentLevelId] = useState<string>('forest');

  // Tool & Layer State
  const [activeTool, setActiveTool] = useState<ToolType>('brush');
  const [activeLayer, setActiveLayer] = useState<EditLayer>('terrain');
  const [selectedTileId, setSelectedTileId] = useState<string>('tree');
  const [selectedEntityIndex, setSelectedEntityIndex] = useState<number | null>(null);

  // View Options
  const [zoom, setZoom] = useState<number>(2);
  const [showGrid, setShowGrid] = useState<boolean>(true);
  const [showCollision, setShowCollision] = useState<boolean>(false);
  const [showTerrain, setShowTerrain] = useState<boolean>(true);
  const [showExits, setShowExits] = useState<boolean>(true);
  const [showObjects, setShowObjects] = useState<boolean>(true);
  const [showRegions, setShowRegions] = useState<boolean>(true);

  // Undo / Redo Stacks
  const [history, setHistory] = useState<EditorLevel[]>([]);
  const [redoStack, setRedoStack] = useState<EditorLevel[]>([]);

  // Modals
  const [showValidateModal, setShowValidateModal] = useState<boolean>(false);
  const [showDescribeModal, setShowDescribeModal] = useState<boolean>(false);
  const [describeFormat, setDescribeFormat] = useState<'markdown' | 'json'>('markdown');

  // Push state to undo history
  const pushState = useCallback((newLevel: EditorLevel) => {
    setHistory((prev) => [...prev.slice(-30), JSON.parse(JSON.stringify(level))]);
    setRedoStack([]);
    setLevel(newLevel);
  }, [level]);

  // Undo / Redo
  const handleUndo = () => {
    if (history.length === 0) return;
    const previous = history[history.length - 1];
    setHistory((prev) => prev.slice(0, -1));
    setRedoStack((prev) => [JSON.parse(JSON.stringify(level)), ...prev]);
    setLevel(previous);
  };

  const handleRedo = () => {
    if (redoStack.length === 0) return;
    const next = redoStack[0];
    setRedoStack((prev) => prev.slice(1));
    setHistory((prev) => [...prev, JSON.parse(JSON.stringify(level))]);
    setLevel(next);
  };

  // Keyboard Shortcuts
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.target instanceof HTMLInputElement || e.target instanceof HTMLTextAreaElement || e.target instanceof HTMLSelectElement) {
        return;
      }

      if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'z') {
        e.preventDefault();
        if (e.shiftKey) {
          handleRedo();
        } else {
          handleUndo();
        }
      } else if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'y') {
        e.preventDefault();
        handleRedo();
      } else if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 's') {
        e.preventDefault();
        downloadLevelJson(level);
      } else if (e.key.toLowerCase() === 'b') {
        setActiveTool('brush');
      } else if (e.key.toLowerCase() === 'e') {
        setActiveTool('eraser');
      } else if (e.key.toLowerCase() === 'r') {
        setActiveTool('rect');
      } else if (e.key.toLowerCase() === 'g') {
        setActiveTool('fill');
      } else if (e.key.toLowerCase() === 'i') {
        setActiveTool('eyedropper');
      } else if (e.key.toLowerCase() === 'v') {
        setActiveTool('select');
      }
    };

    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [level, history, redoStack]);

  // Select level from dropdown
  const handleSelectLevel = (selectedId: string) => {
    if (selectedId === '__new__') {
      handleCreateNewLevel();
      return;
    }

    const found = EXISTING_LEVELS.find((l) => l.id === selectedId);
    if (found) {
      const parsed = levelDataToEditor(found.data);
      pushState(parsed);
      setCurrentLevelId(selectedId);
      setSelectedEntityIndex(null);
    }
  };

  // Create new level
  const handleCreateNewLevel = () => {
    const id = prompt('Enter new level ID (e.g. desert_ruins):', 'new_level');
    if (!id) return;
    const cleanId = id.trim().toLowerCase().replace(/[^a-z0-9_]/g, '_') || 'new_level';
    const empty = createEmptyEditorLevel(cleanId, 'forest', 20, 18);
    empty.name = cleanId.replace(/_/g, ' ').replace(/\b\w/g, (c) => c.toUpperCase());
    pushState(empty);
    setCurrentLevelId(cleanId);
    setSelectedEntityIndex(null);
  };

  // Drawing Actions
  const handleTilePainted = (x: number, y: number, tileId: string) => {
    if (level.grid[y]?.[x] === tileId) return;
    const newGrid = level.grid.map((row, ry) =>
      ry === y ? row.map((col, rx) => (rx === x ? tileId : col)) : [...row]
    );
    pushState({ ...level, grid: newGrid });
  };

  const handleRectPainted = (rx: number, ry: number, rw: number, rh: number, tileId: string) => {
    const newGrid = level.grid.map((row, y) =>
      row.map((col, x) => (x >= rx && x < rx + rw && y >= ry && y < ry + rh ? tileId : col))
    );
    pushState({ ...level, grid: newGrid });
  };

  const handleFill = (startX: number, startY: number, newTileId: string) => {
    const targetTileId = level.grid[startY]?.[startX];
    if (!targetTileId || targetTileId === newTileId) return;

    const newGrid = level.grid.map((row) => [...row]);
    const queue: Array<[number, number]> = [[startX, startY]];
    const visited = new Set<string>();

    while (queue.length > 0) {
      const [cx, cy] = queue.pop()!;
      const key = `${cx},${cy}`;
      if (visited.has(key)) continue;
      visited.add(key);

      if (cx < 0 || cx >= level.width || cy < 0 || cy >= level.height) continue;
      if (newGrid[cy][cx] !== targetTileId) continue;

      newGrid[cy][cx] = newTileId;

      queue.push([cx + 1, cy]);
      queue.push([cx - 1, cy]);
      queue.push([cx, cy + 1]);
      queue.push([cx, cy - 1]);
    }

    pushState({ ...level, grid: newGrid });
  };

  // Spawn Move
  const handleSpawnMoved = (x: number, y: number) => {
    pushState({ ...level, spawn: { ...level.spawn, x, y } });
  };

  // Object Operations
  const handleObjectMoved = (index: number, x: number, y: number) => {
    const newObjs = [...level.objects];
    if (newObjs[index]) {
      newObjs[index] = { ...newObjs[index], position: { x, y } };
      pushState({ ...level, objects: newObjs });
    }
  };

  const handleAddObject = (obj: LevelObject) => {
    pushState({ ...level, objects: [...level.objects, obj] });
  };

  const handleUpdateObject = (index: number, obj: LevelObject) => {
    const newObjs = [...level.objects];
    newObjs[index] = obj;
    pushState({ ...level, objects: newObjs });
  };

  const handleDeleteObject = (index: number) => {
    const newObjs = level.objects.filter((_, i) => i !== index);
    pushState({ ...level, objects: newObjs });
    setSelectedEntityIndex(null);
  };

  // Exit Operations
  const handleExitMoved = (index: number, x: number, y: number) => {
    const newExits = [...level.exits];
    if (newExits[index]) {
      newExits[index] = { ...newExits[index], x, y };
      pushState({ ...level, exits: newExits });
    }
  };

  const handleAddExit = (exit: LevelExit) => {
    pushState({ ...level, exits: [...level.exits, exit] });
  };

  const handleUpdateExit = (index: number, exit: LevelExit) => {
    const newExits = [...level.exits];
    newExits[index] = exit;
    pushState({ ...level, exits: newExits });
  };

  const handleDeleteExit = (index: number) => {
    const newExits = level.exits.filter((_, i) => i !== index);
    pushState({ ...level, exits: newExits });
    setSelectedEntityIndex(null);
  };

  // Region Operations
  const handleAddRegion = (region: LevelRegion) => {
    pushState({ ...level, regions: [...level.regions, region] });
  };

  const handleUpdateRegion = (index: number, region: LevelRegion) => {
    const newRegions = [...level.regions];
    newRegions[index] = region;
    pushState({ ...level, regions: newRegions });
  };

  const handleDeleteRegion = (index: number) => {
    const newRegions = level.regions.filter((_, i) => i !== index);
    pushState({ ...level, regions: newRegions });
    setSelectedEntityIndex(null);
  };

  // Level Meta Updates
  const handleUpdateLevelMeta = (updates: Partial<EditorLevel>) => {
    let newGrid = level.grid;
    const newW = updates.width !== undefined ? updates.width : level.width;
    const newH = updates.height !== undefined ? updates.height : level.height;

    if (newW !== level.width || newH !== level.height) {
      newGrid = [];
      for (let y = 0; y < newH; y++) {
        const row: string[] = [];
        for (let x = 0; x < newW; x++) {
          row.push(level.grid[y]?.[x] || 'floor');
        }
        newGrid.push(row);
      }
    }

    pushState({ ...level, ...updates, grid: newGrid });
  };

  // Validate Logic
  const runValidation = () => {
    const passed: string[] = [];
    const errors: string[] = [];
    const warnings: string[] = [];

    // Dimensions
    if (level.width >= 4 && level.width <= 40 && level.height >= 4 && level.height <= 24) {
      passed.push('Schema valid (Dimensions <= 40x24)');
    } else {
      errors.push(`Dimensions ${level.width}x${level.height} out of engine bounds (max 40x24)`);
    }

    // Tileset
    const ts = BUILTIN_TILESETS[level.tileset];
    if (ts) {
      passed.push(`Tiles valid (Tileset '${level.tileset}')`);
    } else {
      errors.push(`Unknown tileset '${level.tileset}'`);
    }

    // Spawn
    if (level.spawn.x >= 0 && level.spawn.x < level.width && level.spawn.y >= 0 && level.spawn.y < level.height) {
      const spawnTile = level.grid[level.spawn.y]?.[level.spawn.x];
      const tDef = ts?.tiles.find((t) => t.id === spawnTile);
      if (tDef && !tDef.walkable) {
        errors.push(`Player spawn at (${level.spawn.x}, ${level.spawn.y}) is blocked by solid tile '${spawnTile}'`);
      } else {
        passed.push('Player spawn valid & walkable');
        passed.push('Collision valid');
      }
    } else {
      errors.push(`Player spawn (${level.spawn.x}, ${level.spawn.y}) is outside map bounds`);
    }

    // Exits
    let exitsOk = true;
    level.exits.forEach((ex, i) => {
      if (ex.x < 0 || ex.x >= level.width || ex.y < 0 || ex.y >= level.height) {
        errors.push(`Exit #${i + 1} position (${ex.x}, ${ex.y}) out of map bounds`);
        exitsOk = false;
      }
    });
    if (exitsOk) passed.push('Exits valid');

    // Objects
    let objectsOk = true;
    const seenIds = new Set<string>();
    level.objects.forEach((obj, i) => {
      if (!obj.id) {
        errors.push(`Object #${i + 1} has empty ID`);
        objectsOk = false;
      } else if (seenIds.has(obj.id)) {
        errors.push(`Duplicate object ID: '${obj.id}'`);
        objectsOk = false;
      } else {
        seenIds.add(obj.id);
      }
    });
    if (objectsOk) passed.push('Objects valid');

    return { passed, errors, warnings, isValid: errors.length === 0 };
  };

  const validationResult = runValidation();

  // Generate LLM Description
  const generateLLMDescription = () => {
    if (describeFormat === 'json') {
      const jsonDesc = {
        level: level.id,
        name: level.name,
        size: `${level.width}x${level.height}`,
        tileset: level.tileset,
        music: level.music,
        player_start: [level.spawn.x, level.spawn.y],
        connections: level.exits.map(
          (e) => `${e.direction?.toLowerCase() || 'exit'} -> ${e.target_scene} (spawn ${e.target_x},${e.target_y})`
        ),
        regions: level.regions.map((r) => ({
          name: r.id,
          bounds: [r.bounds.x, r.bounds.y, r.bounds.width, r.bounds.height],
          purpose: r.gameplay?.purpose || 'exploration',
          description: r.description || '',
        })),
        objects: level.objects.map((o) => ({
          id: o.id,
          type: o.type,
          position: [o.position.x, o.position.y],
          properties: o.properties,
        })),
      };
      return JSON.stringify(jsonDesc, null, 2);
    }

    const lines: string[] = [];
    lines.push(`# ${level.name.toUpperCase()} (\`${level.id}\`)`);
    lines.push(`\n**Size:** ${level.width}×${level.height} tiles | **Tileset:** \`${level.tileset}\` | **Music:** \`${level.music}\``);
    lines.push(`**Player Start:** \`(${level.spawn.x}, ${level.spawn.y})\` facing ${level.spawn.facing || 'DOWN'}`);

    lines.push('\n## Connections');
    if (level.exits.length > 0) {
      level.exits.forEach((e) => {
        lines.push(`- ${e.direction || 'Exit'} (at ${e.x},${e.y}) → **${e.target_scene}** (spawn at ${e.target_x},${e.target_y})`);
      });
    } else {
      lines.push('- None');
    }

    lines.push('\n## Regions & Semantic Context');
    if (level.regions.length > 0) {
      level.regions.forEach((r) => {
        lines.push(`### ${r.id.replace(/_/g, ' ').toUpperCase()}`);
        lines.push(`- **Bounds:** [${r.bounds.x}, ${r.bounds.y}] to [${r.bounds.x + r.bounds.width}, ${r.bounds.y + r.bounds.height}]`);
        lines.push(`- **Purpose:** \`${r.gameplay?.purpose || 'exploration'}\` (Difficulty: ${r.gameplay?.difficulty || 1})`);
        if (r.description) lines.push(`- **Description:** ${r.description}`);
      });
    } else {
      lines.push('- No explicit regions defined.');
    }

    lines.push('\n## Important Objects & Entities');
    if (level.objects.length > 0) {
      level.objects.forEach((o) => {
        lines.push(`- **[${o.type.toUpperCase()}]** \`${o.id}\` at (${o.position.x}, ${o.position.y})`);
      });
    } else {
      lines.push('- None');
    }

    return lines.join('\n');
  };

  return (
    <div className="app-container">
      {/* Top Header */}
      <header className="app-header">
        <div className="header-title">
          <span className="brand-icon">🎮</span>
          <span>Game Boy RPG Level Editor</span>
        </div>
        <div className="header-levels">
          <label htmlFor="level-select-dropdown" className="header-levels-label">
            Level:
          </label>
          <select
            id="level-select-dropdown"
            className="level-select"
            value={currentLevelId}
            onChange={(e) => handleSelectLevel(e.target.value)}
          >
            <optgroup label="Overworld Levels">
              {EXISTING_LEVELS.filter((l) => l.category === 'levels').map((lvl) => (
                <option key={lvl.id} value={lvl.id}>
                  {lvl.name} ({lvl.id}.json)
                </option>
              ))}
            </optgroup>
            <optgroup label="Screens">
              {EXISTING_LEVELS.filter((l) => l.category === 'screens').map((lvl) => (
                <option key={lvl.id} value={lvl.id}>
                  {lvl.name} (screens/{lvl.id}.json)
                </option>
              ))}
            </optgroup>
            {!EXISTING_LEVELS.some((l) => l.id === currentLevelId) && (
              <optgroup label="Current Level">
                <option value={currentLevelId}>
                  {level.name || currentLevelId} ({currentLevelId}.json)
                </option>
              </optgroup>
            )}
            <optgroup label="Actions">
              <option value="__new__">➕ + New Level...</option>
            </optgroup>
          </select>
        </div>
      </header>

      {/* Main App Body */}
      <div className="app-body">
        <Toolbar
          activeTool={activeTool}
          onSelectTool={setActiveTool}
          canUndo={history.length > 0}
          canRedo={redoStack.length > 0}
          onUndo={handleUndo}
          onRedo={handleRedo}
          zoom={zoom}
          onZoomChange={setZoom}
          showGrid={showGrid}
          onToggleGrid={() => setShowGrid(!showGrid)}
          showCollision={showCollision}
          onToggleCollision={() => setShowCollision(!showCollision)}
          onSave={() => downloadLevelJson(level)}
          onLoad={async () => {
            try {
              const res = await promptLoadLevelFile();
              pushState(res.level);
              setCurrentLevelId(res.level.id);
            } catch (err) {
              alert(`Failed to load level: ${err}`);
            }
          }}
          onValidate={() => setShowValidateModal(true)}
          onDescribe={() => setShowDescribeModal(true)}
          onNew={handleCreateNewLevel}
        />

        <div className="main-content">
          {/* Left Sidebar */}
          <aside className="sidebar-left">
            <TilesetPalette
              tilesetId={level.tileset}
              selectedTileId={selectedTileId}
              onSelectTileset={(ts) => pushState({ ...level, tileset: ts })}
              onSelectTile={setSelectedTileId}
            />
            <LayerPanel
              activeLayer={activeLayer}
              onSelectLayer={setActiveLayer}
              showTerrain={showTerrain}
              onToggleShowTerrain={() => setShowTerrain(!showTerrain)}
              showExits={showExits}
              onToggleShowExits={() => setShowExits(!showExits)}
              showObjects={showObjects}
              onToggleShowObjects={() => setShowObjects(!showObjects)}
              showRegions={showRegions}
              onToggleShowRegions={() => setShowRegions(!showRegions)}
              exitCount={level.exits.length}
              objectCount={level.objects.length}
              regionCount={level.regions.length}
            />
          </aside>

          {/* Canvas Work Area */}
          <main className="center-stage">
            <MapCanvas
              level={level}
              activeTool={activeTool}
              activeLayer={activeLayer}
              selectedTileId={selectedTileId}
              zoom={zoom}
              showGrid={showGrid}
              showCollision={showCollision}
              showTerrain={showTerrain}
              showExits={showExits}
              showObjects={showObjects}
              showRegions={showRegions}
              selectedEntityIndex={selectedEntityIndex}
              onSelectEntityIndex={setSelectedEntityIndex}
              onTilePainted={handleTilePainted}
              onRectPainted={handleRectPainted}
              onFill={handleFill}
              onTilePicked={(picked) => {
                setSelectedTileId(picked);
                setActiveTool('brush');
              }}
              onSpawnMoved={handleSpawnMoved}
              onObjectMoved={handleObjectMoved}
              onExitMoved={handleExitMoved}
              onAddObjectAt={(x, y) => {
                handleAddObject({
                  id: `obj_${Date.now().toString().slice(-4)}`,
                  type: 'npc',
                  position: { x, y },
                  properties: { display_name: 'NPC' },
                });
              }}
              onAddExitAt={(x, y) => {
                handleAddExit({
                  x,
                  y,
                  target_scene: 'field',
                  target_x: 2,
                  target_y: 7,
                  direction: 'SOUTH',
                  tile_char: '<',
                });
              }}
            />
          </main>

          {/* Right Inspector */}
          <aside className="sidebar-right">
            <Inspector
              level={level}
              activeLayer={activeLayer}
              selectedEntityIndex={selectedEntityIndex}
              onSelectEntityIndex={setSelectedEntityIndex}
              onUpdateLevelMeta={handleUpdateLevelMeta}
              onUpdateSpawn={(spawn) => pushState({ ...level, spawn })}
              onAddExit={handleAddExit}
              onUpdateExit={handleUpdateExit}
              onDeleteExit={handleDeleteExit}
              onAddObject={handleAddObject}
              onUpdateObject={handleUpdateObject}
              onDeleteObject={handleDeleteObject}
              onAddRegion={handleAddRegion}
              onUpdateRegion={handleUpdateRegion}
              onDeleteRegion={handleDeleteRegion}
            />
          </aside>
        </div>
      </div>

      {/* Validation Modal */}
      {showValidateModal && (
        <div className="modal-overlay" onClick={() => setShowValidateModal(false)}>
          <div className="modal-card" onClick={(e) => e.stopPropagation()}>
            <div className="modal-header">
              <h3>Level Validation Results</h3>
              <button className="modal-close-btn" onClick={() => setShowValidateModal(false)}>
                ✕
              </button>
            </div>
            <div className="modal-body">
              <div className="validation-list">
                {validationResult.passed.map((p, i) => (
                  <div key={i} className="val-item passed">
                    ✓ {p}
                  </div>
                ))}
                {validationResult.warnings.map((w, i) => (
                  <div key={i} className="val-item warning">
                    ⚠️ {w}
                  </div>
                ))}
                {validationResult.errors.map((e, i) => (
                  <div key={i} className="val-item failed">
                    ✗ {e}
                  </div>
                ))}
              </div>
              <div style={{ marginTop: '16px', fontWeight: 'bold', color: validationResult.isValid ? '#56d364' : '#f85149' }}>
                {validationResult.isValid ? '🎉 LEVEL VALID' : '❌ LEVEL INVALID (Fix errors above)'}
              </div>
            </div>
            <div className="modal-footer">
              <button className="btn btn-primary" onClick={() => setShowValidateModal(false)}>
                Close
              </button>
            </div>
          </div>
        </div>
      )}

      {/* LLM Description Modal */}
      {showDescribeModal && (
        <div className="modal-overlay" onClick={() => setShowDescribeModal(false)}>
          <div className="modal-card" onClick={(e) => e.stopPropagation()}>
            <div className="modal-header">
              <h3>LLM Semantic Representation</h3>
              <div style={{ display: 'flex', gap: '8px', alignItems: 'center' }}>
                <button
                  className={`btn btn-sm ${describeFormat === 'markdown' ? 'btn-primary' : ''}`}
                  onClick={() => setDescribeFormat('markdown')}
                >
                  Markdown
                </button>
                <button
                  className={`btn btn-sm ${describeFormat === 'json' ? 'btn-primary' : ''}`}
                  onClick={() => setDescribeFormat('json')}
                >
                  JSON
                </button>
                <button className="modal-close-btn" onClick={() => setShowDescribeModal(false)}>
                  ✕
                </button>
              </div>
            </div>
            <div className="modal-body">
              <pre className="code-block">{generateLLMDescription()}</pre>
            </div>
            <div className="modal-footer">
              <button
                className="btn"
                onClick={() => {
                  navigator.clipboard.writeText(generateLLMDescription());
                  alert('Copied to clipboard!');
                }}
              >
                📋 Copy
              </button>
              <button className="btn btn-primary" onClick={() => setShowDescribeModal(false)}>
                Close
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
};

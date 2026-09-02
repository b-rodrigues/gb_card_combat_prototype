import React, { useState, useEffect } from 'react';
import { EditorLevel, LevelExit, LevelRegion, PlayerSpawn } from './model/Level';
import { LevelObject, OBJECT_TEMPLATES } from './model/Objects';
import { EditLayer, LayerPanel } from './LayerPanel';
import { BUILTIN_TILESETS, TileDefinition } from './model/Tileset';

const BOSS_9X9_TEMPLATES: Record<string, { name: string; tiles: string[][] }> = {
  giausar: {
    name: 'LORD GIAUSAR',
    tiles: [
      ['combat.boss_horns_left', 'combat.boss_horns_left', 'combat.boss_horns_left', 'combat.boss_horns_mid', 'combat.boss_horns_mid', 'combat.boss_horns_mid', 'combat.boss_horns_right', 'combat.boss_horns_right', 'combat.boss_horns_right'],
      ['combat.boss_horns_left', 'combat.boss_horns_left', 'combat.boss_horns_left', 'combat.boss_horns_mid', 'combat.boss_horns_mid', 'combat.boss_horns_mid', 'combat.boss_horns_right', 'combat.boss_horns_right', 'combat.boss_horns_right'],
      ['combat.boss_horns_left', 'combat.boss_horns_left', 'combat.boss_horns_left', 'combat.boss_horns_mid', 'combat.boss_horns_mid', 'combat.boss_horns_mid', 'combat.boss_horns_right', 'combat.boss_horns_right', 'combat.boss_horns_right'],
      ['combat.boss_head_left', 'combat.boss_head_left', 'combat.boss_head_left', 'combat.boss_head_mid', 'combat.boss_head_mid', 'combat.boss_head_mid', 'combat.boss_head_right', 'combat.boss_head_right', 'combat.boss_head_right'],
      ['combat.boss_head_left', 'combat.boss_head_left', 'combat.boss_head_left', 'combat.boss_head_mid', 'combat.boss_head_mid', 'combat.boss_head_mid', 'combat.boss_head_right', 'combat.boss_head_right', 'combat.boss_head_right'],
      ['combat.boss_head_left', 'combat.boss_head_left', 'combat.boss_head_left', 'combat.boss_head_mid', 'combat.boss_head_mid', 'combat.boss_head_mid', 'combat.boss_head_right', 'combat.boss_head_right', 'combat.boss_head_right'],
      ['combat.boss_torso_left', 'combat.boss_torso_left', 'combat.boss_torso_left', 'combat.boss_torso_mid', 'combat.boss_torso_mid', 'combat.boss_torso_mid', 'combat.boss_torso_right', 'combat.boss_torso_right', 'combat.boss_torso_right'],
      ['combat.boss_torso_left', 'combat.boss_torso_left', 'combat.boss_torso_left', 'combat.boss_torso_mid', 'combat.boss_torso_mid', 'combat.boss_torso_mid', 'combat.boss_torso_right', 'combat.boss_torso_right', 'combat.boss_torso_right'],
      ['combat.boss_torso_left', 'combat.boss_torso_left', 'combat.boss_torso_left', 'combat.boss_torso_mid', 'combat.boss_torso_mid', 'combat.boss_torso_mid', 'combat.boss_torso_right', 'combat.boss_torso_right', 'combat.boss_torso_right']
    ]
  },
  dragon: {
    name: 'COLOSSAL DRAGON',
    tiles: [
      ['dungeon.wall', 'dungeon.wall', 'dungeon.floor', 'dungeon.floor', 'dungeon.stairs_down', 'dungeon.floor', 'dungeon.floor', 'dungeon.wall', 'dungeon.wall'],
      ['dungeon.wall', 'dungeon.floor', 'dungeon.floor', 'dungeon.wall', 'dungeon.wall', 'dungeon.wall', 'dungeon.floor', 'dungeon.floor', 'dungeon.wall'],
      ['dungeon.floor', 'dungeon.floor', 'dungeon.stairs_down', 'dungeon.wall', 'dungeon.wall', 'dungeon.wall', 'dungeon.stairs_down', 'dungeon.floor', 'dungeon.floor'],
      ['dungeon.floor', 'dungeon.wall', 'dungeon.wall', 'dungeon.stairs_down', 'dungeon.wall', 'dungeon.stairs_down', 'dungeon.wall', 'dungeon.wall', 'dungeon.floor'],
      ['dungeon.stairs_down', 'dungeon.wall', 'dungeon.stairs_down', 'dungeon.wall', 'dungeon.stairs_down', 'dungeon.wall', 'dungeon.stairs_down', 'dungeon.wall', 'dungeon.stairs_down'],
      ['dungeon.floor', 'dungeon.wall', 'dungeon.wall', 'dungeon.wall', 'dungeon.stairs_down', 'dungeon.wall', 'dungeon.wall', 'dungeon.wall', 'dungeon.floor'],
      ['dungeon.floor', 'dungeon.floor', 'dungeon.wall', 'dungeon.wall', 'dungeon.stairs_down', 'dungeon.wall', 'dungeon.wall', 'dungeon.floor', 'dungeon.floor'],
      ['dungeon.wall', 'dungeon.floor', 'dungeon.floor', 'dungeon.wall', 'dungeon.wall', 'dungeon.wall', 'dungeon.floor', 'dungeon.floor', 'dungeon.wall'],
      ['dungeon.wall', 'dungeon.wall', 'dungeon.wall', 'dungeon.floor', 'dungeon.wall', 'dungeon.floor', 'dungeon.wall', 'dungeon.wall', 'dungeon.wall']
    ]
  },
  demon: {
    name: 'DEMON OVERLORD',
    tiles: [
      ['dungeon.wall', 'dungeon.stairs_down', 'dungeon.wall', 'dungeon.wall', 'dungeon.floor', 'dungeon.wall', 'dungeon.wall', 'dungeon.stairs_down', 'dungeon.wall'],
      ['dungeon.stairs_down', 'dungeon.wall', 'dungeon.wall', 'dungeon.floor', 'dungeon.stairs_down', 'dungeon.floor', 'dungeon.wall', 'dungeon.wall', 'dungeon.stairs_down'],
      ['dungeon.wall', 'dungeon.wall', 'dungeon.stairs_down', 'dungeon.stairs_down', 'dungeon.wall', 'dungeon.stairs_down', 'dungeon.stairs_down', 'dungeon.wall', 'dungeon.wall'],
      ['dungeon.floor', 'dungeon.stairs_down', 'dungeon.wall', 'dungeon.wall', 'dungeon.stairs_down', 'dungeon.wall', 'dungeon.wall', 'dungeon.stairs_down', 'dungeon.floor'],
      ['dungeon.floor', 'dungeon.floor', 'dungeon.stairs_down', 'dungeon.wall', 'dungeon.stairs_down', 'dungeon.wall', 'dungeon.stairs_down', 'dungeon.floor', 'dungeon.floor'],
      ['dungeon.wall', 'dungeon.floor', 'dungeon.floor', 'dungeon.stairs_down', 'dungeon.stairs_down', 'dungeon.stairs_down', 'dungeon.floor', 'dungeon.floor', 'dungeon.wall'],
      ['dungeon.wall', 'dungeon.wall', 'dungeon.floor', 'dungeon.floor', 'dungeon.wall', 'dungeon.floor', 'dungeon.floor', 'dungeon.wall', 'dungeon.wall'],
      ['dungeon.wall', 'dungeon.wall', 'dungeon.wall', 'dungeon.floor', 'dungeon.floor', 'dungeon.floor', 'dungeon.wall', 'dungeon.wall', 'dungeon.wall'],
      ['dungeon.wall', 'dungeon.wall', 'dungeon.wall', 'dungeon.wall', 'dungeon.stairs_down', 'dungeon.wall', 'dungeon.wall', 'dungeon.wall', 'dungeon.wall']
    ]
  },
  titan: {
    name: 'OBSIDIAN TITAN',
    tiles: [
      ['dungeon.wall', 'dungeon.wall', 'dungeon.wall', 'dungeon.wall', 'dungeon.wall', 'dungeon.wall', 'dungeon.wall', 'dungeon.wall', 'dungeon.wall'],
      ['dungeon.wall', 'dungeon.floor', 'dungeon.floor', 'dungeon.floor', 'dungeon.floor', 'dungeon.floor', 'dungeon.floor', 'dungeon.floor', 'dungeon.wall'],
      ['dungeon.wall', 'dungeon.floor', 'dungeon.stairs_down', 'dungeon.stairs_down', 'dungeon.floor', 'dungeon.stairs_down', 'dungeon.stairs_down', 'dungeon.floor', 'dungeon.wall'],
      ['dungeon.wall', 'dungeon.floor', 'dungeon.stairs_down', 'dungeon.wall', 'dungeon.floor', 'dungeon.wall', 'dungeon.stairs_down', 'dungeon.floor', 'dungeon.wall'],
      ['dungeon.wall', 'dungeon.floor', 'dungeon.floor', 'dungeon.floor', 'dungeon.stairs_down', 'dungeon.floor', 'dungeon.floor', 'dungeon.floor', 'dungeon.wall'],
      ['dungeon.wall', 'dungeon.floor', 'dungeon.stairs_down', 'dungeon.stairs_down', 'dungeon.stairs_down', 'dungeon.stairs_down', 'dungeon.stairs_down', 'dungeon.floor', 'dungeon.wall'],
      ['dungeon.wall', 'dungeon.floor', 'dungeon.floor', 'dungeon.stairs_down', 'dungeon.floor', 'dungeon.stairs_down', 'dungeon.floor', 'dungeon.floor', 'dungeon.wall'],
      ['dungeon.wall', 'dungeon.floor', 'dungeon.floor', 'dungeon.floor', 'dungeon.floor', 'dungeon.floor', 'dungeon.floor', 'dungeon.floor', 'dungeon.wall'],
      ['dungeon.wall', 'dungeon.wall', 'dungeon.wall', 'dungeon.wall', 'dungeon.wall', 'dungeon.wall', 'dungeon.wall', 'dungeon.wall', 'dungeon.wall']
    ]
  }
};

interface InspectorProps {
  level: EditorLevel;
  activeLayer: EditLayer;
  onSelectLayer: (layer: EditLayer) => void;
  showTerrain: boolean;
  onToggleShowTerrain: () => void;
  showExits: boolean;
  onToggleShowExits: () => void;
  showObjects: boolean;
  onToggleShowObjects: () => void;
  showRegions: boolean;
  onToggleShowRegions: () => void;
  selectedEntityIndex: number | null;
  onSelectEntityIndex: (index: number | null) => void;
  onUpdateLevelMeta: (updates: Partial<EditorLevel>) => void;
  onUpdateSpawn: (spawn: PlayerSpawn) => void;
  onAddExit: (exit: LevelExit) => void;
  onUpdateExit: (index: number, exit: LevelExit) => void;
  onDeleteExit: (index: number) => void;
  onAddObject: (obj: LevelObject) => void;
  onUpdateObject: (index: number, obj: LevelObject) => void;
  onDeleteObject: (index: number) => void;
  onAddRegion: (region: LevelRegion) => void;
  onUpdateRegion: (index: number, region: LevelRegion) => void;
  onDeleteRegion: (index: number) => void;
}

export const Inspector: React.FC<InspectorProps> = ({
  level,
  activeLayer,
  onSelectLayer,
  showTerrain,
  onToggleShowTerrain,
  showExits,
  onToggleShowExits,
  showObjects,
  onToggleShowObjects,
  showRegions,
  onToggleShowRegions,
  selectedEntityIndex,
  onSelectEntityIndex,
  onUpdateLevelMeta,
  onUpdateSpawn,
  onAddExit,
  onUpdateExit,
  onDeleteExit,
  onAddObject,
  onUpdateObject,
  onDeleteObject,
  onAddRegion,
  onUpdateRegion,
  onDeleteRegion,
}) => {
  const isBattleScreen = !!(level.isScreen && (level.mapId === 'SCREEN_BATTLE' || level.id.includes('battle')));
  const isTitleScreen = !!(level.isScreen && (level.mapId === 'SCREEN_TITLE' || level.id === 'title'));
  const [tab, setTab] = useState<'context' | 'layers' | 'map' | 'battle' | 'title'>('context');

  useEffect(() => {
    if (isBattleScreen) {
      setTab('battle');
    } else if (isTitleScreen) {
      setTab('title');
    }
  }, [level.id, isBattleScreen, isTitleScreen]);
  const [previewTick, setPreviewTick] = useState<number>(0);
  useEffect(() => {
    const timer = setInterval(() => {
      setPreviewTick((t) => (t + 1) % 60);
    }, 250);
    return () => clearInterval(timer);
  }, []);

  // Collect all tiles from all tilesets with scoped IDs
  const allTilesWithScope = Object.values(BUILTIN_TILESETS).flatMap((ts) =>
    ts.tiles.map((t) => ({
      ...t,
      tilesetId: ts.id,
      tilesetLabel: ts.label,
      scopedId: `${ts.id}.${t.id}`,
    }))
  );

  const selectedExit = activeLayer === 'exits' && selectedEntityIndex !== null ? level.exits[selectedEntityIndex] : null;
  const selectedObject = activeLayer === 'objects' && selectedEntityIndex !== null ? level.objects[selectedEntityIndex] : null;
  const selectedRegion = activeLayer === 'regions' && selectedEntityIndex !== null ? level.regions[selectedEntityIndex] : null;

  return (
    <div className="panel inspector-panel">
      <div className="inspector-tabs">
        <button
          className={`tab-btn ${tab === 'context' ? 'active' : ''}`}
          onClick={() => setTab('context')}
          title="Current layer properties and entity details"
        >
          {activeLayer === 'terrain'
            ? '🎨 Terrain'
            : activeLayer === 'spawn'
            ? '🚩 Spawn'
            : activeLayer === 'exits'
            ? '🚪 Exits'
            : activeLayer === 'objects'
            ? '👾 Objects'
            : '🏷️ Regions'}
        </button>
        <button
          className={`tab-btn ${tab === 'layers' ? 'active' : ''}`}
          onClick={() => setTab('layers')}
          title="Layers and edit modes"
        >
          📑 Layers
        </button>
        <button
          className={`tab-btn ${tab === 'map' ? 'active' : ''}`}
          onClick={() => setTab('map')}
          title="Map metadata and settings"
        >
          ⚙️ Map Info
        </button>
        {isBattleScreen && (
          <button
            className={`tab-btn ${tab === 'battle' ? 'active' : ''}`}
            onClick={() => setTab('battle')}
            title="Battle HUD Layout and Coordinates"
          >
            ⚔️ Battle HUD
          </button>
        )}
        {isTitleScreen && (
          <button
            className={`tab-btn ${tab === 'title' ? 'active' : ''}`}
            onClick={() => setTab('title')}
            title="Title Screen Layout, Big Image, Prompt & Credits"
          >
            👑 Title Studio
          </button>
        )}
      </div>

      <div className="panel-body inspector-content">
        {tab === 'layers' && (
          <div className="inspector-section">
            <LayerPanel
              activeLayer={activeLayer}
              onSelectLayer={onSelectLayer}
              showTerrain={showTerrain}
              onToggleShowTerrain={onToggleShowTerrain}
              showExits={showExits}
              onToggleShowExits={onToggleShowExits}
              showObjects={showObjects}
              onToggleShowObjects={onToggleShowObjects}
              showRegions={showRegions}
              onToggleShowRegions={onToggleShowRegions}
              exitCount={level.exits.length}
              objectCount={level.objects.length}
              regionCount={level.regions.length}
              embedded={true}
            />
          </div>
        )}

        {tab === 'map' && (
          <div className="inspector-section">
            <h4>Map Configuration</h4>
            <div className="form-group">
              <label>Scene ID</label>
              <input
                type="text"
                value={level.id}
                onChange={(e) => onUpdateLevelMeta({ id: e.target.value.toLowerCase().replace(/[^a-z0-9_]/g, '') })}
              />
            </div>
            <div className="form-group">
              <label>Scene Name</label>
              <input
                type="text"
                value={level.name}
                onChange={(e) => onUpdateLevelMeta({ name: e.target.value })}
              />
            </div>
            <div className="form-row">
              <div className="form-group">
                <label>Width (cols)</label>
                <input
                  type="number"
                  min="4"
                  max="40"
                  value={level.width}
                  onChange={(e) => onUpdateLevelMeta({ width: parseInt(e.target.value) || 20 })}
                />
              </div>
              <div className="form-group">
                <label>Height (rows)</label>
                <input
                  type="number"
                  min="4"
                  max="24"
                  value={level.height}
                  onChange={(e) => onUpdateLevelMeta({ height: parseInt(e.target.value) || 18 })}
                />
              </div>
            </div>
            <div className="form-group">
              <label>BGM Track</label>
              <select
                value={level.music}
                onChange={(e) => onUpdateLevelMeta({ music: e.target.value })}
              >
                <option value="MUSIC_OVERWORLD">MUSIC_OVERWORLD</option>
                <option value="MUSIC_TITLE">MUSIC_TITLE</option>
                <option value="MUSIC_TOWN">MUSIC_TOWN</option>
                <option value="MUSIC_DUNGEON">MUSIC_DUNGEON</option>
                <option value="MUSIC_BATTLE">MUSIC_BATTLE</option>
                <option value="MUSIC_DESOLATE">MUSIC_DESOLATE</option>
                <option value="MUSIC_DESOLATE_LANDSCAPE">MUSIC_DESOLATE_LANDSCAPE (desolate_landscape.uge)</option>
                <option value="MUSIC_FOREST">MUSIC_FOREST (Forest.uge)</option>
                <option value="MUSIC_BOSS">MUSIC_BOSS (Boss fight.uge)</option>
              </select>
            </div>
            <div className="form-group">
              <label>Engine Map ID</label>
              <input
                type="text"
                value={level.mapId}
                onChange={(e) => onUpdateLevelMeta({ mapId: e.target.value })}
              />
            </div>
          </div>
        )}

        {tab === 'battle' && (
          <div className="inspector-section">
            <h4>⚔️ Battle Screen HUD Layout</h4>
            <p className="hint-text">
              Configure row and column coordinates for every battle screen HUD element (matches <code>assets/battle_screen_mockup.jpg</code>).
            </p>

            {/* Turn Banner */}
            <div className="form-group">
              <label>Turn Banner Row (0-17)</label>
              <input
                type="number"
                min={0}
                max={17}
                value={level.battleHudLayout?.turn_banner_row ?? 0}
                onChange={(e) =>
                  onUpdateLevelMeta({
                    battleHudLayout: {
                      ...(level.battleHudLayout || {}),
                      turn_banner_row: parseInt(e.target.value) || 0,
                    },
                  })
                }
              />
            </div>

            {/* 👑 Boss 9x9 Meta-Tile Configuration */}
            <div className="form-group" style={{ background: 'rgba(142, 68, 173, 0.15)', border: '1px solid #8e44ad', borderRadius: 6, padding: 8, margin: '8px 0' }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                <label style={{ margin: 0, fontWeight: 700, color: '#f59e0b' }}>👑 Boss 9×9 Meta-Tile</label>
                <label style={{ display: 'flex', alignItems: 'center', gap: 4, fontSize: 12 }}>
                  <input
                    type="checkbox"
                    checked={level.bossMetaTile?.enabled ?? (level.id === 'boss' || level.id.includes('boss'))}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        bossMetaTile: {
                          ...(level.bossMetaTile || { width: 9, height: 9, x: 5, y: 1, name: 'LORD GIAUSAR', hp: 100, max_hp: 100, tiles: BOSS_9X9_TEMPLATES.dragon.tiles }),
                          enabled: e.target.checked,
                        },
                      })
                    }
                  />
                  Enable Boss Meta-Tile
                </label>
              </div>

              {(level.bossMetaTile?.enabled ?? (level.id === 'boss' || level.id.includes('boss'))) && (
                <div style={{ marginTop: 8 }}>
                  <div style={{ display: 'grid', gridTemplateColumns: '2fr 1fr 1fr', gap: 6 }}>
                    <div>
                      <label style={{ fontSize: 11 }}>Boss Name</label>
                      <input
                        type="text"
                        value={level.bossMetaTile?.name ?? 'LORD GIAUSAR'}
                        onChange={(e) =>
                          onUpdateLevelMeta({
                            bossMetaTile: {
                              ...(level.bossMetaTile || { enabled: true, width: 9, height: 9, x: 5, y: 1, hp: 100, max_hp: 100 }),
                              name: e.target.value,
                            },
                          })
                        }
                      />
                    </div>
                    <div>
                      <label style={{ fontSize: 11 }}>HP</label>
                      <input
                        type="number"
                        min={1}
                        max={999}
                        value={level.bossMetaTile?.hp ?? 100}
                        onChange={(e) =>
                          onUpdateLevelMeta({
                            bossMetaTile: {
                              ...(level.bossMetaTile || { enabled: true, width: 9, height: 9, x: 5, y: 1, name: 'LORD GIAUSAR', max_hp: 100 }),
                              hp: parseInt(e.target.value) || 100,
                            },
                          })
                        }
                      />
                    </div>
                    <div>
                      <label style={{ fontSize: 11 }}>Max HP</label>
                      <input
                        type="number"
                        min={1}
                        max={999}
                        value={level.bossMetaTile?.max_hp ?? 100}
                        onChange={(e) =>
                          onUpdateLevelMeta({
                            bossMetaTile: {
                              ...(level.bossMetaTile || { enabled: true, width: 9, height: 9, x: 5, y: 1, name: 'LORD GIAUSAR', hp: 100 }),
                              max_hp: parseInt(e.target.value) || 100,
                            },
                          })
                        }
                      />
                    </div>
                  </div>

                  <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr 1fr 1fr', gap: 6, margin: '6px 0' }}>
                    <div>
                      <label style={{ fontSize: 11 }}>Col X</label>
                      <input
                        type="number"
                        min={0}
                        max={19}
                        value={level.bossMetaTile?.x ?? 5}
                        onChange={(e) =>
                          onUpdateLevelMeta({
                            bossMetaTile: {
                              ...(level.bossMetaTile || { enabled: true, width: 9, height: 9, y: 1 }),
                              x: parseInt(e.target.value) || 0,
                            },
                          })
                        }
                      />
                    </div>
                    <div>
                      <label style={{ fontSize: 11 }}>Row Y</label>
                      <input
                        type="number"
                        min={0}
                        max={17}
                        value={level.bossMetaTile?.y ?? 1}
                        onChange={(e) =>
                          onUpdateLevelMeta({
                            bossMetaTile: {
                              ...(level.bossMetaTile || { enabled: true, width: 9, height: 9, x: 5 }),
                              y: parseInt(e.target.value) || 0,
                            },
                          })
                        }
                      />
                    </div>
                    <div>
                      <label style={{ fontSize: 11 }}>Width</label>
                      <input
                        type="number"
                        min={1}
                        max={12}
                        value={level.bossMetaTile?.width ?? 9}
                        onChange={(e) =>
                          onUpdateLevelMeta({
                            bossMetaTile: {
                              ...(level.bossMetaTile || { enabled: true, height: 9, x: 5, y: 1 }),
                              width: parseInt(e.target.value) || 9,
                            },
                          })
                        }
                      />
                    </div>
                    <div>
                      <label style={{ fontSize: 11 }}>Height</label>
                      <input
                        type="number"
                        min={1}
                        max={12}
                        value={level.bossMetaTile?.height ?? 9}
                        onChange={(e) =>
                          onUpdateLevelMeta({
                            bossMetaTile: {
                              ...(level.bossMetaTile || { enabled: true, width: 9, x: 5, y: 1 }),
                              height: parseInt(e.target.value) || 9,
                            },
                          })
                        }
                      />
                    </div>
                  </div>

                  {/* 9x9 Quick Templates */}
                  <div style={{ margin: '6px 0' }}>
                    <label style={{ fontSize: 10, color: '#cbd5e1' }}>9×9 Boss Presets:</label>
                    <div style={{ display: 'flex', gap: 4, flexWrap: 'wrap', marginTop: 2 }}>
                      {Object.entries(BOSS_9X9_TEMPLATES).map(([key, tmpl]) => (
                        <button
                          key={key}
                          type="button"
                          className="btn btn-sm"
                          style={{ fontSize: 10, padding: '2px 6px' }}
                          onClick={() =>
                            onUpdateLevelMeta({
                              bossMetaTile: {
                                ...(level.bossMetaTile || { enabled: true, width: 9, height: 9, x: 5, y: 1 }),
                                name: tmpl.name,
                                tiles: JSON.parse(JSON.stringify(tmpl.tiles)),
                              },
                            })
                          }
                        >
                          👑 {tmpl.name}
                        </button>
                      ))}
                    </div>
                  </div>

                  {/* 9x9 Tile Matrix Visual Editor */}
                  <label style={{ fontSize: 11 }}>9×9 Meta-Tile Matrix Grid (Click cell to cycle tile):</label>
                  <div
                    style={{
                      display: 'grid',
                      gridTemplateColumns: `repeat(${level.bossMetaTile?.width || 9}, 18px)`,
                      gap: 2,
                      background: '#090d16',
                      padding: 6,
                      borderRadius: 4,
                      justifyContent: 'center',
                      overflowX: 'auto',
                    }}
                  >
                    {Array.from({ length: level.bossMetaTile?.height || 9 }).map((_, r) =>
                      Array.from({ length: level.bossMetaTile?.width || 9 }).map((_, c) => {
                        const curTile = level.bossMetaTile?.tiles?.[r]?.[c] || 'dungeon.floor';
                        const tDef = allTilesWithScope.find((t) => t.scopedId === curTile || t.id === curTile);
                        return (
                          <div
                            key={`${r}-${c}`}
                            title={`[${r},${c}] ${curTile}`}
                            onClick={() => {
                              const newTiles = level.bossMetaTile?.tiles
                                ? JSON.parse(JSON.stringify(level.bossMetaTile.tiles))
                                : Array.from({ length: 9 }, () => Array(9).fill('dungeon.floor'));
                              while (newTiles.length <= r) newTiles.push(Array(9).fill('dungeon.floor'));
                              while (newTiles[r].length <= c) newTiles[r].push('dungeon.floor');
                              newTiles[r][c] =
                                curTile === 'dungeon.wall'
                                  ? 'dungeon.stairs_down'
                                  : curTile === 'dungeon.stairs_down'
                                  ? 'dungeon.floor'
                                  : 'dungeon.wall';
                              onUpdateLevelMeta({
                                bossMetaTile: {
                                  ...(level.bossMetaTile || { enabled: true, width: 9, height: 9, x: 5, y: 1 }),
                                  tiles: newTiles,
                                },
                              });
                            }}
                            style={{
                              width: 18,
                              height: 18,
                              border: '1px solid rgba(245, 158, 11, 0.4)',
                              borderRadius: 2,
                              cursor: 'pointer',
                              display: 'flex',
                              alignItems: 'center',
                              justifyContent: 'center',
                              background: curTile.includes('wall') ? '#450a0a' : curTile.includes('stairs') ? '#7f1d1d' : '#1e1b4b',
                            }}
                          >
                            {tDef?.image_url ? (
                              <img src={tDef.image_url} alt="" style={{ width: 14, height: 14, imageRendering: 'pixelated' }} />
                            ) : (
                              <span style={{ fontSize: 8, color: '#fca5a5' }}>{curTile.slice(0, 2)}</span>
                            )}
                          </div>
                        );
                      })
                    )}
                  </div>
                </div>
              )}
            </div>

            {/* Enemy Roster Layout */}
            <div className="form-group">
              <label style={{ fontWeight: 600 }}>👾 Enemy Roster Layout</label>
              <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 6 }}>
                <div>
                  <label style={{ fontSize: 11 }}>HP Row</label>
                  <input
                    type="number"
                    min={0}
                    max={17}
                    value={level.battleHudLayout?.enemy_hp_row ?? 1}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        battleHudLayout: {
                          ...(level.battleHudLayout || {}),
                          enemy_hp_row: parseInt(e.target.value) || 0,
                        },
                      })
                    }
                  />
                </div>
                <div>
                  <label style={{ fontSize: 11 }}>Sprite Row</label>
                  <input
                    type="number"
                    min={0}
                    max={17}
                    value={level.battleHudLayout?.enemy_sprite_row ?? 2}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        battleHudLayout: {
                          ...(level.battleHudLayout || {}),
                          enemy_sprite_row: parseInt(e.target.value) || 0,
                        },
                      })
                    }
                  />
                </div>
                <div>
                  <label style={{ fontSize: 11 }}>Target Arrow Row</label>
                  <input
                    type="number"
                    min={0}
                    max={17}
                    value={level.battleHudLayout?.enemy_cursor_row ?? 4}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        battleHudLayout: {
                          ...(level.battleHudLayout || {}),
                          enemy_cursor_row: parseInt(e.target.value) || 0,
                        },
                      })
                    }
                  />
                </div>
                <div>
                  <label style={{ fontSize: 11 }}>Column Step</label>
                  <input
                    type="number"
                    min={1}
                    max={10}
                    value={level.battleHudLayout?.enemy_col_step ?? 7}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        battleHudLayout: {
                          ...(level.battleHudLayout || {}),
                          enemy_col_step: parseInt(e.target.value) || 7,
                        },
                      })
                    }
                  />
                </div>
              </div>
            </div>

            {/* Dual-Column Status Panel */}
            <div className="form-group">
              <label style={{ fontWeight: 600 }}>👤 Hero & Status Panel (Rows 6 & 7)</label>
              <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 6 }}>
                <div>
                  <label style={{ fontSize: 11 }}>Hero Label Row</label>
                  <input
                    type="number"
                    min={0}
                    max={17}
                    value={level.battleHudLayout?.hero_label_row ?? 6}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        battleHudLayout: {
                          ...(level.battleHudLayout || {}),
                          hero_label_row: parseInt(e.target.value) || 0,
                        },
                      })
                    }
                  />
                </div>
                <div>
                  <label style={{ fontSize: 11 }}>Heart HP Row / Col</label>
                  <div style={{ display: 'flex', gap: 4 }}>
                    <input
                      type="number"
                      min={0}
                      max={17}
                      value={level.battleHudLayout?.hero_hp_row ?? 6}
                      onChange={(e) =>
                        onUpdateLevelMeta({
                          battleHudLayout: {
                            ...(level.battleHudLayout || {}),
                            hero_hp_row: parseInt(e.target.value) || 0,
                          },
                        })
                      }
                    />
                    <input
                      type="number"
                      min={0}
                      max={19}
                      value={level.battleHudLayout?.hero_hp_col ?? 13}
                      onChange={(e) =>
                        onUpdateLevelMeta({
                          battleHudLayout: {
                            ...(level.battleHudLayout || {}),
                            hero_hp_col: parseInt(e.target.value) || 0,
                          },
                        })
                      }
                    />
                  </div>
                </div>
                <div>
                  <label style={{ fontSize: 11 }}>Deck Row</label>
                  <input
                    type="number"
                    min={0}
                    max={17}
                    value={level.battleHudLayout?.deck_row ?? 7}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        battleHudLayout: {
                          ...(level.battleHudLayout || {}),
                          deck_row: parseInt(e.target.value) || 0,
                        },
                      })
                    }
                  />
                </div>
                <div>
                  <label style={{ fontSize: 11 }}>Battery AP Row / Col</label>
                  <div style={{ display: 'flex', gap: 4 }}>
                    <input
                      type="number"
                      min={0}
                      max={17}
                      value={level.battleHudLayout?.ap_row ?? 7}
                      onChange={(e) =>
                        onUpdateLevelMeta({
                          battleHudLayout: {
                            ...(level.battleHudLayout || {}),
                            ap_row: parseInt(e.target.value) || 0,
                          },
                        })
                      }
                    />
                    <input
                      type="number"
                      min={0}
                      max={19}
                      value={level.battleHudLayout?.ap_col ?? 13}
                      onChange={(e) =>
                        onUpdateLevelMeta({
                          battleHudLayout: {
                            ...(level.battleHudLayout || {}),
                            ap_col: parseInt(e.target.value) || 0,
                          },
                        })
                      }
                    />
                  </div>
                </div>
              </div>
            </div>

            {/* Hand Cards & Description */}
            <div className="form-group">
              <label style={{ fontWeight: 600 }}>🎴 Framed Cards Hand (Rows 10–15)</label>
              <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 6 }}>
                <div>
                  <label style={{ fontSize: 11 }}>Combo Row</label>
                  <input
                    type="number"
                    min={0}
                    max={17}
                    value={level.battleHudLayout?.combo_row ?? 9}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        battleHudLayout: {
                          ...(level.battleHudLayout || {}),
                          combo_row: parseInt(e.target.value) || 0,
                        },
                      })
                    }
                  />
                </div>
                <div>
                  <label style={{ fontSize: 11 }}>Cards Row</label>
                  <input
                    type="number"
                    min={0}
                    max={17}
                    value={level.battleHudLayout?.cards_row ?? 10}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        battleHudLayout: {
                          ...(level.battleHudLayout || {}),
                          cards_row: parseInt(e.target.value) || 0,
                        },
                      })
                    }
                  />
                </div>
                <div>
                  <label style={{ fontSize: 11 }}>Card Cursor Row</label>
                  <input
                    type="number"
                    min={0}
                    max={17}
                    value={level.battleHudLayout?.card_cursor_row ?? 14}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        battleHudLayout: {
                          ...(level.battleHudLayout || {}),
                          card_cursor_row: parseInt(e.target.value) || 0,
                        },
                      })
                    }
                  />
                </div>
                <div>
                  <label style={{ fontSize: 11 }}>Description Row</label>
                  <input
                    type="number"
                    min={0}
                    max={17}
                    value={level.battleHudLayout?.card_desc_row ?? 15}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        battleHudLayout: {
                          ...(level.battleHudLayout || {}),
                          card_desc_row: parseInt(e.target.value) || 0,
                        },
                      })
                    }
                  />
                </div>
              </div>
            </div>

            {/* Turn Timer Bar */}
            <div className="form-group">
              <label style={{ fontWeight: 600 }}>⏱️ Turn Timer Bar</label>
              <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 6 }}>
                <div>
                  <label style={{ fontSize: 11 }}>Timer Row</label>
                  <input
                    type="number"
                    min={0}
                    max={17}
                    value={level.battleHudLayout?.timer_row ?? 16}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        battleHudLayout: {
                          ...(level.battleHudLayout || {}),
                          timer_row: parseInt(e.target.value) || 0,
                        },
                      })
                    }
                  />
                </div>
                <div>
                  <label style={{ fontSize: 11 }}>Timer Width (tiles)</label>
                  <input
                    type="number"
                    min={1}
                    max={20}
                    value={level.battleHudLayout?.timer_width ?? 11}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        battleHudLayout: {
                          ...(level.battleHudLayout || {}),
                          timer_width: parseInt(e.target.value) || 11,
                        },
                      })
                    }
                  />
                </div>
              </div>
            </div>
          </div>
        )}

        {tab === 'title' && (
          <div className="inspector-section">
            <h4>👑 Title Screen Studio</h4>
            <p className="hint-text">
              Completely data-driven Title Screen: customize the Big Title Graphic, Game Title, Centered &ldquo;PRESS START&rdquo;, and Bottom-Row Credits.
            </p>

            {/* Game Title & Subtitle */}
            <div className="form-group">
              <label style={{ fontWeight: 600 }}>🏷️ Game Title & Subtitle Lines</label>
              <input
                type="text"
                placeholder="Game Title"
                value={level.titleLayout?.title ?? 'Giausar'}
                onChange={(e) =>
                  onUpdateLevelMeta({
                    titleLayout: {
                      ...(level.titleLayout || {}),
                      title: e.target.value,
                    },
                  })
                }
              />
              <label style={{ fontSize: 11, marginTop: 4 }}>Banner & Subtitle Lines (Row 1-5)</label>
              <textarea
                rows={5}
                style={{ fontFamily: 'monospace', fontSize: 11, width: '100%' }}
                value={(level.titleLayout?.logo?.lines ?? []).join('\n')}
                onChange={(e) =>
                  onUpdateLevelMeta({
                    titleLayout: {
                      ...(level.titleLayout || {}),
                      logo: {
                        ...(level.titleLayout?.logo || { x: 0, y: 1 }),
                        lines: e.target.value.split('\n'),
                      },
                    },
                  })
                }
              />
            </div>

            {/* Big Title Graphic / Tile Image */}
            <div className="form-group" style={{ background: 'rgba(30, 41, 59, 0.4)', padding: 8, borderRadius: 4 }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                <label style={{ fontWeight: 600 }}>🖼️ Big Title Graphic / Multi-Tile Artwork</label>
                <label style={{ display: 'flex', alignItems: 'center', gap: 4, fontSize: 12 }}>
                  <input
                    type="checkbox"
                    checked={level.titleLayout?.graphic?.enabled ?? true}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        titleLayout: {
                          ...(level.titleLayout || {}),
                          graphic: {
                            ...(level.titleLayout?.graphic || { x: 2, y: 7, width: 16, height: 5, lines: [] }),
                            enabled: e.target.checked,
                          },
                        },
                      })
                    }
                  />
                  Enabled
                </label>
              </div>

              <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr 1fr 1fr', gap: 6, margin: '6px 0' }}>
                <div>
                  <label style={{ fontSize: 11 }}>Col X</label>
                  <input
                    type="number"
                    min={0}
                    max={19}
                    value={level.titleLayout?.graphic?.x ?? 2}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        titleLayout: {
                          ...(level.titleLayout || {}),
                          graphic: {
                            ...(level.titleLayout?.graphic || { enabled: true, y: 7, width: 16, height: 5, lines: [] }),
                            x: parseInt(e.target.value) || 0,
                          },
                        },
                      })
                    }
                  />
                </div>
                <div>
                  <label style={{ fontSize: 11 }}>Row Y</label>
                  <input
                    type="number"
                    min={0}
                    max={17}
                    value={level.titleLayout?.graphic?.y ?? 7}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        titleLayout: {
                          ...(level.titleLayout || {}),
                          graphic: {
                            ...(level.titleLayout?.graphic || { enabled: true, x: 2, width: 16, height: 5, lines: [] }),
                            y: parseInt(e.target.value) || 0,
                          },
                        },
                      })
                    }
                  />
                </div>
                <div>
                  <label style={{ fontSize: 11 }}>Width</label>
                  <input
                    type="number"
                    min={1}
                    max={20}
                    value={level.titleLayout?.graphic?.width ?? 16}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        titleLayout: {
                          ...(level.titleLayout || {}),
                          graphic: {
                            ...(level.titleLayout?.graphic || { enabled: true, x: 2, y: 7, height: 5, lines: [] }),
                            width: parseInt(e.target.value) || 16,
                          },
                        },
                      })
                    }
                  />
                </div>
                <div>
                  <label style={{ fontSize: 11 }}>Height</label>
                  <input
                    type="number"
                    min={1}
                    max={18}
                    value={level.titleLayout?.graphic?.height ?? 5}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        titleLayout: {
                          ...(level.titleLayout || {}),
                          graphic: {
                            ...(level.titleLayout?.graphic || { enabled: true, x: 2, y: 7, width: 16, lines: [] }),
                            height: parseInt(e.target.value) || 5,
                          },
                        },
                      })
                    }
                  />
                </div>
              </div>

              {/* Quick Graphic Templates */}
              <div style={{ margin: '4px 0 6px' }}>
                <label style={{ fontSize: 10, color: '#94a3b8' }}>Quick Templates:</label>
                <div style={{ display: 'flex', gap: 4, flexWrap: 'wrap', marginTop: 2 }}>
                  <button
                    type="button"
                    className="btn btn-sm"
                    style={{ fontSize: 10, padding: '2px 6px' }}
                    onClick={() =>
                      onUpdateLevelMeta({
                        titleLayout: {
                          ...(level.titleLayout || {}),
                          graphic: {
                            ...(level.titleLayout?.graphic || { enabled: true, x: 2, y: 7, width: 16, height: 5 }),
                            lines: [
                              '  /\\____/\\    ',
                              ' (  o  o  )   ',
                              ' (  ==0== )   ',
                              '  )      (    ',
                              ' (________)   ',
                            ],
                          },
                        },
                      })
                    }
                  >
                    🐉 Dragon / Beast
                  </button>
                  <button
                    type="button"
                    className="btn btn-sm"
                    style={{ fontSize: 10, padding: '2px 6px' }}
                    onClick={() =>
                      onUpdateLevelMeta({
                        titleLayout: {
                          ...(level.titleLayout || {}),
                          graphic: {
                            ...(level.titleLayout?.graphic || { enabled: true, x: 2, y: 7, width: 16, height: 5 }),
                            lines: [
                              '    /\\    /\\    ',
                              '   /  \\  /  \\   ',
                              '  <====><====>  ',
                              '   \\  /  \\  /   ',
                              '    \\/    \\/    ',
                            ],
                          },
                        },
                      })
                    }
                  >
                    ⚔️ Cross Blades
                  </button>
                  <button
                    type="button"
                    className="btn btn-sm"
                    style={{ fontSize: 10, padding: '2px 6px' }}
                    onClick={() =>
                      onUpdateLevelMeta({
                        titleLayout: {
                          ...(level.titleLayout || {}),
                          graphic: {
                            ...(level.titleLayout?.graphic || { enabled: true, x: 2, y: 7, width: 16, height: 5 }),
                            lines: [
                              '   |#|  |#|   ',
                              '  _|_|__|_|_  ',
                              ' |  _    _  | ',
                              ' | | |  | | | ',
                              ' |___|__|___| ',
                            ],
                          },
                        },
                      })
                    }
                  >
                    🏰 Citadel
                  </button>
                  <button
                    type="button"
                    className="btn btn-sm"
                    style={{ fontSize: 10, padding: '2px 6px' }}
                    onClick={() =>
                      onUpdateLevelMeta({
                        titleLayout: {
                          ...(level.titleLayout || {}),
                          graphic: {
                            ...(level.titleLayout?.graphic || { enabled: true, x: 2, y: 7, width: 16, height: 5 }),
                            lines: [
                              '      .---.     ',
                              '     /   . \\    ',
                              '    |  (o)  |___',
                              '  ~/         /  ',
                              '   \\________/   ',
                            ],
                          },
                        },
                      })
                    }
                  >
                    🐋 Whale Sigil
                  </button>
                </div>
              </div>

              <label style={{ fontSize: 11 }}>Tile / ASCII Artwork Lines</label>
              <textarea
                rows={5}
                style={{ fontFamily: 'monospace', fontSize: 11, width: '100%' }}
                value={(level.titleLayout?.graphic?.lines ?? []).join('\n')}
                onChange={(e) =>
                  onUpdateLevelMeta({
                    titleLayout: {
                      ...(level.titleLayout || {}),
                      graphic: {
                        ...(level.titleLayout?.graphic || { enabled: true, x: 2, y: 7, width: 16, height: 5 }),
                        lines: e.target.value.split('\n'),
                      },
                    },
                  })
                }
              />
            </div>

            {/* PRESS START Prompt */}
            <div className="form-group">
              <label style={{ fontWeight: 600 }}>🕹️ &ldquo;PRESS START&rdquo; Prompt</label>
              <div style={{ display: 'grid', gridTemplateColumns: '2fr 1fr 1fr', gap: 6 }}>
                <div>
                  <label style={{ fontSize: 11 }}>Text</label>
                  <input
                    type="text"
                    value={level.titleLayout?.prompt?.text ?? 'PRESS START'}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        titleLayout: {
                          ...(level.titleLayout || {}),
                          prompt: {
                            ...(level.titleLayout?.prompt || { x: 4, y: 14, align: 'center' }),
                            text: e.target.value,
                          },
                        },
                      })
                    }
                  />
                </div>
                <div>
                  <label style={{ fontSize: 11 }}>Row (Y)</label>
                  <input
                    type="number"
                    min={0}
                    max={17}
                    value={level.titleLayout?.prompt?.y ?? 14}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        titleLayout: {
                          ...(level.titleLayout || {}),
                          prompt: {
                            ...(level.titleLayout?.prompt || { text: 'PRESS START', x: 4, align: 'center' }),
                            y: parseInt(e.target.value) || 0,
                          },
                        },
                      })
                    }
                  />
                </div>
                <div>
                  <label style={{ fontSize: 11 }}>Align</label>
                  <select
                    value={level.titleLayout?.prompt?.align ?? 'center'}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        titleLayout: {
                          ...(level.titleLayout || {}),
                          prompt: {
                            ...(level.titleLayout?.prompt || { text: 'PRESS START', x: 4, y: 14 }),
                            align: e.target.value as any,
                          },
                        },
                      })
                    }
                  >
                    <option value="center">Center</option>
                    <option value="left">Left</option>
                    <option value="right">Right</option>
                  </select>
                </div>
              </div>
            </div>

            {/* Bottom Row Credits / Author Info */}
            <div className="form-group" style={{ background: 'rgba(30, 41, 59, 0.4)', padding: 8, borderRadius: 4 }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                <label style={{ fontWeight: 600 }}>✍️ Bottom Row Credits / Author Attribution</label>
                <label style={{ display: 'flex', alignItems: 'center', gap: 4, fontSize: 12 }}>
                  <input
                    type="checkbox"
                    checked={level.titleLayout?.credits?.enabled ?? true}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        titleLayout: {
                          ...(level.titleLayout || {}),
                          credits: {
                            ...(level.titleLayout?.credits || { text: 'GAME BY BRODRIGUES', x: 2, y: 17, align: 'right' }),
                            enabled: e.target.checked,
                          },
                        },
                      })
                    }
                  />
                  Enabled
                </label>
              </div>

              <div style={{ display: 'grid', gridTemplateColumns: '2fr 1fr 1fr', gap: 6, marginTop: 4 }}>
                <div>
                  <label style={{ fontSize: 11 }}>Credits Text</label>
                  <input
                    type="text"
                    value={level.titleLayout?.credits?.text ?? 'GAME BY BRODRIGUES'}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        titleLayout: {
                          ...(level.titleLayout || {}),
                          credits: {
                            ...(level.titleLayout?.credits || { enabled: true, x: 2, y: 17, align: 'right' }),
                            text: e.target.value,
                          },
                        },
                      })
                    }
                  />
                </div>
                <div>
                  <label style={{ fontSize: 11 }}>Row (Y)</label>
                  <input
                    type="number"
                    min={0}
                    max={17}
                    value={level.titleLayout?.credits?.y ?? 17}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        titleLayout: {
                          ...(level.titleLayout || {}),
                          credits: {
                            ...(level.titleLayout?.credits || { enabled: true, text: 'GAME BY BRODRIGUES', x: 2, align: 'right' }),
                            y: parseInt(e.target.value) || 17,
                          },
                        },
                      })
                    }
                  />
                </div>
                <div>
                  <label style={{ fontSize: 11 }}>Align</label>
                  <select
                    value={level.titleLayout?.credits?.align ?? 'right'}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        titleLayout: {
                          ...(level.titleLayout || {}),
                          credits: {
                            ...(level.titleLayout?.credits || { enabled: true, text: 'GAME BY BRODRIGUES', x: 2, y: 17 }),
                            align: e.target.value as any,
                          },
                        },
                      })
                    }
                  >
                    <option value="right">Right</option>
                    <option value="center">Center</option>
                    <option value="left">Left</option>
                  </select>
                </div>
              </div>
            </div>

            {/* Menu Options */}
            <div className="form-group">
              <label style={{ fontWeight: 600 }}>📋 Menu Options (When Menu Opens)</label>
              <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr 1fr', gap: 6, marginBottom: 6 }}>
                <div>
                  <label style={{ fontSize: 11 }}>Menu Col (X)</label>
                  <input
                    type="number"
                    min={0}
                    max={19}
                    value={level.titleLayout?.menu?.x ?? 3}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        titleLayout: {
                          ...(level.titleLayout || {}),
                          menu: {
                            ...(level.titleLayout?.menu || { caret_x: 3, first_row: 10, row_step: 2, options: [] }),
                            x: parseInt(e.target.value) || 0,
                          },
                        },
                      })
                    }
                  />
                </div>
                <div>
                  <label style={{ fontSize: 11 }}>First Row (Y)</label>
                  <input
                    type="number"
                    min={0}
                    max={17}
                    value={level.titleLayout?.menu?.first_row ?? 10}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        titleLayout: {
                          ...(level.titleLayout || {}),
                          menu: {
                            ...(level.titleLayout?.menu || { x: 3, caret_x: 3, row_step: 2, options: [] }),
                            first_row: parseInt(e.target.value) || 0,
                          },
                        },
                      })
                    }
                  />
                </div>
                <div>
                  <label style={{ fontSize: 11 }}>Row Step</label>
                  <input
                    type="number"
                    min={1}
                    max={5}
                    value={level.titleLayout?.menu?.row_step ?? 2}
                    onChange={(e) =>
                      onUpdateLevelMeta({
                        titleLayout: {
                          ...(level.titleLayout || {}),
                          menu: {
                            ...(level.titleLayout?.menu || { x: 3, caret_x: 3, first_row: 10, options: [] }),
                            row_step: parseInt(e.target.value) || 1,
                          },
                        },
                      })
                    }
                  />
                </div>
              </div>

              <label style={{ fontSize: 11 }}>Menu Option Labels</label>
              <textarea
                rows={4}
                style={{ fontFamily: 'monospace', fontSize: 11, width: '100%' }}
                value={(level.titleLayout?.menu?.options ?? []).join('\n')}
                onChange={(e) =>
                  onUpdateLevelMeta({
                    titleLayout: {
                      ...(level.titleLayout || {}),
                      menu: {
                        ...(level.titleLayout?.menu || { x: 3, caret_x: 3, first_row: 10, row_step: 2 }),
                        options: e.target.value.split('\n'),
                      },
                    },
                  })
                }
              />
            </div>
          </div>
        )}

        {tab === 'context' && activeLayer === 'spawn' && (
          <div className="inspector-section">
            <h4>Player Spawn Position</h4>
            <p className="hint-text">Click anywhere on the map in Spawn mode to move player spawn.</p>
            <div className="form-row">
              <div className="form-group">
                <label>Spawn X</label>
                <input
                  type="number"
                  min="0"
                  max={level.width - 1}
                  value={level.spawn.x}
                  onChange={(e) =>
                    onUpdateSpawn({ ...level.spawn, x: parseInt(e.target.value) || 0, facing: level.spawn.facing || 'DOWN' })
                  }
                />
              </div>
              <div className="form-group">
                <label>Spawn Y</label>
                <input
                  type="number"
                  min="0"
                  max={level.height - 1}
                  value={level.spawn.y}
                  onChange={(e) =>
                    onUpdateSpawn({ ...level.spawn, y: parseInt(e.target.value) || 0, facing: level.spawn.facing || 'DOWN' })
                  }
                />
              </div>
            </div>
            <div className="form-group">
              <label>Initial Facing</label>
              <select
                value={level.spawn.facing || 'DOWN'}
                onChange={(e) => onUpdateSpawn({ ...level.spawn, facing: e.target.value })}
              >
                <option value="UP">UP / North</option>
                <option value="DOWN">DOWN / South</option>
                <option value="LEFT">LEFT / West</option>
                <option value="RIGHT">RIGHT / East</option>
              </select>
            </div>

            {/* Hero Animation Frames */}
            <div className="form-group animation-frames-group" style={{ marginTop: 12 }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: 6 }}>
                <label style={{ margin: 0, fontWeight: 600 }}>🎞️ Hero Animation Frames</label>
                <button
                  type="button"
                  className="btn-tiny"
                  title="Quick-set Hero Frames 1 & 2"
                  onClick={() => {
                    onUpdateSpawn({
                      ...level.spawn,
                      animation_frames: [
                        'desolate_landscape.desolate_hero_01',
                        'desolate_landscape.desolate_hero_02',
                      ],
                    });
                  }}
                >
                  🧙 Hero (2 frames)
                </button>
              </div>

              {level.spawn.animation_frames && level.spawn.animation_frames.length > 0 ? (
                <>
                  {/* Live Preview */}
                  <div className="anim-preview-box">
                    {(() => {
                      const curKey = level.spawn.animation_frames[previewTick % level.spawn.animation_frames.length];
                      const t = allTilesWithScope.find((tile) => tile.scopedId === curKey || tile.id === curKey);
                      return (
                        <>
                          {t?.image_url ? (
                            <img src={t.image_url} alt="" style={{ width: 28, height: 28, imageRendering: 'pixelated' }} />
                          ) : (
                            <span style={{ fontSize: 22 }}>🧙</span>
                          )}
                          <span style={{ fontSize: 12, color: '#e6edf3' }}>
                            Playing frame {(previewTick % level.spawn.animation_frames.length) + 1} of {level.spawn.animation_frames.length} ({t?.label || curKey})
                          </span>
                        </>
                      );
                    })()}
                  </div>

                  <div className="frames-list" style={{ display: 'flex', flexDirection: 'column', gap: 4, marginBottom: 8 }}>
                    {level.spawn.animation_frames.map((frameId, fIdx) => {
                      const t = allTilesWithScope.find((tile) => tile.scopedId === frameId || tile.id === frameId);
                      return (
                        <div
                          key={fIdx}
                          className="frame-row"
                          style={{
                            display: 'flex',
                            alignItems: 'center',
                            gap: 8,
                            background: '#1c2430',
                            padding: '4px 8px',
                            borderRadius: 4,
                            border: '1px solid #2d3848',
                          }}
                        >
                          <span style={{ fontSize: 11, color: '#a0aec0', width: 45 }}>#{fIdx + 1}</span>
                          {t?.image_url && (
                            <img src={t.image_url} alt="" style={{ width: 20, height: 20, imageRendering: 'pixelated' }} />
                          )}
                          <span style={{ flex: 1, fontSize: 12, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>
                            {t?.label || frameId}
                          </span>
                          {fIdx > 0 && (
                            <button
                              type="button"
                              className="btn-icon-tiny"
                              title="Move Up"
                              onClick={() => {
                                const frames = [...level.spawn.animation_frames!];
                                const tmp = frames[fIdx - 1];
                                frames[fIdx - 1] = frames[fIdx];
                                frames[fIdx] = tmp;
                                onUpdateSpawn({ ...level.spawn, animation_frames: frames });
                              }}
                            >
                              ⬆️
                            </button>
                          )}
                          {level.spawn.animation_frames && fIdx < level.spawn.animation_frames.length - 1 && (
                            <button
                              type="button"
                              className="btn-icon-tiny"
                              title="Move Down"
                              onClick={() => {
                                const frames = [...level.spawn.animation_frames!];
                                const tmp = frames[fIdx + 1];
                                frames[fIdx + 1] = frames[fIdx];
                                frames[fIdx] = tmp;
                                onUpdateSpawn({ ...level.spawn, animation_frames: frames });
                              }}
                            >
                              ⬇️
                            </button>
                          )}
                          <button
                            type="button"
                            className="btn-icon-tiny"
                            title="Remove Frame"
                            onClick={() => {
                              const frames = level.spawn.animation_frames!.filter((_, i) => i !== fIdx);
                              onUpdateSpawn({
                                ...level.spawn,
                                animation_frames: frames.length > 0 ? frames : undefined,
                              });
                            }}
                          >
                            ❌
                          </button>
                        </div>
                      );
                    })}
                  </div>
                </>
              ) : (
                <div style={{ fontSize: 12, color: '#718096', marginBottom: 8, fontStyle: 'italic' }}>
                  No animation frames set (using default hero sprite).
                </div>
              )}

              <div style={{ display: 'flex', gap: 6, alignItems: 'center' }}>
                <select id="new-spawn-frame-select" style={{ flex: 1 }} defaultValue="">
                  <option value="" disabled>-- Select tile to add as hero frame --</option>
                  {Object.values(BUILTIN_TILESETS).map((ts) => (
                    <optgroup key={ts.id} label={ts.label}>
                      {ts.tiles.map((tile) => (
                        <option key={`${ts.id}.${tile.id}`} value={`${ts.id}.${tile.id}`}>
                          {tile.label} ({tile.gb_constant})
                        </option>
                      ))}
                    </optgroup>
                  ))}
                </select>
                <button
                  type="button"
                  className="btn-secondary btn-sm"
                  onClick={() => {
                    const sel = document.getElementById('new-spawn-frame-select') as HTMLSelectElement;
                    if (!sel || !sel.value) return;
                    const chosen = sel.value;
                    const current = level.spawn.animation_frames || [];
                    onUpdateSpawn({
                      ...level.spawn,
                      animation_frames: [...current, chosen],
                    });
                  }}
                >
                  ➕ Add Frame
                </button>
              </div>

              {/* Visual Quick-Picker Palette */}
              <div style={{ marginTop: 8 }}>
                <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: 4 }}>
                  <label style={{ fontSize: 11, color: '#a0aec0', margin: 0 }}>
                    Quick Visual Tile Picker (Click to append):
                  </label>
                  {level.spawn.animation_frames && level.spawn.animation_frames.length > 0 && (
                    <button
                      type="button"
                      className="btn-tiny"
                      style={{ color: '#f85149' }}
                      title="Clear animation frames and revert to default sprite"
                      onClick={() => onUpdateSpawn({ ...level.spawn, animation_frames: undefined })}
                    >
                      🗑️ Clear
                    </button>
                  )}
                </div>
                <div
                  style={{
                    display: 'grid',
                    gridTemplateColumns: 'repeat(auto-fill, minmax(34px, 1fr))',
                    gap: 4,
                    maxHeight: 110,
                    overflowY: 'auto',
                    background: '#141a22',
                    padding: 4,
                    borderRadius: 4,
                    border: '1px solid #2d3848',
                  }}
                >
                  {allTilesWithScope
                    .filter((t) => t.category === 'npc' || t.category === 'enemy' || t.tilesetId === level.tileset)
                    .map((tile) => (
                      <button
                        key={tile.scopedId}
                        type="button"
                        title={`Click to add ${tile.label}`}
                        style={{
                          background: '#1c2430',
                          border: '1px solid #2d3848',
                          borderRadius: 4,
                          padding: 2,
                          cursor: 'pointer',
                          display: 'flex',
                          alignItems: 'center',
                          justifyContent: 'center',
                        }}
                        onClick={() => {
                          const current = level.spawn.animation_frames || [];
                          onUpdateSpawn({
                            ...level.spawn,
                            animation_frames: [...current, tile.scopedId],
                          });
                        }}
                      >
                        <img
                          src={tile.image_url}
                          alt={tile.label}
                          width={26}
                          height={26}
                          style={{ imageRendering: 'pixelated' }}
                        />
                      </button>
                    ))}
                </div>
              </div>
            </div>
          </div>
        )}

        {tab === 'context' && activeLayer === 'terrain' && (
          <div className="inspector-section">
            <h4>Terrain Painter</h4>
            <p className="hint-text">Select a tile from the palette and click or drag on the map.</p>
            <div className="stats-box">
              <div className="stat-item">
                <span className="stat-label">Map Size:</span>
                <span className="stat-value">{level.width} × {level.height} tiles</span>
              </div>
              <div className="stat-item">
                <span className="stat-label">Active Tileset:</span>
                <span className="stat-value">{level.tileset}</span>
              </div>
            </div>
          </div>
        )}

        {tab === 'context' && activeLayer === 'exits' && (
          <div className="inspector-section">
            <div className="section-header-row">
              <h4>Exits ({level.exits.length})</h4>
              <button
                className="btn btn-sm btn-primary"
                onClick={() => {
                  onAddExit({
                    x: 12,
                    y: 0,
                    target_scene: 'mountain_pass',
                    target_x: 12,
                    target_y: 10,
                    direction: 'NORTH',
                    tile_char: '>',
                  });
                  onSelectEntityIndex(level.exits.length);
                }}
              >
                ➕ Add Exit
              </button>
            </div>

            {selectedExit && selectedEntityIndex !== null ? (
              <div className="entity-editor-box">
                <h5>Edit Selected Exit</h5>
                <div className="form-row">
                  <div className="form-group">
                    <label>Gate X</label>
                    <input
                      type="number"
                      value={selectedExit.x}
                      onChange={(e) =>
                        onUpdateExit(selectedEntityIndex, { ...selectedExit, x: parseInt(e.target.value) || 0 })
                      }
                    />
                  </div>
                  <div className="form-group">
                    <label>Gate Y</label>
                    <input
                      type="number"
                      value={selectedExit.y}
                      onChange={(e) =>
                        onUpdateExit(selectedEntityIndex, { ...selectedExit, y: parseInt(e.target.value) || 0 })
                      }
                    />
                  </div>
                </div>

                <div className="form-group">
                  <label>Target Scene</label>
                  <input
                    type="text"
                    value={selectedExit.target_scene}
                    onChange={(e) =>
                      onUpdateExit(selectedEntityIndex, { ...selectedExit, target_scene: e.target.value })
                    }
                  />
                </div>

                <div className="form-row">
                  <div className="form-group">
                    <label>Target Spawn X</label>
                    <input
                      type="number"
                      value={selectedExit.target_x}
                      onChange={(e) =>
                        onUpdateExit(selectedEntityIndex, { ...selectedExit, target_x: parseInt(e.target.value) || 0 })
                      }
                    />
                  </div>
                  <div className="form-group">
                    <label>Target Spawn Y</label>
                    <input
                      type="number"
                      value={selectedExit.target_y}
                      onChange={(e) =>
                        onUpdateExit(selectedEntityIndex, { ...selectedExit, target_y: parseInt(e.target.value) || 0 })
                      }
                    />
                  </div>
                </div>

                <div className="form-row">
                  <div className="form-group">
                    <label>Direction</label>
                    <select
                      value={selectedExit.direction || 'SOUTH'}
                      onChange={(e) =>
                        onUpdateExit(selectedEntityIndex, {
                          ...selectedExit,
                          direction: e.target.value,
                          tile_char: e.target.value === 'NORTH' || e.target.value === 'EAST' ? '>' : '<',
                        })
                      }
                    >
                      <option value="NORTH">North (Up)</option>
                      <option value="SOUTH">South (Down)</option>
                      <option value="EAST">East (Right)</option>
                      <option value="WEST">West (Left)</option>
                    </select>
                  </div>
                  <div className="form-group">
                    <label>Glyph</label>
                    <input
                      type="text"
                      maxLength={1}
                      value={selectedExit.tile_char || '>'}
                      onChange={(e) =>
                        onUpdateExit(selectedEntityIndex, { ...selectedExit, tile_char: e.target.value })
                      }
                    />
                  </div>
                </div>

                <div className="btn-group-row">
                  <button
                    className="btn btn-sm btn-danger"
                    onClick={() => onDeleteExit(selectedEntityIndex)}
                  >
                    🗑️ Delete Exit
                  </button>
                  <button
                    className="btn btn-sm"
                    onClick={() => onSelectEntityIndex(null)}
                  >
                    Done
                  </button>
                </div>
              </div>
            ) : (
              <div className="entity-list">
                {level.exits.map((ex, idx) => (
                  <div
                    key={idx}
                    className="entity-list-item"
                    onClick={() => onSelectEntityIndex(idx)}
                  >
                    <span className="item-title">
                      🚪 ({ex.x},{ex.y}) → <strong>{ex.target_scene}</strong>
                    </span>
                    <span className="item-sub">spawn ({ex.target_x},{ex.target_y})</span>
                  </div>
                ))}
              </div>
            )}
          </div>
        )}

        {tab === 'context' && activeLayer === 'objects' && (
          <div className="inspector-section">
            <div className="section-header-row">
              <h4>Objects ({level.objects.length})</h4>
              <button
                className="btn btn-sm btn-primary"
                onClick={() => {
                  const t = OBJECT_TEMPLATES[0];
                  onAddObject({
                    id: `obj_${Date.now().toString().slice(-4)}`,
                    type: t.type,
                    position: { x: Math.floor(level.width / 2), y: Math.floor(level.height / 2) },
                    properties: { ...t.defaultProps },
                  });
                  onSelectEntityIndex(level.objects.length);
                }}
              >
                ➕ Add Object
              </button>
            </div>

            {selectedObject && selectedEntityIndex !== null ? (
              <div className="entity-editor-box">
                <h5>Edit Object</h5>
                <div className="form-group">
                  <label>Object ID</label>
                  <input
                    type="text"
                    value={selectedObject.id}
                    onChange={(e) =>
                      onUpdateObject(selectedEntityIndex, { ...selectedObject, id: e.target.value })
                    }
                  />
                </div>

                <div className="form-group">
                  <label>Type</label>
                  <select
                    value={selectedObject.type}
                    onChange={(e) => {
                      const newType = e.target.value as any;
                      const tmpl = OBJECT_TEMPLATES.find((t) => t.type === newType);
                      onUpdateObject(selectedEntityIndex, {
                        ...selectedObject,
                        type: newType,
                        properties: tmpl ? { ...tmpl.defaultProps } : selectedObject.properties,
                      });
                    }}
                  >
                    {OBJECT_TEMPLATES.map((t) => (
                      <option key={t.type} value={t.type}>
                        {t.icon} {t.label}
                      </option>
                    ))}
                  </select>
                </div>

                <div className="form-row">
                  <div className="form-group">
                    <label>Position X</label>
                    <input
                      type="number"
                      value={selectedObject.position.x}
                      onChange={(e) =>
                        onUpdateObject(selectedEntityIndex, {
                          ...selectedObject,
                          position: { ...selectedObject.position, x: parseInt(e.target.value) || 0 },
                        })
                      }
                    />
                  </div>
                  <div className="form-group">
                    <label>Position Y</label>
                    <input
                      type="number"
                      value={selectedObject.position.y}
                      onChange={(e) =>
                        onUpdateObject(selectedEntityIndex, {
                          ...selectedObject,
                          position: { ...selectedObject.position, y: parseInt(e.target.value) || 0 },
                        })
                      }
                    />
                  </div>
                </div>

                <div className="form-group">
                  <label>Display Name</label>
                  <input
                    type="text"
                    value={selectedObject.properties?.display_name || ''}
                    onChange={(e) =>
                      onUpdateObject(selectedEntityIndex, {
                        ...selectedObject,
                        properties: { ...selectedObject.properties, display_name: e.target.value },
                      })
                    }
                  />
                </div>

                {/* 👑 Boss Meta-Tile (e.g. 9x9 Boss) */}
                <div className="form-group" style={{ background: 'rgba(142, 68, 173, 0.15)', border: '1px solid #8e44ad', borderRadius: 6, padding: 8, margin: '8px 0' }}>
                  <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                    <label style={{ margin: 0, fontWeight: 700, color: '#f59e0b' }}>👑 Boss Meta-Tile</label>
                    <label style={{ display: 'flex', alignItems: 'center', gap: 4, fontSize: 12 }}>
                      <input
                        type="checkbox"
                        checked={selectedObject.is_boss ?? (selectedObject.sprite_width === 9)}
                        onChange={(e) => {
                          const isBoss = e.target.checked;
                          onUpdateObject(selectedEntityIndex, {
                            ...selectedObject,
                            is_boss: isBoss,
                            sprite_width: isBoss ? 9 : 1,
                            sprite_height: isBoss ? 9 : 1,
                            meta_tiles: isBoss ? (selectedObject.meta_tiles || BOSS_9X9_TEMPLATES.dragon.tiles) : undefined,
                          });
                        }}
                      />
                      Is Boss
                    </label>
                  </div>

                  <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr auto', gap: 6, marginTop: 6, alignItems: 'flex-end' }}>
                    <div>
                      <label style={{ fontSize: 11 }}>Width (Tiles)</label>
                      <input
                        type="number"
                        min={1}
                        max={12}
                        value={selectedObject.sprite_width ?? (selectedObject.is_boss ? 9 : 1)}
                        onChange={(e) =>
                          onUpdateObject(selectedEntityIndex, {
                            ...selectedObject,
                            sprite_width: parseInt(e.target.value) || 1,
                          })
                        }
                      />
                    </div>
                    <div>
                      <label style={{ fontSize: 11 }}>Height (Tiles)</label>
                      <input
                        type="number"
                        min={1}
                        max={12}
                        value={selectedObject.sprite_height ?? (selectedObject.is_boss ? 9 : 1)}
                        onChange={(e) =>
                          onUpdateObject(selectedEntityIndex, {
                            ...selectedObject,
                            sprite_height: parseInt(e.target.value) || 1,
                          })
                        }
                      />
                    </div>
                    <button
                      type="button"
                      className="btn btn-sm btn-primary"
                      style={{ height: 32, fontSize: 11 }}
                      title="Set to 9x9 Colossal Boss Meta-Tile"
                      onClick={() =>
                        onUpdateObject(selectedEntityIndex, {
                          ...selectedObject,
                          is_boss: true,
                          sprite_width: 9,
                          sprite_height: 9,
                          meta_tiles: BOSS_9X9_TEMPLATES.dragon.tiles,
                        })
                      }
                    >
                      👑 9×9 Boss
                    </button>
                  </div>

                  {/* Quick Presets if Boss or multi-tile */}
                  {(selectedObject.is_boss || (selectedObject.sprite_width && selectedObject.sprite_width >= 3)) && (
                    <div style={{ marginTop: 8 }}>
                      <label style={{ fontSize: 10, color: '#cbd5e1' }}>9×9 Meta-Tile Presets:</label>
                      <div style={{ display: 'flex', gap: 4, flexWrap: 'wrap', margin: '4px 0' }}>
                        {Object.entries(BOSS_9X9_TEMPLATES).map(([key, tmpl]) => (
                          <button
                            key={key}
                            type="button"
                            className="btn btn-sm"
                            style={{ fontSize: 10, padding: '2px 6px' }}
                            onClick={() =>
                              onUpdateObject(selectedEntityIndex, {
                                ...selectedObject,
                                is_boss: true,
                                sprite_width: 9,
                                sprite_height: 9,
                                meta_tiles: JSON.parse(JSON.stringify(tmpl.tiles)),
                                properties: {
                                  ...(selectedObject.properties || {}),
                                  display_name: tmpl.name,
                                },
                              })
                            }
                          >
                            👑 {tmpl.name}
                          </button>
                        ))}
                      </div>

                      {/* 9x9 Visual Grid */}
                      <label style={{ fontSize: 11 }}>9×9 Meta-Tile Grid (Click cell to cycle tile):</label>
                      <div
                        style={{
                          display: 'grid',
                          gridTemplateColumns: `repeat(${selectedObject.sprite_width || 9}, 18px)`,
                          gap: 2,
                          background: '#090d16',
                          padding: 6,
                          borderRadius: 4,
                          justifyContent: 'center',
                          overflowX: 'auto',
                        }}
                      >
                        {Array.from({ length: selectedObject.sprite_height || 9 }).map((_, r) =>
                          Array.from({ length: selectedObject.sprite_width || 9 }).map((_, c) => {
                            const curTile = selectedObject.meta_tiles?.[r]?.[c] || 'dungeon.floor';
                            const tDef = allTilesWithScope.find((t) => t.scopedId === curTile || t.id === curTile);
                            return (
                              <div
                                key={`${r}-${c}`}
                                title={`[${r},${c}] ${curTile}`}
                                onClick={() => {
                                  const newTiles = selectedObject.meta_tiles
                                    ? JSON.parse(JSON.stringify(selectedObject.meta_tiles))
                                    : Array.from({ length: 9 }, () => Array(9).fill('dungeon.floor'));
                                  while (newTiles.length <= r) newTiles.push(Array(9).fill('dungeon.floor'));
                                  while (newTiles[r].length <= c) newTiles[r].push('dungeon.floor');
                                  newTiles[r][c] =
                                    curTile === 'dungeon.wall'
                                      ? 'dungeon.stairs_down'
                                      : curTile === 'dungeon.stairs_down'
                                      ? 'dungeon.floor'
                                      : 'dungeon.wall';
                                  onUpdateObject(selectedEntityIndex, {
                                    ...selectedObject,
                                    meta_tiles: newTiles,
                                  });
                                }}
                                style={{
                                  width: 18,
                                  height: 18,
                                  border: '1px solid rgba(245, 158, 11, 0.4)',
                                  borderRadius: 2,
                                  cursor: 'pointer',
                                  display: 'flex',
                                  alignItems: 'center',
                                  justifyContent: 'center',
                                  background: curTile.includes('wall') ? '#450a0a' : curTile.includes('stairs') ? '#7f1d1d' : '#1e1b4b',
                                }}
                              >
                                {tDef?.image_url ? (
                                  <img src={tDef.image_url} alt="" style={{ width: 14, height: 14, imageRendering: 'pixelated' }} />
                                ) : (
                                  <span style={{ fontSize: 8, color: '#fca5a5' }}>{curTile.slice(0, 2)}</span>
                                )}
                              </div>
                            );
                          })
                        )}
                      </div>
                    </div>
                  )}
                </div>

                {selectedObject.type === 'enemy' && (
                  <div className="form-group">
                    <label>AI Pattern</label>
                    <select
                      value={selectedObject.properties?.ai || 'AI_PATROL_CROSS'}
                      onChange={(e) =>
                        onUpdateObject(selectedEntityIndex, {
                          ...selectedObject,
                          properties: { ...selectedObject.properties, ai: e.target.value },
                        })
                      }
                    >
                      <option value="AI_NONE">AI_NONE</option>
                      <option value="AI_PATROL_CROSS">AI_PATROL_CROSS</option>
                      <option value="AI_PATROL_CIRCLE">AI_PATROL_CIRCLE</option>
                    </select>
                  </div>
                )}

                {selectedObject.type === 'npc' && (
                  <div className="form-group">
                    <label>Dialogue ID</label>
                    <input
                      type="text"
                      value={selectedObject.properties?.dialogue || ''}
                      onChange={(e) =>
                        onUpdateObject(selectedEntityIndex, {
                          ...selectedObject,
                          properties: { ...selectedObject.properties, dialogue: e.target.value },
                        })
                      }
                    />
                  </div>
                )}

                {/* Sprite/Tile Configuration */}
                <div className="inspector-section">
                  <h5>🎨 Sprite/Tile Configuration</h5>
                  
                  <div className="form-group">
                    <label>Overworld Sprite</label>
                    {(() => {
                      const currentVal = selectedObject.overworld_sprite || '';
                      const matched = allTilesWithScope.find(
                        (t) => t.scopedId === currentVal || t.id === currentVal
                      );
                      const selectVal = matched ? matched.scopedId : currentVal;

                      return (
                        <select
                          value={selectVal}
                          onChange={(e) =>
                            onUpdateObject(selectedEntityIndex, {
                              ...selectedObject,
                              overworld_sprite: e.target.value,
                            })
                          }
                        >
                          <option value="">-- None --</option>
                          {selectVal && !allTilesWithScope.some((t) => t.scopedId === selectVal) && (
                            <option value={selectVal}>{selectVal} (custom)</option>
                          )}
                          {Object.values(BUILTIN_TILESETS).map((ts) => (
                            <optgroup key={ts.id} label={ts.label}>
                              {ts.tiles.map((tile) => (
                                <option key={`${ts.id}.${tile.id}`} value={`${ts.id}.${tile.id}`}>
                                  {tile.label} ({tile.gb_constant})
                                </option>
                              ))}
                            </optgroup>
                          ))}
                        </select>
                      );
                    })()}
                  </div>

                  {/* Animation Frames (Multi-frame Sequence) */}
                  <div className="form-group animation-frames-group" style={{ marginTop: 8 }}>
                    <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: 6 }}>
                      <label style={{ margin: 0, fontWeight: 600 }}>🎞️ Animation Frames</label>
                      <div className="preset-buttons" style={{ display: 'flex', gap: 4 }}>
                        <button
                          type="button"
                          className="btn-tiny"
                          title="Set Fireplace Animation (2 frames)"
                          onClick={() => {
                            const newFrames = [
                              'desolate_landscape.desolate_fire_01',
                              'desolate_landscape.desolate_fire_02',
                            ];
                            onUpdateObject(selectedEntityIndex, {
                              ...selectedObject,
                              animation_frames: newFrames,
                              overworld_sprite: newFrames[0],
                            });
                          }}
                        >
                          🔥 Fire
                        </button>
                        <button
                          type="button"
                          className="btn-tiny"
                          title="Set Kobold Animation (2 frames)"
                          onClick={() => {
                            const newFrames = [
                              'desolate_landscape.desolate_kobold_01',
                              'desolate_landscape.desolate_kobold_02',
                            ];
                            onUpdateObject(selectedEntityIndex, {
                              ...selectedObject,
                              animation_frames: newFrames,
                              overworld_sprite: newFrames[0],
                            });
                          }}
                        >
                          👾 Kobold
                        </button>
                        <button
                          type="button"
                          className="btn-tiny"
                          title="Set Hero Animation (2 frames)"
                          onClick={() => {
                            const newFrames = [
                              'desolate_landscape.desolate_hero_01',
                              'desolate_landscape.desolate_hero_02',
                            ];
                            onUpdateObject(selectedEntityIndex, {
                              ...selectedObject,
                              animation_frames: newFrames,
                              overworld_sprite: newFrames[0],
                            });
                          }}
                        >
                          🧙 Hero
                        </button>
                      </div>
                    </div>

                    {/* Live Preview Box */}
                    {selectedObject.animation_frames && selectedObject.animation_frames.length > 0 && (
                      <div className="anim-preview-box">
                        {(() => {
                          const curKey = selectedObject.animation_frames[previewTick % selectedObject.animation_frames.length];
                          const t = allTilesWithScope.find((tile) => tile.scopedId === curKey || tile.id === curKey);
                          return (
                            <>
                              {t?.image_url ? (
                                <img src={t.image_url} alt="" style={{ width: 28, height: 28, imageRendering: 'pixelated' }} />
                              ) : (
                                <span style={{ fontSize: 22 }}>👾</span>
                              )}
                              <span style={{ fontSize: 12, color: '#e6edf3' }}>
                                Playing frame {(previewTick % selectedObject.animation_frames.length) + 1} of {selectedObject.animation_frames.length} ({t?.label || curKey})
                              </span>
                            </>
                          );
                        })()}
                      </div>
                    )}

                    {/* Frames list */}
                    {selectedObject.animation_frames && selectedObject.animation_frames.length > 0 ? (
                      <div className="frames-list" style={{ display: 'flex', flexDirection: 'column', gap: 4, marginBottom: 8 }}>
                        {selectedObject.animation_frames.map((frameId, fIdx) => {
                          const t = allTilesWithScope.find((tile) => tile.scopedId === frameId || tile.id === frameId);
                          return (
                            <div
                              key={fIdx}
                              className="frame-row"
                              style={{
                                display: 'flex',
                                alignItems: 'center',
                                gap: 8,
                                background: '#1c2430',
                                padding: '4px 8px',
                                borderRadius: 4,
                                border: '1px solid #2d3848',
                              }}
                            >
                              <span style={{ fontSize: 11, color: '#a0aec0', width: 45 }}>
                                #{fIdx + 1}
                              </span>
                              {t?.image_url && (
                                <img
                                  src={t.image_url}
                                  alt=""
                                  style={{ width: 20, height: 20, imageRendering: 'pixelated' }}
                                />
                              )}
                              <span style={{ flex: 1, fontSize: 12, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>
                                {t?.label || frameId}
                              </span>
                              {fIdx > 0 && (
                                <button
                                  type="button"
                                  className="btn-icon-tiny"
                                  title="Move Up"
                                  onClick={() => {
                                    const frames = [...selectedObject.animation_frames!];
                                    const tmp = frames[fIdx - 1];
                                    frames[fIdx - 1] = frames[fIdx];
                                    frames[fIdx] = tmp;
                                    onUpdateObject(selectedEntityIndex, {
                                      ...selectedObject,
                                      animation_frames: frames,
                                      overworld_sprite: frames[0],
                                    });
                                  }}
                                >
                                  ⬆️
                                </button>
                              )}
                              {selectedObject.animation_frames && fIdx < selectedObject.animation_frames.length - 1 && (
                                <button
                                  type="button"
                                  className="btn-icon-tiny"
                                  title="Move Down"
                                  onClick={() => {
                                    const frames = [...selectedObject.animation_frames!];
                                    const tmp = frames[fIdx + 1];
                                    frames[fIdx + 1] = frames[fIdx];
                                    frames[fIdx] = tmp;
                                    onUpdateObject(selectedEntityIndex, {
                                      ...selectedObject,
                                      animation_frames: frames,
                                      overworld_sprite: frames[0],
                                    });
                                  }}
                                >
                                  ⬇️
                                </button>
                              )}
                              <button
                                type="button"
                                className="btn-icon-tiny"
                                title="Remove Frame"
                                onClick={() => {
                                  const frames = selectedObject.animation_frames!.filter((_, i) => i !== fIdx);
                                  onUpdateObject(selectedEntityIndex, {
                                    ...selectedObject,
                                    animation_frames: frames.length > 0 ? frames : undefined,
                                    overworld_sprite: frames.length > 0 ? frames[0] : selectedObject.overworld_sprite,
                                  });
                                }}
                              >
                                ❌
                              </button>
                            </div>
                          );
                        })}
                      </div>
                    ) : (
                      <div style={{ fontSize: 12, color: '#718096', marginBottom: 8, fontStyle: 'italic' }}>
                        No animation frames set (using static overworld sprite).
                      </div>
                    )}

                    {/* Add frame selector */}
                    <div style={{ display: 'flex', gap: 6, alignItems: 'center' }}>
                      <select
                        id="new-object-frame-select"
                        style={{ flex: 1 }}
                        defaultValue=""
                      >
                        <option value="" disabled>-- Select tile to add as frame --</option>
                        {Object.values(BUILTIN_TILESETS).map((ts) => (
                          <optgroup key={ts.id} label={ts.label}>
                            {ts.tiles.map((tile) => (
                              <option key={`${ts.id}.${tile.id}`} value={`${ts.id}.${tile.id}`}>
                                {tile.label} ({tile.gb_constant})
                              </option>
                            ))}
                          </optgroup>
                        ))}
                      </select>
                      <button
                        type="button"
                        className="btn-secondary btn-sm"
                        onClick={() => {
                          const sel = document.getElementById('new-object-frame-select') as HTMLSelectElement;
                          if (!sel || !sel.value) return;
                          const chosen = sel.value;
                          const current = selectedObject.animation_frames || (selectedObject.overworld_sprite ? [selectedObject.overworld_sprite] : []);
                          const newFrames = [...current, chosen];
                          onUpdateObject(selectedEntityIndex, {
                            ...selectedObject,
                            animation_frames: newFrames,
                            overworld_sprite: newFrames[0],
                          });
                        }}
                      >
                        ➕ Add Frame
                      </button>
                    </div>

                    {/* Visual Quick-Picker Palette */}
                    <div style={{ marginTop: 8 }}>
                      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: 4 }}>
                        <label style={{ fontSize: 11, color: '#a0aec0', margin: 0 }}>
                          Quick Visual Tile Picker (Click to append):
                        </label>
                        {selectedObject.animation_frames && selectedObject.animation_frames.length > 0 && (
                          <button
                            type="button"
                            className="btn-tiny"
                            style={{ color: '#f85149' }}
                            title="Clear animation frames and revert to static sprite"
                            onClick={() =>
                              onUpdateObject(selectedEntityIndex, {
                                ...selectedObject,
                                animation_frames: undefined,
                              })
                            }
                          >
                            🗑️ Clear
                          </button>
                        )}
                      </div>
                      <div
                        style={{
                          display: 'grid',
                          gridTemplateColumns: 'repeat(auto-fill, minmax(34px, 1fr))',
                          gap: 4,
                          maxHeight: 120,
                          overflowY: 'auto',
                          background: '#141a22',
                          padding: 4,
                          borderRadius: 4,
                          border: '1px solid #2d3848',
                        }}
                      >
                        {allTilesWithScope
                          .filter(
                            (t) =>
                              t.category === 'npc' ||
                              t.category === 'enemy' ||
                              t.category === 'object' ||
                              t.tilesetId === level.tileset
                          )
                          .map((tile) => (
                            <button
                              key={tile.scopedId}
                              type="button"
                              title={`Click to add ${tile.label}`}
                              style={{
                                background: '#1c2430',
                                border: '1px solid #2d3848',
                                borderRadius: 4,
                                padding: 2,
                                cursor: 'pointer',
                                display: 'flex',
                                alignItems: 'center',
                                justifyContent: 'center',
                              }}
                              onClick={() => {
                                const current =
                                  selectedObject.animation_frames ||
                                  (selectedObject.overworld_sprite ? [selectedObject.overworld_sprite] : []);
                                const newFrames = [...current, tile.scopedId];
                                onUpdateObject(selectedEntityIndex, {
                                  ...selectedObject,
                                  animation_frames: newFrames,
                                  overworld_sprite: newFrames[0],
                                });
                              }}
                            >
                              <img
                                src={tile.image_url}
                                alt={tile.label}
                                width={26}
                                height={26}
                                style={{ imageRendering: 'pixelated' }}
                              />
                            </button>
                          ))}
                      </div>
                    </div>
                  </div>

                  <div className="form-group">
                    <label>Battle Sprite</label>
                    {(() => {
                      const currentVal = selectedObject.battle_sprite || '';
                      const matched = allTilesWithScope.find(
                        (t) => t.scopedId === currentVal || t.id === currentVal
                      );
                      const selectVal = matched ? matched.scopedId : currentVal;

                      return (
                        <select
                          value={selectVal}
                          onChange={(e) =>
                            onUpdateObject(selectedEntityIndex, {
                              ...selectedObject,
                              battle_sprite: e.target.value,
                            })
                          }
                        >
                          <option value="">-- None --</option>
                          {selectVal && !allTilesWithScope.some((t) => t.scopedId === selectVal) && (
                            <option value={selectVal}>{selectVal} (custom)</option>
                          )}
                          {Object.values(BUILTIN_TILESETS).map((ts) => {
                            const enemyTiles = ts.tiles.filter((t) => t.category === 'enemy');
                            if (enemyTiles.length === 0) return null;
                            return (
                              <optgroup key={ts.id} label={ts.label}>
                                {enemyTiles.map((tile) => (
                                  <option key={`${ts.id}.${tile.id}`} value={`${ts.id}.${tile.id}`}>
                                    {tile.label} ({tile.gb_constant})
                                  </option>
                                ))}
                              </optgroup>
                            );
                          })}
                        </select>
                      );
                    })()}
                  </div>

                  <div className="form-group">
                    <label>Battle Name</label>
                    <input
                      type="text"
                      value={selectedObject.battle_name || selectedObject.properties?.display_name || ''}
                      onChange={(e) =>
                        onUpdateObject(selectedEntityIndex, {
                          ...selectedObject,
                          battle_name: e.target.value,
                        })
                      }
                      placeholder="Name shown in battle UI"
                    />
                  </div>

                  <div className="form-row">
                    <div className="form-group">
                      <label>Sprite Width (tiles)</label>
                      <input
                        type="number"
                        min="1"
                        max="4"
                        value={selectedObject.properties?.sprite_width || 1}
                        onChange={(e) =>
                          onUpdateObject(selectedEntityIndex, {
                            ...selectedObject,
                            properties: { ...selectedObject.properties, sprite_width: parseInt(e.target.value) || 1 },
                          })
                        }
                      />
                    </div>
                    <div className="form-group">
                      <label>Sprite Height (tiles)</label>
                      <input
                        type="number"
                        min="1"
                        max="4"
                        value={selectedObject.properties?.sprite_height || 1}
                        onChange={(e) =>
                          onUpdateObject(selectedEntityIndex, {
                            ...selectedObject,
                            properties: { ...selectedObject.properties, sprite_height: parseInt(e.target.value) || 1 },
                          })
                        }
                      />
                    </div>
                  </div>

                  <div className="form-group">
                    <label>Battle ID</label>
                    <input
                      type="text"
                      value={selectedObject.properties?.battle || ''}
                      onChange={(e) =>
                        onUpdateObject(selectedEntityIndex, {
                          ...selectedObject,
                          properties: { ...selectedObject.properties, battle: e.target.value },
                        })
                      }
                      placeholder="e.g., BATTLE_SLIME, BATTLE_BOSS"
                    />
                  </div>
                </div>

                {selectedObject.type === 'npc' && (
                  <div className="form-group">
                    <label>Dialogue ID</label>
                    <input
                      type="text"
                      value={selectedObject.properties?.dialogue || ''}
                      onChange={(e) =>
                        onUpdateObject(selectedEntityIndex, {
                          ...selectedObject,
                          properties: { ...selectedObject.properties, dialogue: e.target.value },
                        })
                      }
                    />
                  </div>
                )}

                <div className="btn-group-row">
                  <button
                    className="btn btn-sm btn-danger"
                    onClick={() => onDeleteObject(selectedEntityIndex)}
                  >
                    🗑️ Delete
                  </button>
                  <button
                    className="btn btn-sm"
                    onClick={() => onSelectEntityIndex(null)}
                  >
                    Done
                  </button>
                </div>
              </div>
            ) : (
              <div className="entity-list">
                {level.objects.map((obj, idx) => (
                  <div
                    key={idx}
                    className="entity-list-item"
                    onClick={() => onSelectEntityIndex(idx)}
                  >
                    <span className="item-title">
                      👾 <strong>{obj.id}</strong> ({obj.type})
                    </span>
                    <span className="item-sub">pos ({obj.position.x},{obj.position.y})</span>
                  </div>
                ))}
              </div>
            )}
          </div>
        )}

        {tab === 'context' && activeLayer === 'regions' && (
          <div className="inspector-section">
            <div className="section-header-row">
              <h4>Regions ({level.regions.length})</h4>
              <button
                className="btn btn-sm btn-primary"
                onClick={() => {
                  onAddRegion({
                    id: `region_${Date.now().toString().slice(-4)}`,
                    bounds: { x: 2, y: 2, width: 6, height: 6 },
                    description: 'A newly defined semantic zone.',
                    gameplay: { purpose: 'exploration', difficulty: 1 },
                  });
                  onSelectEntityIndex(level.regions.length);
                }}
              >
                ➕ Add Region
              </button>
            </div>

            {selectedRegion && selectedEntityIndex !== null ? (
              <div className="entity-editor-box">
                <h5>Edit Region</h5>
                <div className="form-group">
                  <label>Region ID</label>
                  <input
                    type="text"
                    value={selectedRegion.id}
                    onChange={(e) =>
                      onUpdateRegion(selectedEntityIndex, { ...selectedRegion, id: e.target.value })
                    }
                  />
                </div>

                <div className="form-row">
                  <div className="form-group">
                    <label>X</label>
                    <input
                      type="number"
                      value={selectedRegion.bounds.x}
                      onChange={(e) =>
                        onUpdateRegion(selectedEntityIndex, {
                          ...selectedRegion,
                          bounds: { ...selectedRegion.bounds, x: parseInt(e.target.value) || 0 },
                        })
                      }
                    />
                  </div>
                  <div className="form-group">
                    <label>Y</label>
                    <input
                      type="number"
                      value={selectedRegion.bounds.y}
                      onChange={(e) =>
                        onUpdateRegion(selectedEntityIndex, {
                          ...selectedRegion,
                          bounds: { ...selectedRegion.bounds, y: parseInt(e.target.value) || 0 },
                        })
                      }
                    />
                  </div>
                  <div className="form-group">
                    <label>Width</label>
                    <input
                      type="number"
                      value={selectedRegion.bounds.width}
                      onChange={(e) =>
                        onUpdateRegion(selectedEntityIndex, {
                          ...selectedRegion,
                          bounds: { ...selectedRegion.bounds, width: parseInt(e.target.value) || 1 },
                        })
                      }
                    />
                  </div>
                  <div className="form-group">
                    <label>Height</label>
                    <input
                      type="number"
                      value={selectedRegion.bounds.height}
                      onChange={(e) =>
                        onUpdateRegion(selectedEntityIndex, {
                          ...selectedRegion,
                          bounds: { ...selectedRegion.bounds, height: parseInt(e.target.value) || 1 },
                        })
                      }
                    />
                  </div>
                </div>

                <div className="form-group">
                  <label>Semantic Description (LLM)</label>
                  <textarea
                    rows={3}
                    value={selectedRegion.description || ''}
                    onChange={(e) =>
                      onUpdateRegion(selectedEntityIndex, {
                        ...selectedRegion,
                        description: e.target.value,
                      })
                    }
                  />
                </div>

                <div className="form-row">
                  <div className="form-group">
                    <label>Purpose</label>
                    <input
                      type="text"
                      value={selectedRegion.gameplay?.purpose || 'exploration'}
                      onChange={(e) =>
                        onUpdateRegion(selectedEntityIndex, {
                          ...selectedRegion,
                          gameplay: { ...selectedRegion.gameplay, purpose: e.target.value },
                        })
                      }
                    />
                  </div>
                  <div className="form-group">
                    <label>Difficulty</label>
                    <input
                      type="number"
                      min="0"
                      max="10"
                      value={selectedRegion.gameplay?.difficulty || 1}
                      onChange={(e) =>
                        onUpdateRegion(selectedEntityIndex, {
                          ...selectedRegion,
                          gameplay: { ...selectedRegion.gameplay, difficulty: parseInt(e.target.value) || 1 },
                        })
                      }
                    />
                  </div>
                </div>

                <div className="btn-group-row">
                  <button
                    className="btn btn-sm btn-danger"
                    onClick={() => onDeleteRegion(selectedEntityIndex)}
                  >
                    🗑️ Delete
                  </button>
                  <button
                    className="btn btn-sm"
                    onClick={() => onSelectEntityIndex(null)}
                  >
                    Done
                  </button>
                </div>
              </div>
            ) : (
              <div className="entity-list">
                {level.regions.map((reg, idx) => (
                  <div
                    key={idx}
                    className="entity-list-item"
                    onClick={() => onSelectEntityIndex(idx)}
                  >
                    <span className="item-title">
                      🏷️ <strong>{reg.id}</strong> ({reg.gameplay?.purpose || 'zone'})
                    </span>
                    <span className="item-sub">
                      [{reg.bounds.x},{reg.bounds.y}] {reg.bounds.width}×{reg.bounds.height}
                    </span>
                  </div>
                ))}
              </div>
            )}
          </div>
        )}
      </div>
    </div>
  );
};

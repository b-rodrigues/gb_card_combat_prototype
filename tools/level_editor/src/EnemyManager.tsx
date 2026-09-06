import React, { useEffect, useState } from 'react';
import {
  EnemyTypeListItem, CombatArtListItem,
  fetchEnemyTypeList, fetchEnemyType, saveEnemyType,
  fetchCombatArtList,
} from './io/combatArt';
import { BUILTIN_TILESETS } from './model/Tileset';

/** Enemies view (art-only): one record per enemy type.  The level view
 *  owns placement; this view owns looks.  Per type it edits the shared
 *  overworld sprite (transparent, every world) and shows the battle-art
 *  assignment (edited in the Combat Art Studio).  Selections apply
 *  everywhere the type appears: the compilers resolve placed enemies to
 *  their type row by the ENTITY_ID_X naming convention. */

interface OwDraft {
  cells: string[];
  palette: number;
}

export const EnemyManager: React.FC<{ onOpenComposer: () => void; initialId?: string }> = ({ onOpenComposer, initialId }) => {
  const [enemies, setEnemies] = useState<EnemyTypeListItem[]>([]);
  const [sets, setSets] = useState<CombatArtListItem[]>([]);
  const [activeId, setActiveId] = useState<string>('');
  const [full, setFull] = useState<any>(null);
  const [ow, setOw] = useState<OwDraft | null>(null);
  const [dirty, setDirty] = useState(false);
  const [status, setStatus] = useState('');
  const [enemyRows, setEnemyRows] = useState<Record<string, { art: string; frames: number; category: string; dirty: boolean }>>({});

  const owTiles = (BUILTIN_TILESETS.enemies?.tiles || []).filter((t) => t.category === 'enemy');
  const tileUrl = (id: string) => {
    const t = owTiles.find((x) => x.id === id);
    return t ? t.image_url : `/tiles/enemies/${id}.png`;
  };

  useEffect(() => {
    fetchEnemyTypeList().then((items) => {
      setEnemies(items);
      if (items.length > 0) setActiveId((prev) => prev || initialId || items[0].id);
    }).catch((e) => setStatus(`load failed: ${e.message}`));
    fetchCombatArtList().then(setSets).catch(() => undefined);
  }, []);

  useEffect(() => {
    if (!activeId) { setFull(null); return; }
    fetchEnemyType(activeId).then((data) => {
      setFull(data);
      const o = data.overworld || null;
      setOw(o ? { cells: (o.cells || []).slice(0, 2), palette: o.palette || 0 } : null);
      setDirty(false);
      setStatus('');
    }).catch((e) => setStatus(`load failed: ${e.message}`));
  }, [activeId]);

  const active = enemies.find((e) => e.id === activeId);
  const battleSet = sets.find((s) => s.id === ((full && full.sprite && full.sprite.art) || ''));

  const save = async () => {
    if (!full) return;
    try {
      const data = JSON.parse(JSON.stringify(full));
      if (ow && ow.cells.length > 0) {
        data.overworld = { cells: ow.cells.slice(0, 2), palette: ow.palette };
      } else {
        delete data.overworld;
      }
      await saveEnemyType(activeId, data);
      setFull(data);
      setDirty(false);
      setStatus(`saved screens/enemy_types/${activeId}.json — run make screens + make gfx to recompile`);
      fetchEnemyTypeList().then(setEnemies).catch(() => undefined);
    } catch (e: any) {
      setStatus(`save failed: ${e.message}`);
    }
  };

  const saveEnemyRow = async (id: string) => {
    const row = enemyRows[id];
    if (!row) return;
    try {
      const full = await fetchEnemyType(id);
      if (row.art) full.sprite = { art: row.art, frames: row.frames };
      else full.sprite = null;
      full.category = row.category;
      await saveEnemyType(id, full);
      setEnemyRows((prev) => ({ ...prev, [id]: { ...row, dirty: false } }));
      setStatus(`saved screens/enemy_types/${id}.json — run make screens to recompile`);
    } catch (e: any) {
      setStatus(`save failed: ${e.message}`);
    }
  };

  const setCell = (idx: number, id: string) => {
    setOw((prev) => {
      const cells = ((prev && prev.cells) || []).slice();
      cells[idx] = id;
      return { cells, palette: (prev && prev.palette) || 0 };
    });
    setDirty(true);
  };

  const setFrames = (n: number) => {
    n = Math.max(1, Math.min(2, n));
    setOw((prev) => {
      const cells = ((prev && prev.cells) || []).slice(0, n);
      while (cells.length < n) cells.push((prev && prev.cells[0]) || (owTiles[0] && owTiles[0].id) || '');
      return { cells, palette: (prev && prev.palette) || 0 };
    });
    setDirty(true);
  };

  return (
    <div style={{ display: 'flex', gap: 16, padding: 16, height: '100%', overflow: 'auto', background: '#f4efe4', color: '#222' }}>
      <div style={{ minWidth: 190 }}>
        <div style={{ fontWeight: 'bold', marginBottom: 4 }}>Enemies (by type)</div>
        {enemies.map((e) => (
          <div key={e.id}>
            <div style={{ display: 'flex', alignItems: 'center', gap: 4 }}>
              <button style={{ width: '100%', fontWeight: e.id === activeId ? 'bold' : 'normal', textAlign: 'left', background: 'none', border: 'none', color: 'inherit', cursor: 'pointer', padding: 2 }} onClick={() => setActiveId(e.id)}>
                {e.label}
              </button>
              <select
                value={enemyRows[e.id]?.category ?? e.category}
                onChange={(ev) => {
                  const category = ev.target.value as 'minion' | 'elite' | 'boss';
                  setEnemyRows((prev) => ({ ...prev, [e.id]: { art: prev[e.id]?.art ?? e.art ?? '', frames: prev[e.id]?.frames ?? 1, category, dirty: true } }));
                }}
                style={{ fontSize: 10, padding: '1px 4px', background: '#fff', border: '1px solid #ccc', borderRadius: 3 }}
              >
                <option value="minion">Minion</option>
                <option value="elite">Elite</option>
                <option value="boss">Boss</option>
              </select>
            </div>
          </div>
        ))}
        <div style={{ fontSize: 12, color: '#555', marginTop: 8 }}>
          New types are raw JSON for now: add <code>{'screens/enemy_types/<id>.json'}</code> and it appears here.
        </div>
      </div>

      {full && active && (
        <div style={{ display: 'flex', gap: 24 }}>
          <div>
            <h3 style={{ margin: '0 0 4px' }}>{active.label} — overworld sprite</h3>
            <div style={{ fontSize: 12, color: '#555', marginBottom: 8 }}>
              One transparent-background sprite shared by every world. Applies everywhere a {active.id} is placed.
            </div>
            {(ow ? ow.cells : []).map((cell, i) => (
              <div key={i} style={{ display: 'flex', alignItems: 'center', gap: 8, marginBottom: 6 }}>
                <img src={tileUrl(cell)} alt={cell} width={32} height={32} style={{ imageRendering: 'pixelated', background: '#888' }} />
                <label>Frame {i}:{' '}
                  <select value={cell} onChange={(e) => setCell(i, e.target.value)}>
                    {owTiles.map((t) => <option key={t.id} value={t.id}>{t.label}</option>)}
                  </select>
                </label>
              </div>
            ))}
            <div style={{ marginTop: 6, fontSize: 13 }}>
              Frames: <button onClick={() => setFrames(((ow && ow.cells.length) || 1) - 1)}>-</button>{' '}
              {(ow && ow.cells.length) || 0}{' '}
              <button onClick={() => setFrames(((ow && ow.cells.length) || 0) + 1)}>+</button>{' '}
              (max 2)
            </div>
            <div style={{ marginTop: 6, fontSize: 13 }}>
              <label>Palette:{' '}
                <input type="number" min={0} max={7} style={{ width: 50 }}
                  value={(ow && ow.palette) || 0}
                  onChange={(e) => {
                    const palette = Math.max(0, Math.min(7, parseInt(e.target.value) || 0));
                    setOw((prev) => ({ cells: ((prev && prev.cells) || []).slice(), palette }));
                    setDirty(true);
                  }} />
              </label>
            </div>
            {!ow && (
              <button style={{ marginTop: 8 }} onClick={() => { setOw({ cells: [owTiles[0] ? owTiles[0].id : ''], palette: 0 }); setDirty(true); }}>
                Add shared sprite
              </button>
            )}
            {ow && (
              <button style={{ marginTop: 8 }} onClick={() => { setOw(null); setDirty(true); }}>
                Remove (legacy kinds)
              </button>
            )}
          </div>

          <div>
            <h3 style={{ margin: '0 0 4px' }}>Battle tiles</h3>
            <div style={{ fontSize: 13 }}>
              {battleSet ? `${battleSet.label} (${battleSet.width}x${battleSet.height})` : 'text fallback (none)'}
            </div>
            <div style={{ fontSize: 12, color: '#555', margin: '4px 0 8px' }}>
              Battle art is assigned in the Combat Art Studio (metatiles supported).
            </div>
            <button onClick={onOpenComposer}>Open Combat Art Studio</button>
          </div>

          <div>
            <h3 style={{ margin: '0 0 4px' }}>Category</h3>
            <div style={{ fontSize: 13 }}>
              <select
                value={active.category}
                onChange={async (ev) => {
                  const category = ev.target.value as 'minion' | 'elite' | 'boss';
                  setEnemyRows((prev) => ({ ...prev, [active.id]: { art: prev[active.id]?.art ?? active.art ?? '', frames: prev[active.id]?.frames ?? 1, category, dirty: true } }));
                  try {
                    const full = await fetchEnemyType(active.id);
                    full.category = category;
                    await saveEnemyType(active.id, full);
                    setEnemyRows((prev) => ({ ...prev, [active.id]: { art: prev[active.id]?.art ?? active.art ?? '', frames: prev[active.id]?.frames ?? 1, category, dirty: false } }));
                    setStatus(`saved screens/enemy_types/${active.id}.json — run make screens to recompile`);
                  } catch (err: any) {
                    setStatus(`save failed: ${err.message}`);
                  }
                }}
              >
                <option value="minion">Minion</option>
                <option value="elite">Elite</option>
                <option value="boss">Boss</option>
              </select>
            </div>
            <div style={{ fontSize: 12, color: '#555', margin: '4px 0 8px' }}>
              Category determines battle screen filtering (screens/battle/*.json allowed_categories).
            </div>
            <button onClick={save} disabled={!dirty}>Save enemy{dirty ? ' *' : ''}</button>
            {status && <div style={{ marginTop: 8, fontSize: 13 }}>{status}</div>}
          </div>
        </div>
      )}
    </div>
  );
};
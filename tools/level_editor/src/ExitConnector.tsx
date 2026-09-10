import React, { useEffect, useState } from 'react';
import { LevelExit } from './model/Level';
import { ExitStatus, fetchExitStatus, connectLevels } from './io/exits';

/** Auto-exit helper for the Exits tab: shows, for each exit in the level,
 *  whether the target scene has a return exit, and lets the author preview
 *  + create the missing one.  Placement is computed on the server (one
 *  source of truth) and written to the target level; the from-level's exit
 *  is upserted too, so the pair can never drift.
 *
 *  This is what keeps every level reachable at scale — the walkthrough
 *  sweep fails on an orphan, and this fixes it in one click. */
export const ExitConnector: React.FC<{
  levelId: string;
  exits: LevelExit[];
  onChanged?: () => void;
}> = ({ levelId, exits, onChanged }) => {
  const [items, setItems] = useState<ExitStatus[]>([]);
  const [open, setOpen] = useState(false);
  const [busy, setBusy] = useState(false);
  const [status, setStatus] = useState('');

  const refresh = () => {
    setBusy(true);
    fetchExitStatus(levelId, exits)
      .then(setItems)
      .catch((e) => setStatus(`check failed: ${e.message}`))
      .finally(() => setBusy(false));
  };

  useEffect(() => {
    if (open) refresh();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [open, levelId, exits.length]);

  const missing = items.filter((it) => !it.has_return && it.proposal);

  const create = async (it: ExitStatus) => {
    setBusy(true);
    setStatus('');
    try {
      const res = await connectLevels(levelId, exits[it.index]);
      setStatus(res.created
        ? `created return in ${it.target}: gate (${res.to_exit.x},${res.to_exit.y}) ${res.to_exit.direction}`
        : `return already existed in ${it.target}`);
      refresh();
      onChanged?.();
    } catch (e: any) {
      setStatus(`create failed: ${e.message}`);
    } finally {
      setBusy(false);
    }
  };

  return (
    <div style={{ marginTop: 10, borderTop: '1px solid #555', paddingTop: 8 }}>
      <div className="section-header-row">
        <h5 style={{ margin: 0 }}>🔁 Return exits</h5>
        <button className="btn btn-sm" onClick={() => setOpen((o) => !o)}>
          {open ? 'Hide' : 'Check'}
        </button>
      </div>
      {open && (
        <div style={{ fontSize: 12, marginTop: 6 }}>
          {busy && <div style={{ opacity: 0.7 }}>Checking…</div>}
          {!busy && items.length === 0 && <div style={{ opacity: 0.7 }}>No exits yet.</div>}
          {items.map((it) => (
            <div key={it.index} style={{ display: 'flex', gap: 8, alignItems: 'center', marginBottom: 4 }}>
              <span style={{ minWidth: 28 }}>#{it.index + 1}</span>
              <span style={{ minWidth: 110 }}>→ <strong>{it.target}</strong></span>
              {it.error ? (
                <span style={{ color: '#a60' }}>⚠ {it.error}</span>
              ) : it.has_return ? (
                <span style={{ color: '#393' }}>✓ return exists</span>
              ) : (
                <>
                  <span style={{ color: '#a60' }}>
                    ⚠ none — would add gate ({it.proposal!.x},{it.proposal!.y}) {it.proposal!.direction}
                    , spawn ({it.proposal!.target_x},{it.proposal!.target_y})
                  </span>
                  <button className="btn btn-sm btn-primary" disabled={busy} onClick={() => create(it)}>
                    Create
                  </button>
                </>
              )}
            </div>
          ))}
          {missing.length > 1 && (
            <div style={{ marginTop: 6 }}>
              <button
                className="btn btn-sm"
                disabled={busy}
                onClick={async () => { for (const it of missing) await create(it); }}
              >
                Create all missing ({missing.length})
              </button>
            </div>
          )}
          {status && <div style={{ marginTop: 6, color: '#555' }}>{status}</div>}
          <div style={{ marginTop: 6, color: '#777', lineHeight: 1.4 }}>
            Creates the opposite-side gate in the target scene so every level
            stays reachable. Recompile to apply.
          </div>
        </div>
      )}
    </div>
  );
};

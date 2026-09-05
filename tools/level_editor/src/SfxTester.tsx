import React, { useRef, useState, useEffect } from 'react';

// One-shot SFX rendered by tools/render_sfx_preview.py
// (make sfx-preview) into tools/level_editor/public/audio/sfx/.
// Order and ids match the SFX_* enum in src/audio/audio.h.
const SFX_LIST: { id: string; file: string; blurb: string }[] = [
  { id: 'CURSOR', file: '/audio/sfx/cursor.wav', blurb: 'menu navigation blip' },
  { id: 'CONFIRM', file: '/audio/sfx/confirm.wav', blurb: 'selection confirm' },
  { id: 'SELECT', file: '/audio/sfx/select.wav', blurb: 'battle/card select' },
  { id: 'BACK', file: '/audio/sfx/back.wav', blurb: 'cancel / go back' },
  { id: 'ATTACK', file: '/audio/sfx/attack.wav', blurb: 'player slash (noise)' },
  { id: 'HIT', file: '/audio/sfx/hit.wav', blurb: 'enemy strike (noise)' },
  { id: 'BLOCK', file: '/audio/sfx/block.wav', blurb: 'successful defend' },
];

export const SfxTesterModal: React.FC<{ onClose: () => void }> = ({ onClose }) => {
  const audioRef = useRef<HTMLAudioElement | null>(null);
  const [playingId, setPlayingId] = useState<string | null>(null);

  useEffect(() => {
    const audio = audioRef.current;
    return () => {
      if (audio) audio.pause();
    };
  }, []);

  const play = (id: string, file: string) => {
    const audio = audioRef.current;
    if (!audio) return;
    audio.src = file;
    audio.play().then(() => setPlayingId(id)).catch(() => setPlayingId(null));
  };

  return (
    <div className="modal-overlay" onClick={onClose}>
      <div className="modal-card" onClick={(e) => e.stopPropagation()}>
        <div className="modal-header">
          <h3>🔊 Sound Test</h3>
          <button className="modal-close-btn" onClick={onClose}>
            ✕
          </button>
        </div>
        <div className="modal-body">
          <div style={{ marginBottom: '12px', fontSize: '13px', opacity: 0.85 }}>
            Rendered previews of the transcribed tracker SFX
            (<code>tools/render_sfx_preview.py</code>, <code>make sfx-preview</code>).
            The ROM synth mix stays authoritative.
          </div>
          {SFX_LIST.map((s) => (
            <div
              key={s.id}
              style={{ display: 'flex', gap: '10px', alignItems: 'center', marginBottom: '8px' }}
            >
              <button
                className="btn btn-sm"
                onClick={() => play(s.id, s.file)}
                title={`Play ${s.id}`}
              >
                {playingId === s.id ? '🔊 Playing…' : '▶ Play'}
              </button>
              <code style={{ minWidth: '90px' }}>{s.id}</code>
              <span style={{ fontSize: '13px', opacity: 0.8 }}>{s.blurb}</span>
            </div>
          ))}
          <audio
            ref={audioRef}
            preload="none"
            onEnded={() => setPlayingId(null)}
            onPause={() => setPlayingId(null)}
          />
        </div>
        <div className="modal-footer">
          <button className="btn btn-primary" onClick={onClose}>
            Close
          </button>
        </div>
      </div>
    </div>
  );
};

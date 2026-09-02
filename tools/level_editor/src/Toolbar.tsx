import React from 'react';

export type ToolType = 'brush' | 'eraser' | 'rect' | 'fill' | 'eyedropper' | 'select';

interface ToolbarProps {
  activeTool: ToolType;
  onSelectTool: (tool: ToolType) => void;
  canUndo: boolean;
  canRedo: boolean;
  onUndo: () => void;
  onRedo: () => void;
  zoom: number;
  onZoomChange: (newZoom: number) => void;
  showGrid: boolean;
  onToggleGrid: () => void;
  showCollision: boolean;
  onToggleCollision: () => void;
  onSave: () => void;
  onLoad: () => void;
  onValidate: () => void;
  onDescribe: () => void;
  onNew: () => void;
}

export const Toolbar: React.FC<ToolbarProps> = ({
  activeTool,
  onSelectTool,
  canUndo,
  canRedo,
  onUndo,
  onRedo,
  zoom,
  onZoomChange,
  showGrid,
  onToggleGrid,
  showCollision,
  onToggleCollision,
  onSave,
  onLoad,
  onValidate,
  onDescribe,
  onNew,
}) => {
  return (
    <div className="toolbar">
      <div className="tool-group">
        <button className="btn btn-primary" onClick={onNew} title="New Level">
          ➕ New
        </button>
        <button className="btn" onClick={onLoad} title="Load JSON File">
          📂 Load
        </button>
        <button className="btn btn-success" onClick={onSave} title="Save / Download JSON (Ctrl+S)">
          💾 Save
        </button>
        <button className="btn" onClick={onValidate} title="Validate Level">
          ✓ Validate
        </button>
        <button className="btn btn-accent" onClick={onDescribe} title="Generate LLM Semantic Description">
          🤖 LLM View
        </button>
      </div>

      <div className="divider" />

      <div className="tool-group">
        <button
          className={`btn-tool ${activeTool === 'brush' ? 'active' : ''}`}
          onClick={() => onSelectTool('brush')}
          title="Brush / Paint (B)"
        >
          ✏️ Brush <span className="shortcut">B</span>
        </button>
        <button
          className={`btn-tool ${activeTool === 'eraser' ? 'active' : ''}`}
          onClick={() => onSelectTool('eraser')}
          title="Eraser (E)"
        >
          🧹 Erase <span className="shortcut">E</span>
        </button>
        <button
          className={`btn-tool ${activeTool === 'rect' ? 'active' : ''}`}
          onClick={() => onSelectTool('rect')}
          title="Rectangle (R)"
        >
          ⬜ Rect <span className="shortcut">R</span>
        </button>
        <button
          className={`btn-tool ${activeTool === 'fill' ? 'active' : ''}`}
          onClick={() => onSelectTool('fill')}
          title="Flood Fill (G)"
        >
          🪣 Fill <span className="shortcut">G</span>
        </button>
        <button
          className={`btn-tool ${activeTool === 'eyedropper' ? 'active' : ''}`}
          onClick={() => onSelectTool('eyedropper')}
          title="Eyedropper / Pick (I)"
        >
          💉 Pick <span className="shortcut">I</span>
        </button>
        <button
          className={`btn-tool ${activeTool === 'select' ? 'active' : ''}`}
          onClick={() => onSelectTool('select')}
          title="Select / Move Entity (V)"
        >
          👆 Select <span className="shortcut">V</span>
        </button>
      </div>

      <div className="divider" />

      <div className="tool-group">
        <button className="btn-icon" onClick={onUndo} disabled={!canUndo} title="Undo (Ctrl+Z)">
          ↩️
        </button>
        <button className="btn-icon" onClick={onRedo} disabled={!canRedo} title="Redo (Ctrl+Shift+Z)">
          ↪️
        </button>
      </div>

      <div className="divider" />

      <div className="tool-group">
        <button
          className={`btn-toggle ${showGrid ? 'active' : ''}`}
          onClick={onToggleGrid}
          title="Toggle Grid Overlay"
        >
          Grid {showGrid ? '✓' : '✗'}
        </button>
        <button
          className={`btn-toggle ${showCollision ? 'active' : ''}`}
          onClick={onToggleCollision}
          title="Toggle Collision Walkability Overlay"
        >
          Collision {showCollision ? '✓' : '✗'}
        </button>
      </div>

      <div className="divider" />

      <div className="tool-group zoom-group">
        <button
          className="btn-icon"
          onClick={() => {
            const ZOOM_LEVELS = [0.25, 0.5, 1, 2, 3, 4, 5];
            const prev = [...ZOOM_LEVELS].reverse().find((z) => z < zoom - 0.001);
            if (prev !== undefined) onZoomChange(prev);
          }}
          disabled={zoom <= 0.25}
          title="Zoom Out (Down to 25%)"
        >
          ➖
        </button>
        <span
          className="zoom-text"
          onClick={() => onZoomChange(1)}
          title="Click to reset zoom to 100%"
          style={{ cursor: 'pointer' }}
        >
          {Math.round(zoom * 100)}%
        </span>
        <button
          className="btn-icon"
          onClick={() => {
            const ZOOM_LEVELS = [0.25, 0.5, 1, 2, 3, 4, 5];
            const next = ZOOM_LEVELS.find((z) => z > zoom + 0.001);
            if (next !== undefined) onZoomChange(next);
          }}
          disabled={zoom >= 5}
          title="Zoom In (Up to 500%)"
        >
          ➕
        </button>
      </div>
    </div>
  );
};

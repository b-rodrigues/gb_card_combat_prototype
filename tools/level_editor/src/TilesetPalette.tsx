import React from 'react';
import { BUILTIN_TILESETS, TileDefinition, TilesetDefinition } from './model/Tileset';

interface TilesetPaletteProps {
  tilesetId: string;
  selectedTileId: string;
  onSelectTileset: (tilesetId: string) => void;
  onSelectTile: (tileId: string) => void;
}

export const TilesetPalette: React.FC<TilesetPaletteProps> = ({
  tilesetId,
  selectedTileId,
  onSelectTileset,
  onSelectTile,
}) => {
  const currentTileset: TilesetDefinition = BUILTIN_TILESETS[tilesetId] || BUILTIN_TILESETS.forest;

  return (
    <div className="panel tileset-panel">
      <div className="panel-header">
        <h3>Tileset Palette</h3>
        <select
          className="select-input"
          value={tilesetId}
          onChange={(e) => onSelectTileset(e.target.value)}
        >
          {Object.values(BUILTIN_TILESETS).map((ts) => (
            <option key={ts.id} value={ts.id}>
              {ts.label}
            </option>
          ))}
        </select>
      </div>

      <div className="panel-body">
        <div className="tiles-grid">
          {currentTileset.tiles.map((tile: TileDefinition) => {
            const isSelected = selectedTileId === tile.id;
            return (
              <button
                key={tile.id}
                className={`tile-btn ${isSelected ? 'selected' : ''}`}
                onClick={() => onSelectTile(tile.id)}
                title={`${tile.label} (${tile.gb_constant}) - ${tile.walkable ? 'Walkable' : 'Solid Wall'}`}
              >
                <div
                  className="tile-swatch"
                  style={{
                    backgroundColor: tile.color,
                    borderColor: isSelected ? '#ffffff' : '#000000',
                  }}
                >
                  <span className="tile-ascii">{tile.ascii || '.'}</span>
                </div>
                <div className="tile-info">
                  <span className="tile-name">{tile.label}</span>
                  <span className={`tile-tag ${tile.walkable ? 'walkable' : 'blocked'}`}>
                    {tile.walkable ? 'Walk' : 'Solid'}
                  </span>
                </div>
              </button>
            );
          })}
        </div>
      </div>
    </div>
  );
};

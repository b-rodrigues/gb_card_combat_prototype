import React from 'react';
import { BUILTIN_TILESETS, TileDefinition, TilesetDefinition } from './model/Tileset';

interface TilesetPaletteProps {
  tilesetId: string;
  selectedTileId: string;
  onSelectTileset: (tilesetId: string) => void;
  onSelectTile: (tileId: string) => void;
  categoryFilter?: 'enemy' | 'npc' | 'terrain' | 'ui' | 'object' | 'all';
}

export const TilesetPalette: React.FC<TilesetPaletteProps> = ({
  tilesetId,
  selectedTileId,
  onSelectTileset,
  onSelectTile,
  categoryFilter = 'all',
}) => {
  const currentTileset: TilesetDefinition = BUILTIN_TILESETS[tilesetId] || BUILTIN_TILESETS.forest;
  if (!BUILTIN_TILESETS[tilesetId]) {
    console.warn(`[TilesetPalette] unknown tileset '${tilesetId}', falling back to forest`);
  }

  const filteredTiles = currentTileset.tiles.filter((tile: TileDefinition) => {
    if (categoryFilter === 'all') return true;
    return tile.category === categoryFilter;
  });

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
          {filteredTiles.map((tile: TileDefinition) => {
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
                    borderColor: isSelected ? '#ffffff' : '#000000',
                  }}
                >
                  <img
                    src={tile.image_url}
                    alt={tile.label}
                    width={32}
                    height={32}
                    style={{
                      imageRendering: 'pixelated',
                      display: 'block',
                    }}
                    onError={(e: React.SyntheticEvent<HTMLImageElement>) => {
                      const img = e.currentTarget;
                      img.style.display = 'none';
                      const fallback = img.nextElementSibling as HTMLElement | null;
                      if (fallback) fallback.style.display = 'flex';
                    }}
                  />
                  <div
                    className="tile-ascii-fallback"
                    style={{
                      display: 'none',
                      width: '32px',
                      height: '32px',
                      backgroundColor: tile.color,
                      alignItems: 'center',
                      justifyContent: 'center',
                      fontSize: '24px',
                    }}
                  >
                    {tile.ascii || '.'}
                  </div>
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
import React, { useRef, useEffect, useState, useCallback, useMemo } from 'react';
import { EditorLevel, LevelExit, LevelRegion } from './model/Level';
import { LevelObject, OBJECT_TEMPLATES } from './model/Objects';
import { BUILTIN_TILESETS, TileDefinition, TilesetDefinition } from './model/Tileset';
import { ToolType } from './Toolbar';
import { EditLayer } from './LayerPanel';

interface MapCanvasProps {
  level: EditorLevel;
  activeTool: ToolType;
  activeLayer: EditLayer;
  selectedTileId: string;
  zoom: number;
  showGrid: boolean;
  showCollision: boolean;
  showTerrain: boolean;
  showExits: boolean;
  showObjects: boolean;
  showRegions: boolean;
  selectedEntityIndex: number | null;
  onSelectEntityIndex: (index: number | null) => void;
  onTilePainted: (x: number, y: number, tileId: string) => void;
  onRectPainted: (x: number, y: number, w: number, h: number, tileId: string) => void;
  onFill: (startX: number, startY: number, newTileId: string) => void;
  onTilePicked: (tileId: string) => void;
  onSpawnMoved: (x: number, y: number) => void;
  onObjectMoved: (index: number, x: number, y: number) => void;
  onExitMoved: (index: number, x: number, y: number) => void;
  onAddObjectAt: (x: number, y: number) => void;
  onAddExitAt: (x: number, y: number) => void;
}

const TILE_SIZE = 24; // Base pixel size per tile

export const MapCanvas: React.FC<MapCanvasProps> = ({
  level,
  activeTool,
  activeLayer,
  selectedTileId,
  zoom,
  showGrid,
  showCollision,
  showTerrain,
  showExits,
  showObjects,
  showRegions,
  selectedEntityIndex,
  onSelectEntityIndex,
  onTilePainted,
  onRectPainted,
  onFill,
  onTilePicked,
  onSpawnMoved,
  onObjectMoved,
  onExitMoved,
  onAddObjectAt,
  onAddExitAt,
}) => {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const [isMouseDown, setIsMouseDown] = useState(false);
  const [dragStartTile, setDragStartTile] = useState<{ x: number; y: number } | null>(null);
  const [hoverTile, setHoverTile] = useState<{ x: number; y: number } | null>(null);
  const [draggingEntity, setDraggingEntity] = useState<{ type: 'object' | 'exit' | 'spawn'; index: number } | null>(null);

  const tileset: TilesetDefinition = BUILTIN_TILESETS[level.tileset] || BUILTIN_TILESETS.forest;
  const tileMap = new Map<string, TileDefinition>();
  tileset.tiles.forEach((t) => tileMap.set(t.id, t));

  const tileSize = TILE_SIZE * zoom;
  const canvasWidth = level.width * tileSize;
  const canvasHeight = level.height * tileSize;

  // Pre-load tile images
  const [tileImages, setTileImages] = useState<Map<string, HTMLImageElement>>(new Map());
  useEffect(() => {
    const imgMap = new Map<string, HTMLImageElement>();
    let loadedCount = 0;
    tileset.tiles.forEach((t) => {
      const img = new Image();
      img.src = t.image_url;
      img.onload = () => {
        loadedCount++;
        if (loadedCount === tileset.tiles.length) {
          imgMap.set(t.id, img);
          setTileImages(new Map(imgMap));
        }
      };
      img.onerror = () => {
        loadedCount++;
        if (loadedCount === tileset.tiles.length) {
          setTileImages(new Map(imgMap));
        }
      };
      imgMap.set(t.id, img);
    });
  }, [tileset]);

  // Convert mouse pixel coordinates to tile coordinates
  const getTileCoords = (e: React.MouseEvent<HTMLCanvasElement>): { x: number; y: number } | null => {
    const canvas = canvasRef.current;
    if (!canvas) return null;
    const rect = canvas.getBoundingClientRect();
    const px = e.clientX - rect.left;
    const py = e.clientY - rect.top;

    const tx = Math.floor(px / tileSize);
    const ty = Math.floor(py / tileSize);

    if (tx < 0 || tx >= level.width || ty < 0 || ty >= level.height) {
      return null;
    }
    return { x: tx, y: ty };
  };

  // Render function
  const render = useCallback(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    // Clear canvas
    ctx.fillStyle = '#0f141c';
    ctx.fillRect(0, 0, canvasWidth, canvasHeight);

    // 1. Render Terrain Tiles
    if (showTerrain) {
      for (let y = 0; y < level.height; y++) {
        for (let x = 0; x < level.width; x++) {
          const tileId = level.grid[y]?.[x] || 'floor';
          const tDef = tileMap.get(tileId) || tileMap.get('floor');

          const px = x * tileSize;
          const py = y * tileSize;

          // Draw tile image if loaded, fallback to colored rect
          const img = tileImages.get(tileId);
          if (img && img.complete && img.naturalWidth > 0) {
            ctx.imageSmoothingEnabled = false;
            ctx.drawImage(img, px, py, tileSize, tileSize);
          } else {
            const color = tDef ? tDef.color : '#88c070';
            ctx.fillStyle = color;
            ctx.fillRect(px, py, tileSize, tileSize);
          }

          // Perimeter wall highlight
          if (x === 0 || x === level.width - 1 || y === 0 || y === level.height - 1) {
            ctx.fillStyle = 'rgba(0, 0, 0, 0.25)';
            ctx.fillRect(px, py, tileSize, tileSize);
          }
        }
      }
    }

    // 2. Render Rectangle Drag Preview
    if (isMouseDown && activeTool === 'rect' && dragStartTile && hoverTile) {
      const rx = Math.min(dragStartTile.x, hoverTile.x);
      const ry = Math.min(dragStartTile.y, hoverTile.y);
      const rw = Math.abs(dragStartTile.x - hoverTile.x) + 1;
      const rh = Math.abs(dragStartTile.y - hoverTile.y) + 1;

      const tDef = tileMap.get(selectedTileId);
      const img = tileImages.get(selectedTileId);
      ctx.globalAlpha = 0.6;
      if (img && img.complete && img.naturalWidth > 0) {
        ctx.imageSmoothingEnabled = false;
        for (let ty = ry; ty < ry + rh; ty++) {
          for (let tx = rx; tx < rx + rw; tx++) {
            ctx.drawImage(img, tx * tileSize, ty * tileSize, tileSize, tileSize);
          }
        }
      } else {
        ctx.fillStyle = tDef?.color || '#ffffff';
        ctx.fillRect(rx * tileSize, ry * tileSize, rw * tileSize, rh * tileSize);
      }
      ctx.strokeStyle = '#ffffff';
      ctx.lineWidth = 2;
      ctx.strokeRect(rx * tileSize, ry * tileSize, rw * tileSize, rh * tileSize);
      ctx.globalAlpha = 1.0;
    }

    // 3. Render Collision Walkability Overlay
    if (showCollision) {
      for (let y = 0; y < level.height; y++) {
        for (let x = 0; x < level.width; x++) {
          const tileId = level.grid[y]?.[x] || 'floor';
          const tDef = tileMap.get(tileId);
          const isPerimeter = x === 0 || x === level.width - 1 || y === 0 || y === level.height - 1;
          const isBlocked = isPerimeter || (tDef ? !tDef.walkable : false);

          if (isBlocked) {
            const px = x * tileSize;
            const py = y * tileSize;

            ctx.fillStyle = 'rgba(231, 76, 60, 0.4)';
            ctx.fillRect(px, py, tileSize, tileSize);

            // Red diagonal hatch
            ctx.strokeStyle = 'rgba(231, 76, 60, 0.7)';
            ctx.lineWidth = 1.5;
            ctx.beginPath();
            ctx.moveTo(px, py);
            ctx.lineTo(px + tileSize, py + tileSize);
            ctx.stroke();
          }
        }
      }
    }

    // 4. Render Grid Lines
    if (showGrid && tileSize >= 8) {
      ctx.strokeStyle = 'rgba(255, 255, 255, 0.12)';
      ctx.lineWidth = 1;

      ctx.beginPath();
      for (let x = 0; x <= level.width; x++) {
        ctx.moveTo(x * tileSize, 0);
        ctx.lineTo(x * tileSize, canvasHeight);
      }
      for (let y = 0; y <= level.height; y++) {
        ctx.moveTo(0, y * tileSize);
        ctx.lineTo(canvasWidth, y * tileSize);
      }
      ctx.stroke();
    }

    // 5. Render Regions
    if (showRegions && level.regions) {
      level.regions.forEach((reg, idx) => {
        const isSelected = activeLayer === 'regions' && selectedEntityIndex === idx;
        const b = reg.bounds;
        const rx = b.x * tileSize;
        const ry = b.y * tileSize;
        const rw = b.width * tileSize;
        const rh = b.height * tileSize;

        ctx.fillStyle = isSelected ? 'rgba(52, 152, 219, 0.25)' : 'rgba(52, 152, 219, 0.12)';
        ctx.fillRect(rx, ry, rw, rh);

        ctx.strokeStyle = isSelected ? '#3498db' : 'rgba(52, 152, 219, 0.6)';
        ctx.lineWidth = isSelected ? 2.5 : 1.5;
        ctx.setLineDash([4, 4]);
        ctx.strokeRect(rx, ry, rw, rh);
        ctx.setLineDash([]);

        // Region label
        if (tileSize >= 16) {
          ctx.fillStyle = '#3498db';
          ctx.font = `bold ${Math.max(10, Math.floor(tileSize * 0.4))}px Inter, sans-serif`;
          ctx.textAlign = 'left';
          ctx.textBaseline = 'top';
          ctx.fillText(`🏷️ ${reg.id}`, rx + 4, ry + 4);
        }
      });
    }

    // 6. Render Exits
    if (showExits && level.exits) {
      level.exits.forEach((ex, idx) => {
        const isSelected = activeLayer === 'exits' && selectedEntityIndex === idx;
        const px = ex.x * tileSize;
        const py = ex.y * tileSize;

        const m = tileSize >= 16 ? 2 : (tileSize >= 8 ? 1 : 0);
        const s = Math.max(2, tileSize - m * 2);

        ctx.fillStyle = isSelected ? '#f39c12' : '#e67e22';
        ctx.fillRect(px + m, py + m, s, s);

        ctx.strokeStyle = isSelected ? '#ffffff' : '#d35400';
        ctx.lineWidth = isSelected ? 2 : 1;
        ctx.strokeRect(px + m, py + m, s, s);

        // Exit target label / arrow
        if (tileSize >= 16) {
          ctx.fillStyle = '#ffffff';
          ctx.font = `bold ${Math.floor(tileSize * 0.55)}px sans-serif`;
          ctx.textAlign = 'center';
          ctx.textBaseline = 'middle';
          const glyph = ex.direction === 'NORTH' ? '⬆️' : ex.direction === 'SOUTH' ? '⬇️' : ex.direction === 'WEST' ? '⬅️' : '➡️';
          ctx.fillText(glyph, px + tileSize / 2, py + tileSize / 2);
        }
      });
    }

    // 7. Render Objects / NPCs / Enemies
    if (showObjects && level.objects) {
      level.objects.forEach((obj, idx) => {
        const isSelected = activeLayer === 'objects' && selectedEntityIndex === idx;
        const px = obj.position.x * tileSize;
        const py = obj.position.y * tileSize;

        const tmpl = OBJECT_TEMPLATES.find((t) => t.type === obj.type);
        const color = tmpl ? tmpl.color : '#9b59b6';

        // Draw entity circle / box
        ctx.fillStyle = color;
        ctx.beginPath();
        ctx.arc(px + tileSize / 2, py + tileSize / 2, Math.max(1.5, tileSize * 0.38), 0, Math.PI * 2);
        ctx.fill();

        ctx.strokeStyle = isSelected ? '#ffffff' : 'rgba(0,0,0,0.5)';
        ctx.lineWidth = isSelected ? 2 : 1;
        ctx.stroke();

        // Sprite image or fallback icon
        if (tileSize >= 16) {
          const spriteId = obj.overworld_sprite
            ? obj.overworld_sprite.includes('.')
              ? obj.overworld_sprite.split('.')[1]
              : obj.overworld_sprite
            : null;
          const spriteImg = spriteId ? tileImages.get(spriteId) : null;
          if (spriteImg && spriteImg.complete && spriteImg.naturalWidth > 0) {
            ctx.imageSmoothingEnabled = false;
            ctx.drawImage(spriteImg, px + 2, py + 2, tileSize - 4, tileSize - 4);
          } else {
            ctx.font = `${Math.floor(tileSize * 0.45)}px sans-serif`;
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.fillText(tmpl?.icon || '👾', px + tileSize / 2, py + tileSize / 2);
          }
        }
      });
    }

    // 8. Render Player Spawn
    if (level.spawn) {
      const sp = level.spawn;
      const px = sp.x * tileSize;
      const py = sp.y * tileSize;
      const isSelected = activeLayer === 'spawn';

      ctx.fillStyle = '#2ecc71';
      ctx.beginPath();
      ctx.arc(px + tileSize / 2, py + tileSize / 2, Math.max(2, tileSize * 0.42), 0, Math.PI * 2);
      ctx.fill();

      ctx.strokeStyle = isSelected ? '#ffffff' : '#27ae60';
      ctx.lineWidth = isSelected ? 2 : 1;
      ctx.stroke();

      if (tileSize >= 16) {
        ctx.font = `bold ${Math.floor(tileSize * 0.45)}px sans-serif`;
        ctx.fillStyle = '#ffffff';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillText('🧙', px + tileSize / 2, py + tileSize / 2);
      }
    }

    // 9. Render Hover Cursor
    if (hoverTile) {
      const hpx = hoverTile.x * tileSize;
      const hpy = hoverTile.y * tileSize;

      ctx.strokeStyle = 'rgba(255, 255, 255, 0.8)';
      ctx.lineWidth = 2;
      ctx.strokeRect(hpx, hpy, tileSize, tileSize);
    }
  }, [
    level,
    activeTool,
    activeLayer,
    selectedTileId,
    zoom,
    showGrid,
    showCollision,
    showTerrain,
    showExits,
    showObjects,
    showRegions,
    selectedEntityIndex,
    isMouseDown,
    dragStartTile,
    hoverTile,
    canvasWidth,
    canvasHeight,
    tileSize,
    tileMap,
    tileImages,
  ]);

  useEffect(() => {
    render();
  }, [render]);

  // Mouse Handlers
  const handleMouseDown = (e: React.MouseEvent<HTMLCanvasElement>) => {
    const coords = getTileCoords(e);
    if (!coords) return;

    setIsMouseDown(true);
    setDragStartTile(coords);

    // Spawn mode
    if (activeLayer === 'spawn') {
      onSpawnMoved(coords.x, coords.y);
      return;
    }

    // Select / Move Mode
    if (activeTool === 'select' || activeLayer === 'objects' || activeLayer === 'exits') {
      // Check if clicked an object
      const objIndex = level.objects.findIndex(
        (o) => o.position.x === coords.x && o.position.y === coords.y
      );
      if (objIndex !== -1) {
        onSelectEntityIndex(objIndex);
        setDraggingEntity({ type: 'object', index: objIndex });
        return;
      }

      // Check if clicked an exit
      const exitIndex = level.exits.findIndex((ex) => ex.x === coords.x && ex.y === coords.y);
      if (exitIndex !== -1) {
        onSelectEntityIndex(exitIndex);
        setDraggingEntity({ type: 'exit', index: exitIndex });
        return;
      }

      // Check if clicked player spawn
      if (level.spawn.x === coords.x && level.spawn.y === coords.y) {
        setDraggingEntity({ type: 'spawn', index: 0 });
        return;
      }

      // Check if clicked a region
      const regIndex = level.regions.findIndex(
        (r) =>
          coords.x >= r.bounds.x &&
          coords.x < r.bounds.x + r.bounds.width &&
          coords.y >= r.bounds.y &&
          coords.y < r.bounds.y + r.bounds.height
      );
      if (regIndex !== -1) {
        onSelectEntityIndex(regIndex);
        return;
      }

      onSelectEntityIndex(null);
    }

    // Drawing Tools
    if (activeLayer === 'terrain') {
      if (activeTool === 'brush') {
        onTilePainted(coords.x, coords.y, selectedTileId);
      } else if (activeTool === 'eraser') {
        onTilePainted(coords.x, coords.y, 'floor');
      } else if (activeTool === 'fill') {
        onFill(coords.x, coords.y, selectedTileId);
      } else if (activeTool === 'eyedropper') {
        const picked = level.grid[coords.y]?.[coords.x] || 'floor';
        onTilePicked(picked);
      }
    }
  };

  const handleMouseMove = (e: React.MouseEvent<HTMLCanvasElement>) => {
    const coords = getTileCoords(e);
    setHoverTile(coords);

    if (!isMouseDown || !coords) return;

    // Entity dragging
    if (draggingEntity) {
      if (draggingEntity.type === 'object') {
        onObjectMoved(draggingEntity.index, coords.x, coords.y);
      } else if (draggingEntity.type === 'exit') {
        onExitMoved(draggingEntity.index, coords.x, coords.y);
      } else if (draggingEntity.type === 'spawn') {
        onSpawnMoved(coords.x, coords.y);
      }
      return;
    }

    // Continuous brush painting
    if (activeLayer === 'terrain') {
      if (activeTool === 'brush') {
        onTilePainted(coords.x, coords.y, selectedTileId);
      } else if (activeTool === 'eraser') {
        onTilePainted(coords.x, coords.y, 'floor');
      }
    }
  };

  const handleMouseUp = () => {
    if (isMouseDown && activeTool === 'rect' && dragStartTile && hoverTile && activeLayer === 'terrain') {
      const rx = Math.min(dragStartTile.x, hoverTile.x);
      const ry = Math.min(dragStartTile.y, hoverTile.y);
      const rw = Math.abs(dragStartTile.x - hoverTile.x) + 1;
      const rh = Math.abs(dragStartTile.y - hoverTile.y) + 1;
      onRectPainted(rx, ry, rw, rh, selectedTileId);
    }

    setIsMouseDown(false);
    setDragStartTile(null);
    setDraggingEntity(null);
  };

  return (
    <div className="map-canvas-container">
      <div className="canvas-wrapper">
        <canvas
          ref={canvasRef}
          width={canvasWidth}
          height={canvasHeight}
          className={`map-canvas tool-${activeTool}`}
          onMouseDown={handleMouseDown}
          onMouseMove={handleMouseMove}
          onMouseUp={handleMouseUp}
          onMouseLeave={() => {
            setIsMouseDown(false);
            setHoverTile(null);
            setDraggingEntity(null);
          }}
        />
      </div>
      <div className="canvas-footer-status">
        <span>
          Cursor: {hoverTile ? `(${hoverTile.x}, ${hoverTile.y})` : '—'}
        </span>
        <span>
          Tile: {hoverTile ? level.grid[hoverTile.y]?.[hoverTile.x] || 'floor' : '—'}
        </span>
        <span>
          Dimensions: {level.width} × {level.height}
        </span>
        <span>
          Zoom: {Math.round(zoom * 100)}%
        </span>
      </div>
    </div>
  );
};

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
  const defaultFloorTile = tileset.tiles.find((t) => t.walkable)?.id || tileset.tiles[0]?.id || 'floor';
  const tileMap = new Map<string, TileDefinition>();
  tileset.tiles.forEach((t) => tileMap.set(t.id, t));

  const tileSize = TILE_SIZE * zoom;
  const canvasWidth = level.width * tileSize;
  const canvasHeight = level.height * tileSize;

  // Real-time animation clock for animated actors / objects (4 fps)
  const [animTick, setAnimTick] = useState<number>(0);
  useEffect(() => {
    const timer = setInterval(() => {
      setAnimTick((t) => (t + 1) % 120);
    }, 250);
    return () => clearInterval(timer);
  }, []);

  // Pre-load tile images for all tiles across all tilesets
  const [tileImages, setTileImages] = useState<Map<string, HTMLImageElement>>(new Map());
  useEffect(() => {
    const imgMap = new Map<string, HTMLImageElement>();
    const allTiles = Object.values(BUILTIN_TILESETS).flatMap((ts) => ts.tiles);
    let loadedCount = 0;
    allTiles.forEach((t) => {
      const img = new Image();
      img.src = t.image_url;
      img.onload = () => {
        loadedCount++;
        imgMap.set(t.id, img);
        if (loadedCount === allTiles.length) {
          setTileImages(new Map(imgMap));
        }
      };
      img.onerror = () => {
        loadedCount++;
        if (loadedCount === allTiles.length) {
          setTileImages(new Map(imgMap));
        }
      };
      imgMap.set(t.id, img);
    });
    setTileImages(new Map(imgMap));
  }, []);

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

    // ── TITLE SCREEN AUTHENTIC RENDERER ──
    if (level.isScreen && (level.mapId === 'SCREEN_TITLE' || level.id === 'title')) {
      const titleLayout = level.titleLayout || {};
      const logo = titleLayout.logo || { x: 0, y: 1, lines: [] };
      const graphic = titleLayout.graphic || { enabled: true, x: 2, y: 7, width: 16, height: 5, lines: [] };
      const prompt = titleLayout.prompt || { text: 'PRESS START', x: 4, y: 14, align: 'center' };
      const credits = titleLayout.credits || { enabled: true, text: 'GAME BY BRODRIGUES', x: 2, y: 17, align: 'right' };

      // 1. Dark fantasy / classic Game Boy title screen background
      ctx.fillStyle = '#0f172a';
      ctx.fillRect(0, 0, canvasWidth, canvasHeight);

      // 2. Cyan grid lines if enabled
      if (showGrid && tileSize >= 6) {
        ctx.strokeStyle = 'rgba(74, 185, 209, 0.25)';
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

      // 3. Logo Lines (rows logo.y + idx)
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      if (logo.lines && logo.lines.length > 0) {
        logo.lines.forEach((line: string, idx: number) => {
          const rowY = (logo.y + idx) * tileSize + tileSize * 0.5;
          if (idx === 0) {
            ctx.fillStyle = '#f6d365';
            ctx.font = `bold ${Math.max(12, Math.floor(tileSize * 0.85))}px monospace`;
            ctx.fillText(line.trim(), canvasWidth / 2, rowY);
          } else if (idx === 1) {
            ctx.fillStyle = '#64748b';
            ctx.font = `bold ${Math.max(10, Math.floor(tileSize * 0.7))}px monospace`;
            ctx.fillText(line.trim(), canvasWidth / 2, rowY);
          } else {
            ctx.fillStyle = '#cbd5e1';
            ctx.font = `${Math.max(10, Math.floor(tileSize * 0.65))}px monospace`;
            ctx.fillText(line.trim(), canvasWidth / 2, rowY);
          }
        });
      }

      // 4. Big Title Graphic / Artwork (rows graphic.y + idx)
      if (graphic.enabled && graphic.lines && graphic.lines.length > 0) {
        const gx = graphic.x * tileSize;
        const gy = graphic.y * tileSize;
        const gw = (graphic.width || 16) * tileSize;
        const gh = (graphic.height || graphic.lines.length) * tileSize;

        ctx.fillStyle = 'rgba(30, 41, 59, 0.75)';
        ctx.strokeStyle = '#38bdf8';
        ctx.lineWidth = 1.5;
        ctx.fillRect(gx, gy, gw, gh);
        ctx.strokeRect(gx, gy, gw, gh);

        ctx.font = `bold ${Math.max(9, Math.floor(tileSize * 0.65))}px monospace`;
        ctx.fillStyle = '#38bdf8';
        ctx.textAlign = 'left';
        graphic.lines.forEach((line: string, idx: number) => {
          const lineY = gy + (idx + 0.5) * tileSize;
          ctx.fillText(line, gx + tileSize * 0.5, lineY);
        });
      }

      // 5. Centered "PRESS START" Prompt
      if (prompt.text) {
        const promptY = (prompt.y ?? 14) * tileSize + tileSize * 0.5;
        const isBlink = animTick % 2 === 0;
        ctx.fillStyle = isBlink ? '#ffffff' : '#94a3b8';
        ctx.font = `bold ${Math.max(11, Math.floor(tileSize * 0.75))}px monospace`;
        ctx.textAlign = prompt.align === 'left' ? 'left' : prompt.align === 'right' ? 'right' : 'center';
        const promptX =
          prompt.align === 'left'
            ? (prompt.x ?? 2) * tileSize
            : prompt.align === 'right'
            ? ((prompt.x ?? 18) + 1) * tileSize
            : canvasWidth / 2;
        ctx.fillText(prompt.text, promptX, promptY);
      }

      // 6. Bottom Row Credits (e.g. Row 17, right-aligned)
      if (credits.enabled && credits.text) {
        const credY = (credits.y ?? 17) * tileSize + tileSize * 0.5;
        ctx.fillStyle = '#64748b';
        ctx.font = `bold ${Math.max(8, Math.floor(tileSize * 0.55))}px monospace`;
        ctx.textAlign = credits.align === 'left' ? 'left' : credits.align === 'center' ? 'center' : 'right';
        const credX =
          credits.align === 'left'
            ? (credits.x ?? 0) * tileSize + tileSize * 0.2
            : credits.align === 'center'
            ? canvasWidth / 2
            : canvasWidth - tileSize * 0.5;
        ctx.fillText(credits.text, credX, credY);
      }

      return;
    }

    // ── BATTLE SCREEN AUTHENTIC RENDERER (matching assets/battle_screen_mockup.jpg) ──
    if (level.isScreen && (level.mapId === 'SCREEN_BATTLE' || level.id.includes('battle'))) {
      const layout = level.battleHudLayout || {};
      const bannerRow = layout.turn_banner_row ?? 0;
      const enemyHpRow = layout.enemy_hp_row ?? 1;
      const enemySpriteRow = layout.enemy_sprite_row ?? 2;
      const enemyCursorRow = layout.enemy_cursor_row ?? 4;
      const heroLabelRow = layout.hero_label_row ?? 6;
      const heroLabelCol = layout.hero_label_col ?? 1;
      const heroHpRow = layout.hero_hp_row ?? 6;
      const heroHpCol = layout.hero_hp_col ?? 13;
      const deckRow = layout.deck_row ?? 7;
      const deckCol = layout.deck_col ?? 1;
      const apRow = layout.ap_row ?? 7;
      const apCol = layout.ap_col ?? 13;
      const comboRow = layout.combo_row ?? 9;
      const cardsRow = layout.cards_row ?? 10;
      const cardCursorRow = layout.card_cursor_row ?? 14;
      const cardDescRow = layout.card_desc_row ?? 15;
      const timerRow = layout.timer_row ?? 16;

      // 1. Crisp white background
      ctx.fillStyle = '#ffffff';
      ctx.fillRect(0, 0, canvasWidth, canvasHeight);

      // 2. Mockup Cyan Grid lines
      if (showGrid && tileSize >= 6) {
        ctx.strokeStyle = 'rgba(74, 185, 209, 0.45)';
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

      const fontScale = Math.max(10, Math.floor(tileSize * 0.72));
      ctx.font = `bold ${fontScale}px monospace`;
      ctx.fillStyle = '#593c28';

      // 3. Row 0: Top Banner "PLAYER TURN"
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillText(
        'PLAYER TURN',
        canvasWidth / 2,
        bannerRow * tileSize + tileSize * 0.5
      );

      // 4. Enemy Roster (Rows 1–4)
      const enemyCols = [
        layout.enemy_col_start ?? 1,
        (layout.enemy_col_start ?? 1) + (layout.enemy_col_step ?? 7),
        (layout.enemy_col_start ?? 1) + (layout.enemy_col_step ?? 7) * 2,
      ];
      const enemyHps = ['10/10', '10/10', '02/10'];

      enemyCols.forEach((colX, idx) => {
        const cx = (colX + 1.5) * tileSize;
        // Enemy HP Text above sprite
        ctx.fillStyle = '#593c28';
        ctx.font = `bold ${Math.max(9, Math.floor(tileSize * 0.65))}px monospace`;
        ctx.textAlign = 'center';
        ctx.fillText(enemyHps[idx], cx, (enemyHpRow + 0.5) * tileSize);

        // Slime Sprite (Rows 2–3)
        const sx = (colX + 0.3) * tileSize;
        const sy = (enemySpriteRow + 0.1) * tileSize;
        const sw = 2.4 * tileSize;
        const sh = 1.8 * tileSize;

        // Wobble on animTick
        const wobble = (animTick % 2 === idx % 2) ? 1 : 0;

        // Slime body
        ctx.fillStyle = '#72b847';
        ctx.beginPath();
        ctx.ellipse(cx, sy + sh * 0.55 + wobble, sw * 0.48, sh * 0.44, 0, 0, Math.PI * 2);
        ctx.fill();
        ctx.strokeStyle = '#4e852d';
        ctx.lineWidth = 1.5;
        ctx.stroke();

        // Eyes
        ctx.fillStyle = '#ffffff';
        ctx.fillRect(cx - sw * 0.22, sy + sh * 0.35 + wobble, Math.max(3, tileSize * 0.22), Math.max(3, tileSize * 0.22));
        ctx.fillRect(cx + sw * 0.12, sy + sh * 0.35 + wobble, Math.max(3, tileSize * 0.22), Math.max(3, tileSize * 0.22));

        // Pupils
        ctx.fillStyle = '#000000';
        ctx.fillRect(cx - sw * 0.18, sy + sh * 0.38 + wobble, Math.max(2, tileSize * 0.12), Math.max(2, tileSize * 0.12));
        ctx.fillRect(cx + sw * 0.16, sy + sh * 0.38 + wobble, Math.max(2, tileSize * 0.12), Math.max(2, tileSize * 0.12));

        // Open mouth
        ctx.fillStyle = '#1c1c1c';
        ctx.beginPath();
        ctx.ellipse(cx, sy + sh * 0.65 + wobble, sw * 0.18, sh * 0.12, 0, 0, Math.PI * 2);
        ctx.fill();
      });

      // Target arrow under enemy 1 (middle)
      ctx.fillStyle = '#593c28';
      ctx.font = `bold ${Math.max(12, Math.floor(tileSize * 0.9))}px sans-serif`;
      ctx.textAlign = 'center';
      ctx.fillText('⬆', (enemyCols[1] + 1.5) * tileSize, (enemyCursorRow + 0.5) * tileSize);

      // 5. Dual-Column Hero & Status Panel (Rows 6 & 7)
      ctx.font = `bold ${fontScale}px monospace`;
      ctx.textAlign = 'left';
      ctx.fillStyle = '#593c28';

      // Row 6: Left "HERO" + Icon | Right "[♥] : 10/10"
      ctx.fillText('HERO 🧙', heroLabelCol * tileSize, (heroLabelRow + 0.5) * tileSize);
      ctx.fillStyle = '#c0392b';
      ctx.fillText('♥', heroHpCol * tileSize, (heroHpRow + 0.5) * tileSize);
      ctx.fillStyle = '#593c28';
      ctx.fillText(' : 10/10', (heroHpCol + 0.8) * tileSize, (heroHpRow + 0.5) * tileSize);

      // Row 7: Left "DECK:  7 [🎴]" | Right "[🔋] :  6 :  6"
      ctx.fillText('DECK:  7 🎴', deckCol * tileSize, (deckRow + 0.5) * tileSize);
      ctx.fillText('🔋 :  6 :  6', apCol * tileSize, (apRow + 0.5) * tileSize);

      // 6. Row 9: Combo Header "COMBO"
      ctx.fillText('COMBO', 1 * tileSize, (comboRow + 0.5) * tileSize);

      // 7. Rows 10–13: 5 Framed Multi-Tile Cards
      const cardCols = [1, 5, 8, 12, 16];
      const cardDefs = [
        { icon: '🗡️', val: 3, rider: null },
        { icon: '🏹', val: 2, rider: '🟣' },
        { icon: '🛡️', val: 2, rider: null },
        { icon: '🛡️', val: 2, rider: null },
        { icon: '🗡️', val: 4, rider: '🔥' },
      ];

      cardCols.forEach((cx, idx) => {
        const cDef = cardDefs[idx];
        const cardX = cx * tileSize;
        const cardY = cardsRow * tileSize;
        const cardW = 2.8 * tileSize;
        const cardH = 3.8 * tileSize;

        // Outer border
        ctx.fillStyle = '#b78e58';
        ctx.fillRect(cardX, cardY, cardW, cardH);

        // Inner parchment face
        ctx.fillStyle = '#deb580';
        ctx.fillRect(cardX + 2, cardY + 2, cardW - 4, cardH - 4);

        // Inset border line
        ctx.strokeStyle = '#cd9e64';
        ctx.lineWidth = 1;
        ctx.strokeRect(cardX + 4, cardY + 4, cardW - 8, cardH - 8);

        // Element rider in top right corner
        if (cDef.rider) {
          ctx.font = `${Math.max(8, Math.floor(tileSize * 0.55))}px sans-serif`;
          ctx.textAlign = 'right';
          ctx.fillText(cDef.rider, cardX + cardW - 5, cardY + tileSize * 0.7);
        }

        // Weapon icon in center
        ctx.font = `${Math.max(12, Math.floor(tileSize * 0.85))}px sans-serif`;
        ctx.textAlign = 'center';
        ctx.fillText(cDef.icon, cardX + cardW / 2, cardY + cardH * 0.42);

        // Card number value underneath weapon
        ctx.fillStyle = '#593c28';
        ctx.font = `bold ${Math.max(11, Math.floor(tileSize * 0.8))}px monospace`;
        ctx.fillText(String(cDef.val), cardX + cardW / 2, cardY + cardH * 0.78);
      });

      // 8. Row 14: Card Cursor
      ctx.fillStyle = '#593c28';
      ctx.font = `bold ${Math.max(12, Math.floor(tileSize * 0.9))}px sans-serif`;
      ctx.textAlign = 'center';
      ctx.fillText('⬆', (cardCols[0] + 1.4) * tileSize, (cardCursorRow + 0.5) * tileSize);

      // 9. Row 15: Card Description "Sword: physical"
      ctx.fillStyle = '#593c28';
      ctx.font = `bold ${fontScale}px monospace`;
      ctx.textAlign = 'left';
      ctx.fillText('Sword: physical', 1 * tileSize, (cardDescRow + 0.5) * tileSize);

      // 10. Rows 16–17: Turn Timer Bar
      const timerWidthCols = layout.timer_width ?? 11;
      const barX = (layout.timer_col ?? 0) * tileSize;
      const barY = timerRow * tileSize;
      const barW = timerWidthCols * tileSize;
      const barH = 1.9 * tileSize;

      ctx.fillStyle = '#d59f63';
      ctx.fillRect(barX, barY, barW, barH);
      ctx.strokeStyle = '#b78e58';
      ctx.lineWidth = 1.5;
      ctx.strokeRect(barX, barY, barW, barH);

      // Hover cursor
      if (hoverTile) {
        ctx.strokeStyle = 'rgba(231, 76, 60, 0.8)';
        ctx.lineWidth = 2;
        ctx.strokeRect(hoverTile.x * tileSize, hoverTile.y * tileSize, tileSize, tileSize);
      }

      return;
    }

    // 1. Render Terrain Tiles
    if (showTerrain) {
      for (let y = 0; y < level.height; y++) {
        for (let x = 0; x < level.width; x++) {
          const tileId = level.grid[y]?.[x] || defaultFloorTile;
          const tDef = tileMap.get(tileId) || tileMap.get(defaultFloorTile);

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
          const tileId = level.grid[y]?.[x] || defaultFloorTile;
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

        // Sprite image (animated or static) or fallback icon
        if (tileSize >= 16) {
          let spriteId: string | null = null;
          if (obj.animation_frames && obj.animation_frames.length > 0) {
            const frameKey = obj.animation_frames[animTick % obj.animation_frames.length];
            spriteId = frameKey.includes('.') ? frameKey.split('.')[1] : frameKey;
          } else if (obj.overworld_sprite) {
            spriteId = obj.overworld_sprite.includes('.')
              ? obj.overworld_sprite.split('.')[1]
              : obj.overworld_sprite;
          }

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
        let heroSpriteId: string | null = null;
        if (sp.animation_frames && sp.animation_frames.length > 0) {
          const frameKey = sp.animation_frames[animTick % sp.animation_frames.length];
          heroSpriteId = frameKey.includes('.') ? frameKey.split('.')[1] : frameKey;
        }
        const heroImg = heroSpriteId ? tileImages.get(heroSpriteId) : null;
        if (heroImg && heroImg.complete && heroImg.naturalWidth > 0) {
          ctx.imageSmoothingEnabled = false;
          ctx.drawImage(heroImg, px + 2, py + 2, tileSize - 4, tileSize - 4);
        } else {
          ctx.font = `bold ${Math.floor(tileSize * 0.45)}px sans-serif`;
          ctx.fillStyle = '#ffffff';
          ctx.textAlign = 'center';
          ctx.textBaseline = 'middle';
          ctx.fillText('🧙', px + tileSize / 2, py + tileSize / 2);
        }
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
    animTick,
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
        onTilePainted(coords.x, coords.y, defaultFloorTile);
      } else if (activeTool === 'fill') {
        onFill(coords.x, coords.y, selectedTileId);
      } else if (activeTool === 'eyedropper') {
        const picked = level.grid[coords.y]?.[coords.x] || defaultFloorTile;
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
        onTilePainted(coords.x, coords.y, defaultFloorTile);
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
          Tile: {hoverTile ? level.grid[hoverTile.y]?.[hoverTile.x] || defaultFloorTile : '—'}
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

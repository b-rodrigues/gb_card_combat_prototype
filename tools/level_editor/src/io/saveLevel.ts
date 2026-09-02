import { EditorLevel, editorToLevelData } from '../model/Level';

export function serializeLevelJson(level: EditorLevel): string {
  const data = editorToLevelData(level);
  return JSON.stringify(data, null, 2);
}

export async function saveLevelToServer(level: EditorLevel): Promise<{ success: boolean; path?: string; error?: string }> {
  try {
    const data = editorToLevelData(level);
    const category = level.isScreen ? 'screens' : 'levels';
    const res = await fetch('/api/save-level', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ id: level.id, category, data })
    });
    if (!res.ok) {
      throw new Error(`Server returned status ${res.status}`);
    }
    return await res.json();
  } catch (err: any) {
    return { success: false, error: err.message };
  }
}

export async function compileRom(): Promise<{ success: boolean; log?: string; romPath?: string; error?: string }> {
  try {
    const res = await fetch('/api/compile-rom', { method: 'POST' });
    const data = await res.json();
    if (!res.ok || !data.success) {
      throw new Error(data.error || `Compile failed with status ${res.status}`);
    }
    return data;
  } catch (err: any) {
    return { success: false, error: err.message };
  }
}

export async function runGame(): Promise<{ success: boolean; emulator?: string; message?: string; error?: string }> {
  try {
    const res = await fetch('/api/run-game', { method: 'POST' });
    const data = await res.json();
    if (!res.ok || !data.success) {
      throw new Error(data.error || `Run failed with status ${res.status}`);
    }
    return data;
  } catch (err: any) {
    return { success: false, error: err.message };
  }
}

export function downloadLevelJson(level: EditorLevel): void {
  const jsonStr = serializeLevelJson(level);
  const blob = new Blob([jsonStr], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = `${level.id || 'level'}.json`;
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  URL.revokeObjectURL(url);
}

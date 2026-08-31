import { EditorLevel, editorToLevelData } from '../model/Level';

export function serializeLevelJson(level: EditorLevel): string {
  const data = editorToLevelData(level);
  return JSON.stringify(data, null, 2);
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

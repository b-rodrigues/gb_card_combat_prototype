/** Auto-exit disk I/O via the vite dev API.
 *
 *  A hand-authored exit is one-directional; a level with no incoming exit
 *  is unreachable (the walkthrough sweep fails loudly on that).  These
 *  calls compute the reciprocal exit for a level's exits (`exit-status`)
 *  and write it into the target (`connect-levels`) with one click.
 */

import { LevelExit } from '../model/Level';

export interface ExitStatus {
  index: number;
  target: string;
  has_return: boolean;
  return_exit: LevelExit | null;
  proposal: LevelExit | null;
  error: string | null;
}

export async function fetchExitStatus(fromId: string, exits: LevelExit[]): Promise<ExitStatus[]> {
  const res = await fetch('/api/exit-status', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ from_id: fromId, exits }),
  });
  if (!res.ok) throw new Error(`exit status returned ${res.status}`);
  const body = await res.json();
  if (!body.success) throw new Error(body.error || 'exit status failed');
  return body.items || [];
}

export interface ConnectResult {
  created: boolean;
  to_exit: LevelExit;
}

export async function connectLevels(fromId: string, exit: LevelExit): Promise<ConnectResult> {
  const res = await fetch('/api/connect-levels', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ from_id: fromId, exit }),
  });
  if (!res.ok) throw new Error(`connect returned ${res.status}`);
  const body = await res.json();
  if (!body.success) throw new Error(body.error || 'connect failed');
  return { created: !!body.created, to_exit: body.to_exit };
}

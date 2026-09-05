import { defineConfig, Plugin } from 'vite';
import react from '@vitejs/plugin-react';
import fs from 'fs';
import path from 'path';
import { exec, execSync, spawn } from 'child_process';

// Full ROM toolchain probe, evaluated ONCE at server start (module scope).
// NOTE: never require() here: vite bundles this config to ESM, where
// dynamic require() throws "not supported" and kills the whole editor.
// Static imports above are the only safe form.
const TOOLCHAIN_TOOLS = ['python3', 'make', 'lcc', 'uge2source'];
function probeTools(): { ok: boolean; detail: { [k: string]: string } } {
  const detail: { [k: string]: string } = {};
  let ok = true;
  for (const t of TOOLCHAIN_TOOLS) {
    try {
      execSync(`${t} --version 2>&1 || ${t} -v 2>&1 || ${t} 2>&1`, { stdio: 'ignore' });
      detail[t] = 'runs';
    } catch (e: any) {
      // execSync throws on nonzero exit too; what matters is whether
      // the binary launched at all (status !== 127/126 and no ENOENT/ENOEXEC/Exec format error).
      const msg = String((e && e.message) || e);
      if (/ENOENT|ENOEXEC|Exec format error|command not found|not recognized/i.test(msg) || e.status === 127 || e.status === 126) {
        detail[t] = `MISSING/BROKEN (${msg.split('\n')[0]})`;
        ok = false;
      } else {
        detail[t] = 'runs';
      }
    }
  }
  try {
    execSync('rgbasm-huge --help 2>&1 || rgbasm --help 2>&1', { stdio: 'ignore' });
    detail['rgbasm'] = 'runs';
  } catch (e: any) {
    const msg = String((e && e.message) || e);
    detail['rgbasm'] = `MISSING/BROKEN (${msg.split('\n')[0]})`;
    ok = false;
  }
  return { ok, detail };
}

const toolchainProbe = probeTools();

function getNixBin(): string | null {
  const candidates = ['nix', '/run/current-system/sw/bin/nix', '/usr/bin/nix', '/bin/nix'];
  for (const c of candidates) {
    try {
      if (c.startsWith('/') && fs.existsSync(c)) return c;
    } catch {}
  }
  return 'nix';
}

const nixBin = getNixBin();

function levelEditorApiPlugin(): Plugin {
  return {
    name: 'level-editor-api',
    configureServer(server) {
      console.log(`[level-editor] toolchain=${toolchainProbe.ok ? 'direct' : 'nix-develop'}`,
        JSON.stringify(toolchainProbe.detail), `nix=${nixBin}`);
      server.middlewares.use((req, res, next) => {
        const repoRoot = path.resolve(__dirname, '../..');

        // Editor-facing ids for the screen mockups (App.tsx imports these
        // JSONs; the save/load maps below must stay in sync with that list).
        const SCREEN_ID_TO_PATH: Record<string, string> = {
          'title': 'screens/title.json',
          'battle': 'screens/battle.json',
          'battle_default': 'screens/battle/default.json',
          'battle_boss': 'screens/battle/boss.json',
          'battle_ambush': 'screens/battle/ambush.json',
          'battle_duo': 'screens/battle/duo.json',
        };
        const isSafeId = (id: unknown) =>
          typeof id === 'string' && /^[A-Za-z0-9_]+$/.test(id);

        // Live disk reads (no editor rebuild needed after editing JSON by
        // hand or via another tool).  Bundled static imports in App.tsx /
        // Tileset.ts remain as the fallback for built bundles served
        // without this dev API.
        const sendJson = (obj: unknown) => {
          res.writeHead(200, { 'Content-Type': 'application/json' });
          res.end(JSON.stringify(obj));
        };
        const readJsonFile = (rel: string) => {
          const target = path.resolve(repoRoot, rel);
          if (!target.startsWith(repoRoot + path.sep)) throw new Error('bad path');
          return JSON.parse(fs.readFileSync(target, 'utf-8'));
        };
        if (req.method === 'GET' && req.url === '/api/levels') {
          try {
            const levels = fs.readdirSync(path.join(repoRoot, 'levels'))
              .filter((f) => f.endsWith('.json'))
              .map((f) => {
                const data = readJsonFile(path.join('levels', f));
                return { id: data.id || f.replace(/\.json$/, ''), name: data.name || f, category: 'levels' };
              });
            const screens: Array<{ id: string; name: string; category: string }> = [];
            for (const [id, rel] of Object.entries(SCREEN_ID_TO_PATH)) {
              try {
                const data = readJsonFile(rel);
                screens.push({ id, name: data.title || data.label || id, category: 'screens' });
              } catch { /* missing screen file: skip */ }
            }
            sendJson({ success: true, levels, screens });
          } catch (err: any) {
            res.writeHead(500, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify({ success: false, error: err.message }));
          }
          return;
        }

        if (req.method === 'GET' && (req.url || '').startsWith('/api/level')) {
          try {
            const u = new URL(req.url || '', 'http://localhost');
            const category = u.searchParams.get('category') || 'levels';
            const id = u.searchParams.get('id') || '';
            if (!isSafeId(id)) throw new Error(`invalid id '${id}'`);
            let rel: string;
            if (category === 'screens') {
              rel = SCREEN_ID_TO_PATH[id];
              if (!rel) throw new Error(`unknown screen '${id}'`);
            } else if (category === 'levels') {
              rel = path.join('levels', `${id}.json`);
            } else {
              throw new Error(`unknown category '${category}'`);
            }
            sendJson({ success: true, id, category, data: readJsonFile(rel) });
          } catch (err: any) {
            res.writeHead(404, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify({ success: false, error: err.message }));
          }
          return;
        }

        if (req.method === 'GET' && req.url === '/api/tilesets') {
          try {
            const dir = path.join(repoRoot, 'tools', 'level_editor', 'tilesets');
            const tilesets = fs.readdirSync(dir)
              .filter((f) => f.endsWith('.json'))
              .map((f) => {
                const data = readJsonFile(path.join('tools', 'level_editor', 'tilesets', f));
                return { id: data.id || f.replace(/\.json$/, ''), label: data.label || f };
              });
            sendJson({ success: true, tilesets });
          } catch (err: any) {
            res.writeHead(500, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify({ success: false, error: err.message }));
          }
          return;
        }

        if (req.method === 'GET' && (req.url || '').startsWith('/api/tileset')) {
          try {
            const u = new URL(req.url || '', 'http://localhost');
            const id = u.searchParams.get('id') || '';
            if (!isSafeId(id)) throw new Error(`invalid id '${id}'`);
            sendJson({ success: true, id, data: readJsonFile(path.join('tools', 'level_editor', 'tilesets', `${id}.json`)) });
          } catch (err: any) {
            res.writeHead(404, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify({ success: false, error: err.message }));
          }
          return;
        }

        if (req.method === 'POST' && req.url === '/api/save-level') {
          let body = '';
          req.on('data', chunk => { body += chunk; });
          req.on('end', () => {
            try {
              const { id, category, data } = JSON.parse(body);
              if (!isSafeId(id)) throw new Error(`invalid id '${id}'`);
              let targetPath = path.join(repoRoot, 'levels', `${id}.json`);
              if (category === 'screens') {
                const rel = SCREEN_ID_TO_PATH[id];
                if (rel) {
                  targetPath = path.join(repoRoot, rel);
                } else if (data.hud_layout || data.enemies || data.enemy_positions) {
                  targetPath = path.join(repoRoot, 'screens', 'battle', `${id}.json`);
                } else {
                  targetPath = path.join(repoRoot, 'screens', 'title', `${id}.json`);
                }
              }
              fs.mkdirSync(path.dirname(targetPath), { recursive: true });
              fs.writeFileSync(targetPath, JSON.stringify(data, null, 2), 'utf-8');
              res.writeHead(200, { 'Content-Type': 'application/json' });
              res.end(JSON.stringify({ success: true, path: targetPath }));
            } catch (err: any) {
              res.writeHead(500, { 'Content-Type': 'application/json' });
              res.end(JSON.stringify({ success: false, error: err.message }));
            }
          });
          return;
        }

        if (req.method === 'POST' && req.url === '/api/save-tileset') {
          let body = '';
          req.on('data', chunk => { body += chunk; });
          req.on('end', () => {
            try {
              const { id, data, images } = JSON.parse(body);
              if (!isSafeId(id)) throw new Error(`invalid id '${id}'`);
              const targetPath = path.join(repoRoot, 'tools', 'level_editor', 'tilesets', `${id}.json`);
              fs.mkdirSync(path.dirname(targetPath), { recursive: true });
              fs.writeFileSync(targetPath, JSON.stringify(data, null, 2), 'utf-8');

              if (images && typeof images === 'object') {
                const tilesDir = path.join(repoRoot, 'tools', 'level_editor', 'public', 'tiles', id);
                fs.mkdirSync(tilesDir, { recursive: true });
                for (const [tileId, base64Data] of Object.entries(images)) {
                  if (typeof base64Data === 'string' && base64Data.startsWith('data:image/')) {
                    const base64Content = base64Data.split(',')[1];
                    if (base64Content) {
                      const imgBuffer = Buffer.from(base64Content, 'base64');
                      fs.writeFileSync(path.join(tilesDir, `${tileId}.png`), imgBuffer);
                    }
                  }
                }
              }

              res.writeHead(200, { 'Content-Type': 'application/json' });
              res.end(JSON.stringify({ success: true, path: targetPath }));
            } catch (err: any) {
              res.writeHead(200, { 'Content-Type': 'application/json' });
              res.end(JSON.stringify({ success: false, error: err.message }));
            }
          });
          return;
        }

        function runInToolchain(cmd: string, callback: (err: any, stdout: string, stderr: string) => void) {
          const nixCmd = `${nixBin} develop --command bash --norc -c ${JSON.stringify(cmd)}`;
          const attempts: { route: string; cmd: string; error?: string; stderr?: string }[] = [];
          const finish = (err: any, stdout: string, stderr: string, route: string) => {
            if (err) attempts.push({ route, cmd: route === 'direct' ? cmd : nixCmd, error: err.message, stderr });
            (callback as any)(err, stdout, stderr, attempts);
          };
          const runNix = (cb: (err: any, stdout: string, stderr: string) => void) => {
            exec(nixCmd, { cwd: repoRoot, maxBuffer: 10 * 1024 * 1024 }, (err, stdout, stderr) => cb(err, stdout, stderr));
          };
          const runDirect = (cb: (err: any, stdout: string, stderr: string) => void) => {
            exec(cmd, { cwd: repoRoot, maxBuffer: 10 * 1024 * 1024 }, (err, stdout, stderr) => cb(err, stdout, stderr));
          };
          if (toolchainProbe.ok) {
            // Direct tools verified: run direct; on failure retry once via
            // nix (host env quirks) and report BOTH attempts explicitly.
            runDirect((err, stdout, stderr) => {
              if (!err) return callback(err, stdout, stderr);
              attempts.push({ route: 'direct', cmd, error: err.message, stderr });
              runNix((err2, stdout2, stderr2) => finish(err2, stdout2, stderr2, 'nix-develop'));
            });
          } else if (nixBin) {
            // Toolchain incomplete: direct execution is known-broken, so do
            // not attempt it (it only produces cryptic late failures).
            runNix((err, stdout, stderr) => finish(err, stdout, stderr, 'nix-develop'));
          } else {
            const err: any = new Error(
              `ROM toolchain incomplete (${Object.entries(toolchainProbe.detail).filter(([, v]) => v !== 'runs').map(([k]) => k).join(', ')}) ` +
              `and no 'nix' binary found. Run the editor from inside \`nix develop\` (AGENTS.md section 1).`);
            callback(err, '', '');
          }
        }

        if (req.method === 'POST' && req.url === '/api/compile-rom') {
          runInToolchain('python3 tools/level_compiler/compile.py --all -o src/game/scenes_content.c && python3 tools/screen_compiler/title_compile.py -o src/game/title_data.c screens/title.json && python3 tools/screen_compiler/battle_compile.py --all -o src/game/ && make debug', ((err: any, stdout: string, stderr: string, attempts: any[]) => {
            if (err) {
              console.error('Compile error:', err.message);
              if (stderr) console.error('Compile stderr:\n' + stderr);
              if (stdout) console.error('Compile stdout:\n' + stdout);
              const combinedError = [stderr, stdout, err.message].filter(Boolean).join('\n\n');
              res.writeHead(200, { 'Content-Type': 'application/json' });
              res.end(JSON.stringify({ success: false, error: combinedError || err.message, log: stdout, attempts: attempts || [] }));
            } else {
              res.writeHead(200, { 'Content-Type': 'application/json' });
              res.end(JSON.stringify({ success: true, log: stdout, romPath: 'build/rpg_card_proto_debug.gb' }));
            }
          }) as any);
          return;
        }

        if (req.method === 'POST' && req.url === '/api/run-game') {
          // Run Game always plays the RELEASE ROM (the debug ROM's
          // per-frame harness work makes it feel sluggish in play).
          const romPath = path.join(repoRoot, 'build', 'rpg_card_proto.gb');
          const contentDirs = ['levels', 'screens',
            path.join('tools', 'level_editor', 'tilesets')];

          // Staleness probe: content JSON newer than the release ROM means
          // the build is stale; a missing ROM must be built, not launched.
          const findStale = (): string[] => {
            try {
              const romMtime = fs.statSync(romPath).mtimeMs;
              const fresh: string[] = [];
              const scan = (dir: string) => {
                for (const e of fs.readdirSync(dir, { withFileTypes: true })) {
                  const p = path.join(dir, e.name);
                  if (e.isDirectory()) { scan(p); continue; }
                  if (!e.name.endsWith('.json')) continue;
                  try {
                    if (fs.statSync(p).mtimeMs > romMtime) fresh.push(path.relative(repoRoot, p));
                  } catch { /* raced deletion: ignore */ }
                }
              };
              for (const d of contentDirs) {
                try { scan(path.join(repoRoot, d)); } catch { /* missing dir: ignore */ }
              }
              return fresh.sort().slice(0, 8);
            } catch {
              return ['<rom missing: building release>'];
            }
          };

          const launch = () => {
            runInToolchain('which sameboy mgba pyboy 2>&1', (err, stdout) => {
              const lines = stdout.split('\n').map(l => l.trim()).filter(l => l && !l.includes('no ') && !l.includes('warning'));
              const emu = lines[0] || 'pyboy';
              const hasDirectTools = (() => {
                try {
                  execSync(`${emu} --help 2>&1`, { stdio: 'ignore' });
                  return true;
                } catch {
                  return false;
                }
              })();

              try {
                const child = hasDirectTools
                  ? spawn(emu, [romPath], { cwd: repoRoot, detached: true, stdio: 'ignore' })
                  : spawn(getNixBin() || 'nix', ['develop', '--command', emu, romPath], { cwd: repoRoot, detached: true, stdio: 'ignore' });

                child.unref();
                res.writeHead(200, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify({ success: true, emulator: path.basename(emu), message: `Launched ${path.basename(emu)} on desktop (release ROM)`, stale: [] }));
              } catch (spawnErr: any) {
                res.writeHead(200, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify({ success: false, error: spawnErr.message }));
              }
            });
          };

          if (findStale().length === 0) {
            launch();
            return;
          }
          // Missing or stale release ROM: build it first, then launch.
          runInToolchain('make release', ((err: any, stdout: string, stderr: string) => {
            if (err || !fs.existsSync(romPath)) {
              console.error('Release build error:', err && err.message);
              if (stderr) console.error('Release stderr:\n' + stderr);
              const combinedError = [stderr, stdout, err && err.message].filter(Boolean).join('\n\n');
              res.writeHead(200, { 'Content-Type': 'application/json' });
              res.end(JSON.stringify({ success: false, error: combinedError || 'release ROM missing and rebuild failed' }));
              return;
            }
            launch();
          }) as any);
          return;
        }

        if (req.method === 'GET' && req.url === '/api/rom') {
          const romPath = path.join(repoRoot, 'build', 'rpg_card_proto_debug.gb');
          if (fs.existsSync(romPath)) {
            const data = fs.readFileSync(romPath);
            res.writeHead(200, {
              'Content-Type': 'application/octet-stream',
              'Content-Length': data.length,
              'Content-Disposition': 'inline; filename="rpg_card_proto_debug.gb"'
            });
            res.end(data);
          } else {
            res.writeHead(404, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify({ error: 'ROM not built yet. Click Compile ROM first!' }));
          }
          return;
        }

        next();
      });
    }
  };
}

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [react(), levelEditorApiPlugin()],
  server: {
    port: 3000
  }
});

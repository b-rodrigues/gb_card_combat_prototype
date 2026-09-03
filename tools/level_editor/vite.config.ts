import { defineConfig, Plugin } from 'vite';
import react from '@vitejs/plugin-react';
import fs from 'fs';
import path from 'path';
import { exec, spawn } from 'child_process';

function levelEditorApiPlugin(): Plugin {
  return {
    name: 'level-editor-api',
    configureServer(server) {
      server.middlewares.use((req, res, next) => {
        const repoRoot = path.resolve(__dirname, '../..');

        if (req.method === 'POST' && req.url === '/api/save-level') {
          let body = '';
          req.on('data', chunk => { body += chunk; });
          req.on('end', () => {
            try {
              const { id, category, data } = JSON.parse(body);
              let targetPath = path.join(repoRoot, 'levels', `${id}.json`);
              if (category === 'screens') {
                if (data.hud_layout || data.enemies || data.enemy_positions) {
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

        function getNixBin(): string | null {
          const candidates = ['nix', '/run/current-system/sw/bin/nix', '/usr/bin/nix', '/bin/nix'];
          for (const c of candidates) {
            try {
              if (c.startsWith('/') && fs.existsSync(c)) return c;
            } catch {}
          }
          return 'nix';
        }

        // Full ROM toolchain probe. The old check (python3/make/lcc only)
        // could select the direct path in an env with compilers but no
        // audio tools, dying late at the first native-audio exec with a
        // cryptic OSError. Every binary here must EXECUTE, not just resolve.
        const TOOLCHAIN_TOOLS = ['python3', 'make', 'lcc', 'uge2source'];
        function probeTools(): { ok: boolean; detail: { [k: string]: string } } {
          const { execSync } = require('child_process');
          const detail: { [k: string]: string } = {};
          let ok = true;
          for (const t of TOOLCHAIN_TOOLS) {
            try {
              execSync(`${t} --version 2>&1 || ${t} -v 2>&1 || ${t} 2>&1`, { stdio: 'ignore' });
              detail[t] = 'runs';
            } catch (e: any) {
              // execSync throws on nonzero exit too; what matters is whether
              // the binary launched at all (status !== 127 and no ENOENT/ENOEXEC).
              const msg = String((e && e.message) || e);
              if (/ENOENT|ENOEXEC|command not found|not recognized/i.test(msg) || e.status === 127) {
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
          } catch {
            detail['rgbasm'] = 'MISSING/BROKEN (neither rgbasm-huge nor rgbasm runs)';
            ok = false;
          }
          return { ok, detail };
        }

        const toolchainProbe = probeTools();
        const nixBin = getNixBin();
        console.log(`[level-editor] repoRoot=${repoRoot} toolchain=${toolchainProbe.ok ? 'direct' : 'nix-develop'}`,
          JSON.stringify(toolchainProbe.detail));

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
          runInToolchain('python3 tools/level_compiler/compile.py --all -o src/game/scenes_content.c && make debug', ((err: any, stdout: string, stderr: string, attempts: any[]) => {
            if (err) {
              console.error('Compile error:', err.message, stderr);
              res.writeHead(200, { 'Content-Type': 'application/json' });
              res.end(JSON.stringify({ success: false, error: stderr || err.message, log: stdout, attempts: attempts || [] }));
            } else {
              res.writeHead(200, { 'Content-Type': 'application/json' });
              res.end(JSON.stringify({ success: true, log: stdout, romPath: 'build/rpg_card_proto_debug.gb' }));
            }
          }) as any);
          return;
        }

        if (req.method === 'POST' && req.url === '/api/run-game') {
          runInToolchain('which sameboy mgba pyboy 2>&1', (err, stdout) => {
            const lines = stdout.split('\n').map(l => l.trim()).filter(l => l && !l.includes('no ') && !l.includes('warning'));
            const emu = lines[0] || 'pyboy';
            const romPath = path.join(repoRoot, 'build', 'rpg_card_proto_debug.gb');
            const hasDirectTools = (() => {
              try {
                const { execSync } = require('child_process');
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
              res.end(JSON.stringify({ success: true, emulator: path.basename(emu), message: `Launched ${path.basename(emu)} on desktop` }));
            } catch (spawnErr: any) {
              res.writeHead(200, { 'Content-Type': 'application/json' });
              res.end(JSON.stringify({ success: false, error: spawnErr.message }));
            }
          });
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

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

        function runInToolchain(cmd: string, callback: (err: any, stdout: string, stderr: string) => void) {
          const hasDirectTools = (() => {
            try {
              const { execSync } = require('child_process');
              execSync('python3 --version && make --version && lcc -v', { stdio: 'ignore' });
              return true;
            } catch {
              return false;
            }
          })();

          const nixBin = getNixBin();
          const fullCmd = hasDirectTools
            ? cmd
            : `${nixBin} develop --command bash --norc -c ${JSON.stringify(cmd)}`;

          exec(fullCmd, { cwd: repoRoot, maxBuffer: 10 * 1024 * 1024 }, (err, stdout, stderr) => {
            if (err && !hasDirectTools) {
              // Fallback to direct execution in case nix failed
              exec(cmd, { cwd: repoRoot, maxBuffer: 10 * 1024 * 1024 }, callback);
            } else if (err && hasDirectTools && nixBin) {
              // Fallback to nix develop in case host environment had issues
              const nixFallback = `${nixBin} develop --command bash --norc -c ${JSON.stringify(cmd)}`;
              exec(nixFallback, { cwd: repoRoot, maxBuffer: 10 * 1024 * 1024 }, callback);
            } else {
              callback(err, stdout, stderr);
            }
          });
        }

        if (req.method === 'POST' && req.url === '/api/compile-rom') {
          runInToolchain('python3 tools/level_compiler/compile.py --all -o src/game/scenes_content.c && make debug', (err, stdout, stderr) => {
            if (err) {
              console.error('Compile error:', err.message, stderr);
              res.writeHead(200, { 'Content-Type': 'application/json' });
              res.end(JSON.stringify({ success: false, error: stderr || err.message, log: stdout }));
            } else {
              res.writeHead(200, { 'Content-Type': 'application/json' });
              res.end(JSON.stringify({ success: true, log: stdout, romPath: 'build/rpg_card_proto_debug.gb' }));
            }
          });
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

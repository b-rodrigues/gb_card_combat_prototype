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

        if (req.method === 'POST' && req.url === '/api/compile-rom') {
          exec('python3 tools/level_compiler/compile.py --all -o src/game/scenes_content.c && make debug', { cwd: repoRoot }, (err, stdout, stderr) => {
            if (err) {
              res.writeHead(500, { 'Content-Type': 'application/json' });
              res.end(JSON.stringify({ success: false, error: stderr || err.message, log: stdout }));
            } else {
              res.writeHead(200, { 'Content-Type': 'application/json' });
              res.end(JSON.stringify({ success: true, log: stdout, romPath: 'build/rpg_card_proto_debug.gb' }));
            }
          });
          return;
        }

        if (req.method === 'POST' && req.url === '/api/run-game') {
          exec('which pyboy sameboy mgba-qt mgba 2>&1', { cwd: repoRoot }, (err, stdout) => {
            const lines = stdout.split('\n').map(l => l.trim()).filter(l => l && !l.includes('no ') && !l.includes('warning'));
            const emu = lines[0] || 'pyboy';
            const romPath = path.join(repoRoot, 'build', 'rpg_card_proto_debug.gb');
            try {
              const child = spawn(emu, [romPath], {
                cwd: repoRoot,
                detached: true,
                stdio: 'ignore'
              });
              child.unref();
              res.writeHead(200, { 'Content-Type': 'application/json' });
              res.end(JSON.stringify({ success: true, emulator: path.basename(emu), message: `Launched ${path.basename(emu)} on desktop` }));
            } catch (spawnErr: any) {
              res.writeHead(500, { 'Content-Type': 'application/json' });
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

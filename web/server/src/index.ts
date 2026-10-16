import express from 'express';
import cors from 'cors';
import * as path from 'path';
import * as fs from 'fs';
import { fileURLToPath } from 'url';
import { CompilerRunner } from './compiler.js';
import { DoxygenRunner } from './doxygen.js';
import { loadExamples } from './examples.js';
import { CompileRequest, TargetInfo } from './types.js';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const clientDist = path.resolve(__dirname, '../../client/dist');

const app = express();
const port = process.env.PORT ? parseInt(process.env.PORT, 10) : 3001;

app.use(cors());
app.use(express.json({ limit: '10mb' }));

const doxygenRunner = new DoxygenRunner();
const compilerRunner = new CompilerRunner(doxygenRunner);

// Trigger background indexing on startup if Doxygen is available
if (doxygenRunner.getStatus().available) {
  console.log('[EzPacker Server] Initiating background Doxygen indexing...');
  doxygenRunner.generateDocumentation().then((status) => {
    console.log(`[EzPacker Server] Doxygen documentation indexed: ${status.entityCount} entities (status: ${status.status})`);
  }).catch((err) => {
    console.warn('[EzPacker Server] Background Doxygen indexing failed:', err.message);
  });
} else {
  console.log('[EzPacker Server] Doxygen is not installed on host system. Documentation indexing disabled.');
}

// Target configuration
const targetInfo: TargetInfo = {
  defaultTriple: process.platform === 'win32' ? 'x86_64-pc-windows-msvc' : 'x86_64-linux-gnu',
  availableTriples: [
    { id: 'x86_64-pc-windows-msvc', label: 'x86_64 Windows (MSVC)', os: 'windows', arch: 'x86_64' },
    { id: 'x86_64-linux-gnu', label: 'x86_64 Linux (GNU)', os: 'linux', arch: 'x86_64' },
    { id: 'x86_64-apple-darwin', label: 'x86_64 macOS (Darwin)', os: 'darwin', arch: 'x86_64' },
    { id: 'x86_64', label: 'Generic x86_64', os: 'generic', arch: 'x86_64' },
    { id: 'riscv64-unknown-linux-gnu', label: 'RISC-V 64 Linux (GNU)', os: 'linux', arch: 'riscv64' },
    { id: 'riscv64-unknown-elf', label: 'RISC-V 64 Bare-Metal (ELF)', os: 'elf', arch: 'riscv64' },
    { id: 'riscv64', label: 'Generic RISC-V 64', os: 'generic', arch: 'riscv64' },
  ],
  features: [
    { id: '+avx', label: '+avx', description: 'Enable Advanced Vector Extensions (AVX)' },
    { id: '+avx2', label: '+avx2', description: 'Enable AVX2 256-bit integer and FMA extensions' },
    { id: '+sse', label: '+sse', description: 'Enable Streaming SIMD Extensions', default: true },
    { id: '+sse2', label: '+sse2', description: 'Enable SSE2 128-bit vector instructions', default: true },
    { id: '+sse3', label: '+sse3', description: 'Enable SSE3 instructions' },
    { id: '+sse4.1', label: '+sse4.1', description: 'Enable SSE4.1 instructions' },
    { id: '+m', label: '+m', description: 'Enable Integer Multiplication and Division (RV64M)', default: true },
    { id: '+a', label: '+a', description: 'Enable Atomic Instructions (RV64A)', default: true },
    { id: '+f', label: '+f', description: 'Enable Single-Precision Floating-Point (RV64F)', default: true },
    { id: '+d', label: '+d', description: 'Enable Double-Precision Floating-Point (RV64D)', default: true },
    { id: '+c', label: '+c', description: 'Enable Compressed Instructions (RVC)', default: false },
  ],
};

app.get('/api/health', (req, res) => {
  res.json({
    status: 'ok',
    compilerPath: compilerRunner.getCompilerPath(),
    doxygen: doxygenRunner.getStatus(),
    uptime: process.uptime(),
  });
});

app.get('/api/targets', (req, res) => {
  res.json(targetInfo);
});

app.get('/api/examples', (req, res) => {
  try {
    const examples = loadExamples();
    res.json(examples);
  } catch (err: any) {
    res.status(500).json({ error: err.message });
  }
});

// Doxygen Documentation Endpoints
app.get('/api/doxygen/status', (req, res) => {
  res.json(doxygenRunner.getStatus());
});

app.post('/api/doxygen/generate', async (req, res) => {
  try {
    const status = await doxygenRunner.generateDocumentation(true);
    res.json(status);
  } catch (err: any) {
    res.status(500).json({
      available: false,
      generated: false,
      status: 'error',
      errorMessage: err.message || 'Failed to generate Doxygen documentation',
      entityCount: 0,
    });
  }
});

app.get('/api/doxygen/symbols', (req, res) => {
  const query = typeof req.query.q === 'string' ? req.query.q : '';
  const limit = req.query.limit ? parseInt(req.query.limit as string, 10) : 50;
  const symbols = doxygenRunner.searchSymbols(query, limit);
  res.json(symbols);
});

app.get('/api/doxygen/symbol/:name', (req, res) => {
  const symbol = doxygenRunner.getSymbol(req.params.name);
  if (symbol) {
    res.json(symbol);
  } else {
    res.status(404).json({ error: `Symbol not found: ${req.params.name}` });
  }
});

app.post('/api/compile', async (req, res) => {
  try {
    const compileReq: CompileRequest = req.body;
    if (!compileReq || typeof compileReq.code !== 'string') {
      res.status(400).json({ error: 'Missing code string in request body' });
      return;
    }

    const result = await compilerRunner.compile(compileReq);
    res.json(result);
  } catch (err: any) {
    console.error('Compilation failed:', err);
    res.status(500).json({
      success: false,
      exitCode: 1,
      diagnostics: [
        {
          type: 'error',
          component: 'ServerBridge',
          message: err.message || 'Internal server error while executing compiler',
        },
      ],
      passes: [],
      functions: {},
      executionDurationMs: 0,
    });
  }
});

// Serve production static build if client/dist exists
if (fs.existsSync(clientDist)) {
  console.log(`[EzPacker Server] Serving static web client from: ${clientDist}`);
  app.use(express.static(clientDist));
  app.get('*', (req, res, next) => {
    if (req.path.startsWith('/api')) {
      return next();
    }
    res.sendFile(path.join(clientDist, 'index.html'));
  });
}

app.listen(port, () => {
  console.log(`[EzPacker Server] Compiler API listening at http://localhost:${port}`);
  console.log(`[EzPacker Server] Using compiler binary: ${compilerRunner.getCompilerPath()}`);
});

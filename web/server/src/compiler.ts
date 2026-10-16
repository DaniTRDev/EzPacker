import { spawn } from 'child_process';
import * as fs from 'fs';
import * as path from 'path';
import * as os from 'os';
import { fileURLToPath } from 'url';
import { CompileRequest, CompileResponse, PassTiming, DiagnosticItem, FunctionAnalysis, ObjectAnalysis, DoxygenDocEntity } from './types.js';
import { parseCompilerOutput, extractCleanMir, extractCleanAsm, stripAnsi } from './parser.js';
import { parseObjectFile } from './objectParser.js';
import { DoxygenRunner } from './doxygen.js';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const projectRoot = path.resolve(__dirname, '../../../');

export class CompilerRunner {
  private ezcPath: string;
  private doxygenRunner?: DoxygenRunner;

  constructor(doxygenRunner?: DoxygenRunner) {
    this.ezcPath = this.locateCompilerExecutable();
    this.doxygenRunner = doxygenRunner;
  }

  private locateCompilerExecutable(): string {
    if (process.env.EZC_BIN && fs.existsSync(process.env.EZC_BIN)) {
      return path.resolve(process.env.EZC_BIN);
    }

    const candidates = [
      path.join(projectRoot, 'cmake-build-debug', 'bin', 'ezc.exe'),
      path.join(projectRoot, 'cmake-build-debug', 'bin', 'ezc'),
      path.join(projectRoot, 'build', 'bin', 'ezc.exe'),
      path.join(projectRoot, 'build', 'bin', 'ezc'),
      path.join(projectRoot, 'build', 'Release', 'bin', 'ezc.exe'),
    ];

    for (const cand of candidates) {
      if (fs.existsSync(cand)) {
        return cand;
      }
    }

    console.warn(`[CompilerRunner] Could not find ezc executable in candidates. Defaulting to: ${candidates[0]}`);
    return candidates[0];
  }

  public getCompilerPath(): string {
    return this.ezcPath;
  }

  private executeEzc(args: string[]): Promise<{ stdout: string; stderr: string; exitCode: number }> {
    return new Promise((resolve) => {
      let stdout = '';
      let stderr = '';

      const proc = spawn(this.ezcPath, args, {
        cwd: projectRoot,
      });

      proc.stdout.on('data', (chunk) => {
        stdout += chunk.toString();
      });

      proc.stderr.on('data', (chunk) => {
        stderr += chunk.toString();
      });

      proc.on('close', (code) => {
        resolve({
          stdout,
          stderr,
          exitCode: code ?? 0,
        });
      });

      proc.on('error', (err) => {
        stderr += `\nFailed to start ezc: ${err.message}`;
        resolve({
          stdout,
          stderr,
          exitCode: 1,
        });
      });
    });
  }

  public async compile(req: CompileRequest): Promise<CompileResponse> {
    const startTime = performance.now();
    const tempDir = fs.mkdtempSync(path.join(os.tmpdir(), 'ezc_web_'));
    const tempMirFile = path.join(tempDir, 'source.mir');

    try {
      fs.writeFileSync(tempMirFile, req.code, 'utf-8');

      // Common flags
      const baseFlags: string[] = [];
      if (req.targetTriple) {
        baseFlags.push('--target', req.targetTriple);
      }
      if (req.optLevel) {
        baseFlags.push(`-${req.optLevel}`);
      }
      if (req.isPositionIndependent) {
        baseFlags.push('-fPIC');
      }
      if (req.enableRtti === false) {
        baseFlags.push('--no-rtti');
      } else {
        baseFlags.push('--rtti');
      }
      if (req.features && req.features.length > 0) {
        for (const feat of req.features) {
          baseFlags.push('--target-feature', feat);
        }
      }

      // Main run with telemetry flags to get timings, diagnostics, CFG and liveness
      const telemetryFlags = [...baseFlags, '--time-passes', '--print-passes', '-v'];

      // 1. Run main assembly compilation (-S)
      const asmRes = await this.executeEzc([...telemetryFlags, '-S', tempMirFile]);

      // Parse the output for assembly, passes, diagnostics, CFG and liveness
      const parsedAsm = parseCompilerOutput(asmRes.stdout, asmRes.stderr, req.code);

      let genericMir = '';
      let legalizedMir = '';
      let loweredMir = '';
      let objectInfo: ObjectAnalysis | undefined = undefined;

      // If compilation succeeded, fetch clean intermediates and object binary concurrently
      if (asmRes.exitCode === 0) {
        const tempObjFile = path.join(tempDir, 'output.obj');
        const [mirRes, legRes, lowRes, objRes] = await Promise.all([
          this.executeEzc([...baseFlags, '--emit-mir', tempMirFile]),
          this.executeEzc([...baseFlags, '--emit-legalized-mir', tempMirFile]),
          this.executeEzc([...baseFlags, '--emit-lowered-mir', tempMirFile]),
          this.executeEzc([...baseFlags, '-o', tempObjFile, tempMirFile]),
        ]);

        genericMir = extractCleanMir(mirRes.stdout);
        legalizedMir = extractCleanMir(legRes.stdout);
        loweredMir = extractCleanMir(lowRes.stdout);

        if (fs.existsSync(tempObjFile)) {
          try {
            objectInfo = parseObjectFile(tempObjFile);
          } catch (objErr: any) {
            console.warn('[CompilerRunner] Failed parsing object file:', objErr.message);
          }
        }
      }

      // Collect symbol candidates for Doxygen documentation lookup
      const symbolCandidates = new Set<string>();
      for (const fn of Object.keys(parsedAsm.functions)) {
        symbolCandidates.add(fn);
      }
      if (objectInfo) {
        for (const sym of objectInfo.symbols) {
          if (sym.name) symbolCandidates.add(sym.name);
        }
      }

      // Scan runtime call patterns in source (e.g. @__ez_throw, @__ez_try_enter)
      const tokenRegex = /@([A-Za-z0-9_]+)/g;
      let m: RegExpExecArray | null;
      while ((m = tokenRegex.exec(req.code)) !== null) {
        symbolCandidates.add(m[1]);
      }

      const matchedSymbols = this.doxygenRunner
        ? this.doxygenRunner.matchSymbols(Array.from(symbolCandidates))
        : {};

      const durationMs = Math.round((performance.now() - startTime) * 100) / 100;
      const cleanAsm = extractCleanAsm(asmRes.stdout);

      return {
        success: asmRes.exitCode === 0,
        exitCode: asmRes.exitCode,
        asm: cleanAsm || parsedAsm.cleanOutput,
        mir: genericMir,
        legalizedMir: legalizedMir,
        loweredMir: loweredMir,
        rawOutput: stripAnsi(asmRes.stdout),
        rawStderr: stripAnsi(asmRes.stderr),
        passes: parsedAsm.passes,
        diagnostics: parsedAsm.diagnostics,
        functions: parsedAsm.functions,
        objectInfo,
        doxygenStatus: this.doxygenRunner?.getStatus(),
        matchedSymbols: Object.keys(matchedSymbols).length > 0 ? matchedSymbols : undefined,
        executionDurationMs: durationMs,
      };
    } finally {
      try {
        fs.rmSync(tempDir, { recursive: true, force: true });
      } catch (err) {
        // ignore cleanup error
      }
    }
  }
}

import { spawn } from 'child_process';
import * as fs from 'fs';
import * as path from 'path';
import * as os from 'os';
import { fileURLToPath } from 'url';
import {
  CompileRequest,
  CompileResponse,
  PassTiming,
  DiagnosticItem,
  FunctionAnalysis,
  CfgBlock,
  ObjectAnalysis,
  PipelineTelemetryJson,
} from './types.js';
import { stripAnsi, extractCleanAsm } from './parser.js';
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
    const isC = req.language === 'c' ||
      (Boolean(req.filename) && req.filename!.endsWith('.c')) ||
      (!req.code.includes('fn @') && !req.code.includes('function @') &&
        (/\b(int|void|char|short|long|float|double|struct|typedef|extern|#include|#define)\b/.test(req.code)));
    const sourceFileName = isC ? 'source.c' : 'source.mir';
    const tempSourceFile = path.join(tempDir, sourceFileName);
    const tempJsonFile = path.join(tempDir, 'pipeline.json');
    const tempObjFile = path.join(tempDir, 'output.obj');

    try {
      fs.writeFileSync(tempSourceFile, req.code, 'utf-8');

      // Build single unified ezc command line
      const args: string[] = [
        '-o', tempObjFile,
        '--dump-pipeline-json', tempJsonFile,
        '-v',
        '--time-passes',
        '--print-passes',
      ];

      if (req.targetTriple) {
        args.push('--target', req.targetTriple);
      }
      if (req.optLevel) {
        args.push(`-${req.optLevel}`);
      }
      if (req.isPositionIndependent) {
        args.push('-fPIC');
      }
      if (req.enableRtti === false) {
        args.push('--no-rtti');
      } else {
        args.push('--rtti');
      }
      if (req.features && req.features.length > 0) {
        for (const feat of req.features) {
          args.push('--target-feature', feat);
        }
      }

      args.push(tempSourceFile);

      // Execute ezc once
      const ezcRes = await this.executeEzc(args);

      let pipelineData: PipelineTelemetryJson | undefined;
      if (fs.existsSync(tempJsonFile)) {
        try {
          const jsonContent = fs.readFileSync(tempJsonFile, 'utf-8');
          pipelineData = JSON.parse(jsonContent) as PipelineTelemetryJson;
        } catch (jsonErr: any) {
          console.warn('[CompilerRunner] Failed to parse pipeline telemetry JSON:', jsonErr.message);
        }
      }

      let objectInfo: ObjectAnalysis | undefined;
      if (fs.existsSync(tempObjFile)) {
        try {
          objectInfo = parseObjectFile(tempObjFile);
        } catch (objErr: any) {
          console.warn('[CompilerRunner] Failed parsing object file:', objErr.message);
        }
      }

      // Convert pass results to legacy PassTiming array
      const passes: PassTiming[] = (pipelineData?.passes || []).map((p) => ({
        name: p.name,
        target: p.function,
        durationMs: p.durationMs,
      }));

      // Extract diagnostics
      const diagnostics: DiagnosticItem[] = (pipelineData?.diagnostics || []).map((d) => ({
        type: d.severity,
        component: d.component || 'EzCompiler',
        message: d.message,
        file: d.file || undefined,
        line: d.line > 0 ? d.line : undefined,
        col: d.col > 0 ? d.col : undefined,
      }));

      // If ezc failed and diagnostics are empty, extract error from stderr
      if (ezcRes.exitCode !== 0 && diagnostics.length === 0 && ezcRes.stderr) {
        const cleanErr = stripAnsi(ezcRes.stderr).trim();
        if (cleanErr) {
          diagnostics.push({
            type: 'error',
            component: 'EzCompiler',
            message: cleanErr,
          });
        }
      }

      // Convert functions telemetry to FunctionAnalysis
      const functions: Record<string, FunctionAnalysis> = {};
      if (pipelineData?.functions) {
        for (const [fnName, fnTel] of Object.entries(pipelineData.functions)) {
          const blocks: Record<string, CfgBlock> = {};
          for (const b of fnTel.blocks || []) {
            const blockIdStr = String(b.id);
            const blockObj: CfgBlock = {
              id: blockIdStr,
              blockNumber: typeof b.id === 'number' ? b.id : (parseInt(blockIdStr, 10) || undefined),
              instructionCount: b.instructionCount ?? (b.instructions?.length || 0),
              instructions: b.instructions || [],
              predecessors: b.predecessors || [],
              successors: b.successors || [],
            };
            blocks[blockIdStr] = blockObj;
            if (b.name && b.name !== blockIdStr) {
              blocks[b.name] = blockObj;
            }
          }

          const liveness: Record<string, { def: string[]; use: string[]; liveIn: string[]; liveOut: string[] }> = {};
          if (fnTel.liveness && typeof fnTel.liveness === 'object') {
            if (Array.isArray(fnTel.liveness)) {
              for (const l of fnTel.liveness as any[]) {
                liveness[l.blockId] = {
                  def: l.def || [],
                  use: l.use || [],
                  liveIn: l.liveIn || [],
                  liveOut: l.liveOut || [],
                };
              }
            } else {
              for (const [blkKey, l] of Object.entries(fnTel.liveness)) {
                liveness[blkKey] = {
                  def: (l as any).def || [],
                  use: (l as any).use || [],
                  liveIn: (l as any).liveIn || [],
                  liveOut: (l as any).liveOut || [],
                };
              }
            }
          }

          functions[fnName] = {
            name: fnName,
            blocks,
            liveness,
            frameLayout: fnTel.frameLayout
              ? {
                  totalSize: fnTel.frameLayout.totalFrameSize,
                  calleeSaveSize: fnTel.frameLayout.calleeSavedSize,
                }
              : undefined,
          };
        }
      }

      // Find representative snapshots for coarse legacy stage views
      const allPasses = pipelineData?.passes || [];
      const legalPasses = allPasses.filter((p) => p.stage === 'Legalization');
      const lowerPasses = allPasses.filter((p) => p.stage === 'TargetLowering');

      const genericMir = pipelineData?.initialMir || '';
      const legalizedMir = (legalPasses.length > 0 ? legalPasses[legalPasses.length - 1].mirDump : '') || genericMir;
      const loweredMir = (lowerPasses.length > 0 ? lowerPasses[lowerPasses.length - 1].mirDump : '') || legalizedMir;
      const cleanAsm = pipelineData?.assembly || extractCleanAsm(ezcRes.stdout);

      // Collect symbol candidates for Doxygen documentation lookup
      const symbolCandidates = new Set<string>();
      for (const fn of Object.keys(functions)) {
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

      return {
        success: ezcRes.exitCode === 0,
        exitCode: ezcRes.exitCode,
        pipeline: pipelineData,
        asm: cleanAsm,
        mir: genericMir,
        legalizedMir,
        loweredMir,
        rawOutput: stripAnsi(ezcRes.stdout),
        rawStderr: stripAnsi(ezcRes.stderr),
        passes,
        diagnostics,
        functions,
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

import { DiagnosticItem, PassTiming, FunctionAnalysis, CfgBlock } from './types.js';

/**
 * Strips all ANSI terminal escape sequences (colors, cursor movements, resets).
 */
export function stripAnsi(str: string): string {
  if (!str) return '';
  return str.replace(
    /[\u001b\x1b]\[[0-9;]*[a-zA-Z]|[\u001b\x1b]\([a-zA-Z]|[\u001b\x1b][=>]/g,
    ''
  );
}

/**
 * Extracts pure, clean MIR functions from compiler output.
 */
export function extractCleanMir(rawText: string): string {
  const clean = stripAnsi(rawText);
  const mirStart = clean.search(/(?:^|\n)(?:function @|declare @)/);
  if (mirStart !== -1) {
    return clean.slice(mirStart).trim();
  }

  // Fallback: strip trace lines and function debug dumps
  const lines = clean.split(/\r?\n/);
  const filtered: string[] = [];
  let inFunctionDump = false;

  for (const line of lines) {
    const trimmed = line.trim();
    if (trimmed.includes('Function Dump')) {
      inFunctionDump = trimmed.includes('=== Function Dump');
      continue;
    }
    if (inFunctionDump) continue;

    if (
      trimmed.startsWith('[') ||
      trimmed.startsWith('note:') ||
      trimmed.startsWith('source:') ||
      trimmed.startsWith('|') ||
      trimmed === 'empty'
    ) {
      continue;
    }

    filtered.push(line);
  }

  return filtered.join('\n').trim();
}

/**
 * Extracts pure, clean x86-64 assembly from compiler output.
 */
export function extractCleanAsm(rawText: string): string {
  const clean = stripAnsi(rawText);
  const asmStart = clean.search(/(?:^|\n)(?:\.globl|\.extern|\.section|\.text)\b/);
  if (asmStart !== -1) {
    return clean.slice(asmStart).trim();
  }

  const lines = clean.split(/\r?\n/);
  const filtered: string[] = [];
  for (const line of lines) {
    const trimmed = line.trim();
    if (
      trimmed.startsWith('[') ||
      trimmed.startsWith('note:') ||
      trimmed.startsWith('source:') ||
      trimmed.startsWith('|') ||
      trimmed === 'empty' ||
      trimmed.includes('Function Dump')
    ) {
      continue;
    }
    filtered.push(line);
  }

  return filtered.join('\n').trim();
}

/**
 * Parses block instructions directly from dumped or input MIR text.
 */
export function parseInstructionsByBlock(mirText: string): Record<string, Record<string, string[]>> {
  const clean = stripAnsi(mirText);
  const lines = clean.split(/\r?\n/);
  const result: Record<string, Record<string, string[]>> = {};

  let currentFunc: string | null = null;
  let currentBlock: string | null = null;
  let inFunctionDump = false;

  const FORBIDDEN_BLOCK_NAMES = new Set(['note', 'Params', 'Blocks', 'Stack Frame', 'empty']);

  for (const raw of lines) {
    const l = raw.trim();
    if (!l) continue;

    if (l.includes('Function Dump')) {
      inFunctionDump = l.includes('=== Function Dump');
      continue;
    }
    if (inFunctionDump) continue;

    // function header
    const fnMatch = l.match(/(?:function|fn)\s+@([A-Za-z0-9_]+)/);
    if (fnMatch) {
      currentFunc = fnMatch[1];
      if (!result[currentFunc]) {
        result[currentFunc] = {};
      }
      currentBlock = null;
      continue;
    }

    if (l === '}' && currentFunc) {
      currentFunc = null;
      currentBlock = null;
      continue;
    }

    // block label (e.g. entry:, base_case:)
    const blkMatch = l.match(/^([A-Za-z0-9_]+):$/);
    if (blkMatch && currentFunc) {
      const blkName = blkMatch[1];
      if (!FORBIDDEN_BLOCK_NAMES.has(blkName)) {
        currentBlock = blkName;
        if (!result[currentFunc][currentBlock]) {
          result[currentFunc][currentBlock] = [];
        }
      }
      continue;
    }

    // instruction inside block
    if (
      currentFunc &&
      currentBlock &&
      !l.startsWith('//') &&
      !l.startsWith(';') &&
      !l.startsWith('#') &&
      !l.startsWith('[') &&
      !l.startsWith('note:') &&
      !l.startsWith('source:')
    ) {
      // Avoid duplicate instructions if block re-appears in dump
      if (!result[currentFunc][currentBlock].includes(l)) {
        result[currentFunc][currentBlock].push(l);
      }
    }
  }

  return result;
}

export function parseCompilerOutput(
  stdout: string,
  stderr: string,
  sourceCode?: string
): {
  cleanOutput: string;
  diagnostics: DiagnosticItem[];
  passes: PassTiming[];
  functions: Record<string, FunctionAnalysis>;
} {
  const cleanStdout = stripAnsi(stdout);
  const cleanStderr = stripAnsi(stderr);
  const allText = `${cleanStdout}\n${cleanStderr}`;
  const lines = allText.split(/\r?\n/);

  const diagnostics: DiagnosticItem[] = [];
  const passes: PassTiming[] = [];
  const functions: Record<string, FunctionAnalysis> = {};

  // 1. Discover all functions in order of appearance
  const funcNames: string[] = [];
  const registerFunction = (name: string) => {
    const cleanName = name.replace(/^@/, '').trim();
    if (cleanName && !funcNames.includes(cleanName)) {
      funcNames.push(cleanName);
    }
    if (cleanName && !functions[cleanName]) {
      functions[cleanName] = {
        name: cleanName,
        blocks: {},
        liveness: {},
      };
    }
  };

  // Pre-scan from source code if provided
  if (sourceCode) {
    const fnRegex = /(?:fn|function)\s+@([A-Za-z0-9_]+)/g;
    let match;
    while ((match = fnRegex.exec(sourceCode)) !== null) {
      registerFunction(match[1]);
    }
  }

  // Also scan from telemetry lines
  for (const l of lines) {
    const m1 = l.match(/\[Pass\]\s+Running\s+[A-Za-z0-9_]+\s+on\s+([A-Za-z0-9_@]+)/);
    if (m1) registerFunction(m1[1]);
    const m2 = l.match(/topology for:\s*'([^']+)'/);
    if (m2) registerFunction(m2[1]);
    const m3 = l.match(/(?:function|fn)\s+@([A-Za-z0-9_]+)/);
    if (m3) registerFunction(m3[1]);
  }

  // Pre-parse block instructions strictly from sourceCode if available, otherwise from clean extracted MIR
  const cleanMir = extractCleanMir(allText);
  const blockInstructions = parseInstructionsByBlock(sourceCode || cleanMir);

  // Map each discovered function to its declared blocks in source order
  const funcBlocks: Record<string, Set<string>> = {};
  const funcFirstBlock: Record<string, string> = {};
  for (const fn of funcNames) {
    const declaredList = Object.keys(blockInstructions[fn] || {});
    funcBlocks[fn] = new Set(declaredList);
    funcFirstBlock[fn] = declaredList[0] || 'entry';
  }

  // Mapping from blockId (number) -> { funcName, blockName }
  const blockIdToFunc: Record<number, { funcName: string; blockName: string }> = {};

  let i = 0;
  let currentFrameFunc: string | null = null;

  while (i < lines.length) {
    const line = lines[i].trimEnd();
    const trimmed = line.trim();

    // 1. Pass Timing: e.g. [Time] NonSsaToSsaPass: 1.3799 ms
    const timeMatch = trimmed.match(/^\[Time\]\s+([^:]+):\s+([\d.]+)\s+ms/);
    if (timeMatch) {
      passes.push({
        name: timeMatch[1].trim(),
        durationMs: parseFloat(timeMatch[2]),
      });
      i++;
      continue;
    }

    // Pass Target: e.g. [Pass] Running MirFrameLowererPass on main
    const passMatch = trimmed.match(/^\[Pass\]\s+Running\s+([A-Za-z0-9_]+)\s+on\s+([A-Za-z0-9_@]+)/);
    if (passMatch) {
      const passName = passMatch[1];
      const targetName = passMatch[2].replace(/^@/, '');
      registerFunction(targetName);

      if (passes.length > 0 && passes[passes.length - 1].name === passName) {
        passes[passes.length - 1].target = targetName;
      }
      if (passName === 'MirFrameLowererPass') {
        currentFrameFunc = targetName;
      }
      i++;
      continue;
    }

    // 2. MirFrameLowerer Stack Frame Metrics
    const calleeMatch = trimmed.match(/note:\s*Callee save size:\s*([0-9a-fA-F]+)/i);
    if (calleeMatch && currentFrameFunc && functions[currentFrameFunc]) {
      if (!functions[currentFrameFunc].frameLayout) {
        functions[currentFrameFunc].frameLayout = {};
      }
      const val = parseInt(calleeMatch[1], 16) || parseInt(calleeMatch[1], 10);
      functions[currentFrameFunc].frameLayout!.calleeSaveSize = val;
      i++;
      continue;
    }

    const totalMatch = trimmed.match(/note:\s*Total size:\s*([0-9a-fA-F]+)/i);
    if (totalMatch && currentFrameFunc && functions[currentFrameFunc]) {
      if (!functions[currentFrameFunc].frameLayout) {
        functions[currentFrameFunc].frameLayout = {};
      }
      const val = parseInt(totalMatch[1], 16) || parseInt(totalMatch[1], 10);
      functions[currentFrameFunc].frameLayout!.totalSize = val;
      i++;
      continue;
    }

    // 3. CFG Successor list parsing
    if (trimmed.includes('CodeFlowAnalysisPass SUCCESSOR list:')) {
      i++;
      let curIdx = 0;
      let seenFirstBlockOfCur = false;
      while (
        i < lines.length &&
        !lines[i].includes('CodeFlowAnalysisPass PREDECESSOR list:') &&
        !lines[i].startsWith('[Pass]')
      ) {
        const succLine = lines[i].trim();
        const m = succLine.match(/note:\s*%([A-Za-z0-9_]+)\s*\(id=(\d+),\s*instrs=(\d+)\):/);
        if (m) {
          const blkName = m[1];
          const blkId = parseInt(m[2], 10);
          const instCount = parseInt(m[3], 10);

          // Advance to next function if blkName belongs to next function
          while (
            curIdx < funcNames.length - 1 &&
            (
              !funcBlocks[funcNames[curIdx]]?.has(blkName) ||
              (seenFirstBlockOfCur && funcFirstBlock[funcNames[curIdx + 1]] === blkName)
            )
          ) {
            curIdx++;
            seenFirstBlockOfCur = false;
          }

          if (funcFirstBlock[funcNames[curIdx]] === blkName) {
            seenFirstBlockOfCur = true;
          }

          const fName = funcNames[curIdx] || funcNames[0] || 'main';
          registerFunction(fName);

          blockIdToFunc[blkId] = { funcName: fName, blockName: blkName };

          const instrs = blockInstructions[fName]?.[blkName] || [];

          functions[fName].blocks[blkName] = {
            id: blkName,
            blockNumber: blkId,
            instructionCount: instCount,
            instructions: instrs,
            successors: [],
            predecessors: [],
          };

          i++;
          while (i < lines.length) {
            const edgeLine = lines[i].trim();
            if (edgeLine.startsWith('note:') || edgeLine.startsWith('[') || edgeLine === '') break;
            const edgeM = edgeLine.match(/%([A-Za-z0-9_]+)\s*\(id=\d+/);
            if (edgeM && functions[fName]?.blocks[blkName]) {
              if (!functions[fName].blocks[blkName].successors.includes(edgeM[1])) {
                functions[fName].blocks[blkName].successors.push(edgeM[1]);
              }
            }
            i++;
          }
        } else {
          i++;
        }
      }
      continue;
    }

    // 4. CFG Predecessor list parsing
    if (trimmed.includes('CodeFlowAnalysisPass PREDECESSOR list:')) {
      i++;
      while (
        i < lines.length &&
        !lines[i].startsWith('[') &&
        !lines[i].includes('Final Liveness Analysis')
      ) {
        const predLine = lines[i].trim();
        const m = predLine.match(/note:\s*%([A-Za-z0-9_]+)\s*\(id=(\d+)/);
        if (m) {
          const blkId = parseInt(m[2], 10);
          const mapping = blockIdToFunc[blkId];

          i++;
          while (i < lines.length) {
            const edgeLine = lines[i].trim();
            if (edgeLine.startsWith('note:') || edgeLine.startsWith('[') || edgeLine === '') break;
            const edgeM = edgeLine.match(/%([A-Za-z0-9_]+)\s*\(id=\d+/);
            if (edgeM && mapping && functions[mapping.funcName]?.blocks[mapping.blockName]) {
              if (!functions[mapping.funcName].blocks[mapping.blockName].predecessors.includes(edgeM[1])) {
                functions[mapping.funcName].blocks[mapping.blockName].predecessors.push(edgeM[1]);
              }
            }
            i++;
          }
        } else {
          i++;
        }
      }
      continue;
    }

    // 5. Final Liveness Analysis parsing
    if (trimmed.includes('Final Liveness Analysis')) {
      i++;
      let curSec: 'def' | 'use' | 'liveIn' | 'liveOut' | null = null;
      while (
        i < lines.length &&
        !lines[i].startsWith('[Pass]') &&
        !lines[i].startsWith('[MirFrameLowerer]')
      ) {
        const secLine = lines[i].trim();
        if (secLine === 'note: Def') { curSec = 'def'; i++; continue; }
        if (secLine === 'note: Use') { curSec = 'use'; i++; continue; }
        if (secLine === 'note: LiveIn') { curSec = 'liveIn'; i++; continue; }
        if (secLine === 'note: LiveOut') { curSec = 'liveOut'; i++; continue; }

        const m = secLine.match(/%([A-Za-z0-9_]+)\s*\(id=(\d+)/);
        if (m && curSec) {
          const blkName = m[1];
          const blkId = parseInt(m[2], 10);
          const mapping = blockIdToFunc[blkId];

          const targetFuncName = mapping?.funcName || funcNames.find(fn => functions[fn].blocks[blkName]) || funcNames[0];

          if (targetFuncName && functions[targetFuncName]) {
            const f = functions[targetFuncName];
            if (!f.liveness[blkName]) {
              f.liveness[blkName] = { def: [], use: [], liveIn: [], liveOut: [] };
            }
            i++;
            while (i < lines.length) {
              const regLine = lines[i].trim();
              if (
                regLine.startsWith('note:') ||
                regLine.startsWith('[') ||
                regLine === ''
              ) {
                break;
              }
              if (regLine === 'empty') {
                i++;
                break;
              }
              // Check if we hit the next block header in liveness (e.g. "%entry (id=4, instrs=9):")
              if (regLine.match(/^%[A-Za-z0-9_]+\s*\(id=\d+/)) {
                break;
              }
              f.liveness[blkName][curSec].push(regLine);
              i++;
            }
          } else {
            i++;
          }
        } else {
          i++;
        }
      }
      continue;
    }

    // 6. Diagnostics parser
    const diagMatch = trimmed.match(/^\[([A-Za-z0-9_]+)\]\s+(Error|Warning|Trace|Debug):\s+(.*)$/i);
    if (diagMatch) {
      const component = diagMatch[1];
      const severity = diagMatch[2].toLowerCase() as 'error' | 'warning' | 'trace' | 'debug';
      const message = diagMatch[3].trim();

      const diagItem: DiagnosticItem = {
        type: severity,
        component,
        message,
        notes: [],
      };

      if (i + 1 < lines.length) {
        const locLine = lines[i + 1].trim();
        const locMatch = locLine.match(/^(?:.*[/\\])?([^/\\]+\.mir):(\d+):(\d+)$/);
        if (locMatch) {
          diagItem.file = locMatch[1];
          diagItem.line = parseInt(locMatch[2], 10);
          diagItem.col = parseInt(locMatch[3], 10);
          i++;

          const codeSnippetLines: string[] = [];
          while (
            i + 1 < lines.length &&
            (lines[i + 1].includes('|') || lines[i + 1].includes('^'))
          ) {
            codeSnippetLines.push(lines[i + 1]);
            i++;
          }
          if (codeSnippetLines.length > 0) {
            diagItem.sourceExcerpt = codeSnippetLines.join('\n');
          }
        }
      }

      diagnostics.push(diagItem);
      i++;
      continue;
    }

    // Generic error
    const genErrMatch = trimmed.match(/^error:\s+(.*)$/i);
    if (genErrMatch) {
      diagnostics.push({
        type: 'error',
        component: 'EzCompiler',
        message: genErrMatch[1].trim(),
      });
      i++;
      continue;
    }

    i++;
  }

  // Ensure every function has block instruction previews populated
  for (const fName of Object.keys(functions)) {
    const f = functions[fName];
    if (blockInstructions[fName]) {
      for (const bName of Object.keys(blockInstructions[fName])) {
        if (f.blocks[bName]) {
          if (!f.blocks[bName].instructions || f.blocks[bName].instructions.length === 0) {
            f.blocks[bName].instructions = blockInstructions[fName][bName];
          }
        } else if (Object.keys(f.blocks).length === 0) {
          f.blocks[bName] = {
            id: bName,
            instructionCount: blockInstructions[fName][bName].length,
            instructions: blockInstructions[fName][bName],
            successors: [],
            predecessors: [],
          };
        }
      }
    }
  }

  // Determine clean output
  let cleanOutput = '';
  if (allText.includes('function @') || allText.includes('declare @')) {
    cleanOutput = extractCleanMir(allText);
  } else if (allText.includes('.globl ') || allText.includes('.extern ') || allText.includes('.text')) {
    cleanOutput = extractCleanAsm(allText);
  } else {
    cleanOutput = extractCleanAsm(allText);
  }

  return {
    cleanOutput,
    diagnostics,
    passes,
    functions,
  };
}

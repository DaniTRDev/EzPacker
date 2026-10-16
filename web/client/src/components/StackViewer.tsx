import React from 'react';
import { useCompilerStore } from '../store/compilerStore';
import { Server, Shield, Layers, ArrowDown, Cpu, AlertCircle, CheckCircle2 } from 'lucide-react';

export const StackViewer: React.FC = () => {
  const { result, selectedAnalysisFunc, setSelectedAnalysisFunc } = useCompilerStore();

  if (!result || !result.functions || Object.keys(result.functions).length === 0) {
    return (
      <div className="h-full flex flex-col items-center justify-center text-zinc-400 text-xs p-6 text-center">
        <Server className="w-8 h-8 text-zinc-500 mb-2 stroke-[1.5]" />
        <p>No stack frame analysis available.</p>
        <p className="text-[11px] text-zinc-400 mt-1">Compile code to inspect frame layouts and ABI stack allocations.</p>
      </div>
    );
  }

  const funcNames = Object.keys(result.functions);
  const activeFuncName = selectedAnalysisFunc && result.functions[selectedAnalysisFunc]
    ? selectedAnalysisFunc
    : funcNames[0];
  const activeFunc = result.functions[activeFuncName];

  const frameLayout = activeFunc?.frameLayout;
  const calleeSave = frameLayout?.calleeSaveSize ?? 0;
  const totalSize = frameLayout?.totalSize ?? 0;
  const localSize = Math.max(0, totalSize - calleeSave);
  const isLeafFunction = totalSize === 0 && calleeSave === 0;

  // Extract function-specific prologue/epilogue lines from assembly if present
  const asmText = result.asm || '';
  const funcAsmLines = React.useMemo(() => {
    if (!asmText) return [];
    const lines = asmText.split('\n');
    const startIdx = lines.findIndex(l => l.includes(`${activeFuncName}:`));
    if (startIdx === -1) return [];
    const collected: string[] = [];
    for (let i = startIdx; i < lines.length; i++) {
      const line = lines[i];
      // stop at next function label if different
      if (i > startIdx && /^[a-zA-Z0-9_]+:/.test(line.trim())) break;
      collected.push(line);
    }
    return collected;
  }, [asmText, activeFuncName]);

  const prologueInstructions = funcAsmLines
    .filter(l => /push|sub\s+rsp|mov\s+rbp,\s*rsp/.test(l))
    .slice(0, 4);

  const epilogueInstructions = funcAsmLines
    .filter(l => /pop|add\s+rsp|ret/.test(l))
    .slice(-4);

  return (
    <div className="h-full flex flex-col bg-zinc-950 overflow-hidden text-xs">
      {/* Header with Function Switcher */}
      <div className="h-10 px-3 bg-zinc-900/60 border-b border-zinc-800/80 flex items-center justify-between">
        <div className="flex items-center space-x-2">
          <Server className="w-4 h-4 text-emerald-400" />
          <span className="font-semibold text-zinc-200">Stack Frame Analysis (`MirFrameLowerer`)</span>
        </div>

        <div className="flex items-center space-x-2">
          <span className="text-[11px] text-zinc-400">Function:</span>
          <select
            value={activeFuncName}
            onChange={(e) => setSelectedAnalysisFunc(e.target.value)}
            className="h-7 px-2 font-mono text-xs font-medium bg-zinc-950 border border-zinc-800 rounded text-zinc-200 focus:outline-none focus:ring-1 focus:ring-emerald-500/50 cursor-pointer"
          >
            {funcNames.map((name) => (
              <option key={name} value={name}>
                @{name}
              </option>
            ))}
          </select>
        </div>
      </div>

      <div className="flex-1 overflow-y-auto p-4 space-y-5 custom-scrollbar">
        {/* Metric Overview Cards */}
        <div className="grid grid-cols-2 md:grid-cols-4 gap-3">
          <div className="p-3 rounded-lg bg-zinc-900/50 border border-zinc-800/80">
            <div className="flex items-center justify-between">
              <span className="text-[10px] uppercase font-mono tracking-wider text-zinc-400">Callee-Saved</span>
              <Shield className="w-3.5 h-3.5 text-blue-400" />
            </div>
            <div className="text-lg font-bold font-mono text-zinc-100 mt-1">
              {calleeSave} <span className="text-xs font-normal text-zinc-400">bytes</span>
            </div>
            <div className="text-[10px] text-zinc-400 mt-0.5">
              Preserved registers
            </div>
          </div>

          <div className="p-3 rounded-lg bg-zinc-900/50 border border-zinc-800/80">
            <div className="flex items-center justify-between">
              <span className="text-[10px] uppercase font-mono tracking-wider text-zinc-400">Locals & Spills</span>
              <Layers className="w-3.5 h-3.5 text-amber-400" />
            </div>
            <div className="text-lg font-bold font-mono text-amber-300 mt-1">
              {localSize} <span className="text-xs font-normal text-zinc-400">bytes</span>
            </div>
            <div className="text-[10px] text-zinc-400 mt-0.5">
              Spill slots & locals
            </div>
          </div>

          <div className="p-3 rounded-lg bg-zinc-900/50 border border-zinc-800/80">
            <div className="flex items-center justify-between">
              <span className="text-[10px] uppercase font-mono tracking-wider text-zinc-400">Total Frame</span>
              <Server className="w-3.5 h-3.5 text-emerald-400" />
            </div>
            <div className="text-lg font-bold font-mono text-emerald-400 mt-1">
              {totalSize} <span className="text-xs font-normal text-zinc-400">bytes</span>
            </div>
            <div className="text-[10px] text-zinc-400 mt-0.5">
              Allocated in prologue
            </div>
          </div>

          <div className="p-3 rounded-lg bg-zinc-900/50 border border-zinc-800/80">
            <div className="flex items-center justify-between">
              <span className="text-[10px] uppercase font-mono tracking-wider text-zinc-400">ABI Alignment</span>
              <CheckCircle2 className="w-3.5 h-3.5 text-emerald-400" />
            </div>
            <div className="text-lg font-bold font-mono text-zinc-100 mt-1">
              16 <span className="text-xs font-normal text-zinc-400">bytes</span>
            </div>
            <div className="text-[10px] text-zinc-400 mt-0.5">
              x86_64 Stack Alignment
            </div>
          </div>
        </div>

        {/* Stack Memory Layout Schematic */}
        <div className="rounded-lg border border-zinc-800/80 bg-zinc-900/30 overflow-hidden">
          <div className="px-3.5 py-2.5 bg-zinc-900/80 border-b border-zinc-800/80 flex items-center justify-between">
            <div className="flex items-center space-x-2">
              <Server className="w-4 h-4 text-emerald-400" />
              <span className="font-semibold text-zinc-200">x86_64 Stack Frame Memory Hierarchy</span>
            </div>
            <span className="text-[10px] font-mono text-zinc-400">High Address ➜ Low Address</span>
          </div>

          <div className="p-4 space-y-2">
            {/* Return Address */}
            <div className="p-3 rounded border border-zinc-800 bg-zinc-950 flex items-center justify-between font-mono">
              <div className="flex items-center space-x-2.5">
                <span className="w-2 h-2 rounded-full bg-zinc-600"></span>
                <div>
                  <div className="text-xs font-medium text-zinc-300">Return Address (`[rsp + {totalSize + 8}]`)</div>
                  <div className="text-[10px] text-zinc-400 font-sans">Pushed automatically by caller's `call` instruction</div>
                </div>
              </div>
              <span className="text-xs text-zinc-400 font-semibold">8 bytes</span>
            </div>

            <div className="flex justify-center my-0.5">
              <ArrowDown className="w-3.5 h-3.5 text-zinc-600" />
            </div>

            {/* Callee-Saved Registers */}
            <div className={`p-3 rounded border font-mono transition-colors ${
              calleeSave > 0
                ? 'border-blue-500/40 bg-blue-950/20 text-blue-200'
                : 'border-zinc-800/60 bg-zinc-950/40 text-zinc-400'
            } flex items-center justify-between`}>
              <div className="flex items-center space-x-2.5">
                <span className={`w-2 h-2 rounded-full ${calleeSave > 0 ? 'bg-blue-400' : 'bg-zinc-700'}`}></span>
                <div>
                  <div className="text-xs font-medium text-zinc-200">
                    Callee-Saved Registers {calleeSave > 0 ? `(\`[rsp + ${localSize}]\`)` : '(None needed)'}
                  </div>
                  <div className="text-[10px] text-zinc-400 font-sans">
                    {calleeSave > 0 ? 'Preserved across call ABI boundaries' : 'No callee registers clobbered'}
                  </div>
                </div>
              </div>
              <span className="text-xs font-semibold">{calleeSave} bytes</span>
            </div>

            <div className="flex justify-center my-0.5">
              <ArrowDown className="w-3.5 h-3.5 text-zinc-600" />
            </div>

            {/* Spill Slots & Locals */}
            <div className={`p-3 rounded border font-mono transition-colors ${
              localSize > 0
                ? 'border-amber-500/40 bg-amber-950/20 text-amber-200'
                : 'border-zinc-800/60 bg-zinc-950/40 text-zinc-400'
            } flex items-center justify-between`}>
              <div className="flex items-center space-x-2.5">
                <span className={`w-2 h-2 rounded-full ${localSize > 0 ? 'bg-amber-400' : 'bg-zinc-700'}`}></span>
                <div>
                  <div className="text-xs font-medium text-zinc-200">
                    Local Variables & Spilled Registers (`[rsp + 0]`)
                  </div>
                  <div className="text-[10px] text-zinc-400 font-sans">
                    {localSize > 0
                      ? 'Spill slots allocated during register allocation and lowering'
                      : 'All values kept in machine registers (zero spills)'}
                  </div>
                </div>
              </div>
              <span className="text-xs font-semibold">{localSize} bytes</span>
            </div>

            {/* Bottom RSP pointer */}
            <div className="pt-2 flex items-center justify-between text-[11px] font-mono text-emerald-400">
              <span className="flex items-center space-x-1">
                <span>▲</span>
                <span>Current Stack Pointer (RSP)</span>
              </span>
              <span>Total Offset: {totalSize} bytes</span>
            </div>
          </div>
        </div>

        {/* Leaf Function or Stack Details Banner */}
        {isLeafFunction ? (
          <div className="p-3.5 rounded-lg border border-emerald-500/30 bg-emerald-950/20 flex items-start space-x-3">
            <CheckCircle2 className="w-4 h-4 text-emerald-400 shrink-0 mt-0.5" />
            <div className="space-y-1">
              <div className="font-semibold text-emerald-300">Leaf Function Optimization Active</div>
              <p className="text-[11px] text-zinc-300 leading-relaxed">
                Function <code className="font-mono text-emerald-400">@{activeFuncName}</code> does not require stack memory allocation.
                All active values fit comfortably inside hardware machine registers without spills, and no external calls requiring shadow store space are invoked.
              </p>
            </div>
          </div>
        ) : (
          <div className="p-3.5 rounded-lg border border-zinc-800/80 bg-zinc-900/40 flex items-start space-x-3">
            <AlertCircle className="w-4 h-4 text-blue-400 shrink-0 mt-0.5" />
            <div className="space-y-1">
              <div className="font-semibold text-zinc-200">Frame Layout Construction</div>
              <p className="text-[11px] text-zinc-300 leading-relaxed">
                <code className="font-mono text-zinc-100">MirFrameLowererPass</code> calculated a total stack requirement of{' '}
                <span className="text-emerald-400 font-mono font-semibold">{totalSize} bytes</span> (
                <span className="text-blue-300 font-mono">{calleeSave}B callee</span> +{' '}
                <span className="text-amber-300 font-mono">{localSize}B local</span>
                ), aligned to a 16-byte boundary per x86_64 ABI calling conventions.
              </p>
            </div>
          </div>
        )}

        {/* Prologue / Epilogue Preview if available */}
        {(prologueInstructions.length > 0 || epilogueInstructions.length > 0) && (
          <div className="grid grid-cols-1 md:grid-cols-2 gap-3">
            <div className="p-3 rounded-lg border border-zinc-800/80 bg-zinc-900/30">
              <div className="flex items-center space-x-2 pb-2 border-b border-zinc-800/80">
                <Cpu className="w-3.5 h-3.5 text-emerald-400" />
                <span className="font-mono font-bold text-zinc-200 text-xs">Generated Prologue</span>
              </div>
              <div className="mt-2 space-y-1 font-mono text-[11px]">
                {prologueInstructions.length > 0 ? (
                  prologueInstructions.map((inst, i) => (
                    <div key={i} className="text-zinc-300 bg-zinc-950/60 px-2 py-1 rounded border border-zinc-800/50">
                      {inst.trim()}
                    </div>
                  ))
                ) : (
                  <div className="text-zinc-400 italic">No explicit prologue instructions</div>
                )}
              </div>
            </div>

            <div className="p-3 rounded-lg border border-zinc-800/80 bg-zinc-900/30">
              <div className="flex items-center space-x-2 pb-2 border-b border-zinc-800/80">
                <Cpu className="w-3.5 h-3.5 text-rose-400" />
                <span className="font-mono font-bold text-zinc-200 text-xs">Generated Epilogue</span>
              </div>
              <div className="mt-2 space-y-1 font-mono text-[11px]">
                {epilogueInstructions.length > 0 ? (
                  epilogueInstructions.map((inst, i) => (
                    <div key={i} className="text-zinc-300 bg-zinc-950/60 px-2 py-1 rounded border border-zinc-800/50">
                      {inst.trim()}
                    </div>
                  ))
                ) : (
                  <div className="text-zinc-400 italic">No explicit epilogue instructions</div>
                )}
              </div>
            </div>
          </div>
        )}
      </div>
    </div>
  );
};
